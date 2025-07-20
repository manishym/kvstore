#include "storage/block_lsm_tree.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

BlockLSMTree::BlockLSMTree(const nlohmann::json& config) {
    const nlohmann::json& lsmCfg =
        (config.contains("lsm") && config["lsm"].is_object()) ? config["lsm"] : nlohmann::json::object();
    memtable_max_ = lsmCfg.value("memtable_size", 1024);
    dir_ = lsmCfg.value("directory", std::string("data"));
    std::filesystem::create_directories(dir_);
}

BlockLSMTree::~BlockLSMTree() {
    if (!memtable_.empty())
        flushMemTable();
}

void BlockLSMTree::put(const std::string& key, const std::string& value) {
    memtable_[key] = value;
    if (memtable_.size() >= memtable_max_)
        flushMemTable();
}

void BlockLSMTree::del(const std::string& key) {
    memtable_[key] = std::nullopt;
    if (memtable_.size() >= memtable_max_)
        flushMemTable();
}

std::optional<std::string> BlockLSMTree::get(const std::string& key) {
    auto it = memtable_.find(key);
    if (it != memtable_.end()) {
        return it->second; // optional may be null
    }
    for (auto itf = sst_files_.rbegin(); itf != sst_files_.rend(); ++itf) {
        OptValue v;
        if (searchSST(*itf, key, v))
            return v;
    }
    return std::nullopt;
}

void BlockLSMTree::flush() { flushMemTable(); }

void BlockLSMTree::flushMemTable() {
    if (memtable_.empty())
        return;
    std::string path = dir_ + "/sst_" + std::to_string(next_file_id_++) + ".sst";
    int fd = ::open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0)
        return;
    for (const auto& kv : memtable_) {
        WalEntry entry;
        if (kv.second.has_value()) {
            entry = {WalOpType::PUT, kv.first, kv.second.value()};
        } else {
            entry = {WalOpType::DELETE, kv.first, ""};
        }
        auto buf = WalEntrySerializer::serialize(entry);
        if (!writeAll(fd, buf.data(), buf.size())) {
            break;
        }
    }
    ::fsync(fd);
    ::close(fd);
    sst_files_.push_back(path);
    memtable_.clear();
}

bool BlockLSMTree::searchSST(const std::string& path, const std::string& key, OptValue& value) {
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;
    while (true) {
        uint8_t header[1 + sizeof(uint32_t)*2];
        ssize_t r = ::read(fd, header, sizeof(header));
        if (r == 0)
            break;
        if (r != (ssize_t)sizeof(header)) {
            ::close(fd);
            return false;
        }
        uint32_t klen, vlen;
        std::memcpy(&klen, header+1, sizeof(uint32_t));
        std::memcpy(&vlen, header+1+sizeof(uint32_t), sizeof(uint32_t));
        std::vector<uint8_t> buf(sizeof(header)+klen+vlen);
        std::memcpy(buf.data(), header, sizeof(header));
        if (::read(fd, buf.data()+sizeof(header), klen+vlen) != (ssize_t)(klen+vlen)) {
            ::close(fd);
            return false;
        }
        WalEntry e;
        if (!WalEntrySerializer::deserialize(std::string_view(reinterpret_cast<char*>(buf.data()), buf.size()), e)) {
            ::close(fd);
            return false;
        }
        if (e.key == key) {
            if (e.op_type == WalOpType::PUT)
                value = e.value;
            else
                value = std::nullopt;
            ::close(fd);
            return true;
        }
    }
    ::close(fd);
    return false;
}

bool BlockLSMTree::writeAll(int fd, const uint8_t* data, size_t len) {
    while (len > 0) {
        ssize_t w = ::write(fd, data, len);
        if (w <= 0)
            return false;
        data += w;
        len -= w;
    }
    return true;
}
