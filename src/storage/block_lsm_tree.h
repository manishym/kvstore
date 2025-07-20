#pragma once
#include "wal/wal_entry_serializer.h"
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

class BlockLSMTree {
public:
    explicit BlockLSMTree(const nlohmann::json& config);
    ~BlockLSMTree();

    void put(const std::string& key, const std::string& value);
    void del(const std::string& key);
    std::optional<std::string> get(const std::string& key);
    size_t memtableSize() const { return memtable_.size(); }

    void flush();

private:
    using OptValue = std::optional<std::string>;
    std::map<std::string, OptValue> memtable_;
    size_t memtable_max_;
    std::string dir_;
    std::vector<std::string> sst_files_;
    int next_file_id_ = 0;

    void flushMemTable();
    bool searchSST(const std::string& path, const std::string& key, OptValue& value);
    static bool writeAll(int fd, const uint8_t* data, size_t len);
};
