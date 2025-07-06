#pragma once
#include "wal/interface.h"
#include "wal/wal_entry_serializer.h"
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>
#include <unistd.h>
#include <vector>

class BlockDeviceWAL : public WAL {
public:
  explicit BlockDeviceWAL(const std::string &path) {
    fd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);
    if (fd_ < 0)
      throw std::runtime_error("Failed to open WAL device");
  }

  template <typename T>
  explicit BlockDeviceWAL(
      const T &deviceConfig,
      typename std::enable_if<std::is_same<T, nlohmann::json>::value>::type * =
          nullptr) {
    std::string path;
    if (deviceConfig.is_object()) {
      // New configuration format
      path = deviceConfig.value("path", "kvstore_block.wal");
    } else {
      path = "kvstore_block.wal"; // fallback
    }

    fd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);
    if (fd_ < 0)
      throw std::runtime_error("Failed to open WAL device: " + path);
  }

  ~BlockDeviceWAL() override {
    if (fd_ >= 0)
      ::close(fd_);
  }

  bool append(const WalEntry &entry) override {
    std::lock_guard<std::mutex> lock(mu_);
    auto buf = WalEntrySerializer::serialize(entry);
    return writeAll(fd_, buf.data(), buf.size());
  }

  bool appendBatch(const std::vector<WalEntry> &entries) override {
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto &e : entries) {
      auto buf = WalEntrySerializer::serialize(e);
      if (!writeAll(fd_, buf.data(), buf.size()))
        return false;
    }
    return true;
  }

  bool sync() override {
    std::lock_guard<std::mutex> lock(mu_);
    return ::fsync(fd_) == 0;
  }

  std::vector<WalEntry> replay() override {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<WalEntry> entries;
    ::lseek(fd_, 0, SEEK_SET);
    while (true) {
      uint8_t header[1 + sizeof(uint32_t) * 2];
      ssize_t r = ::read(fd_, header, sizeof(header));
      if (r == 0)
        break;
      if (r != (ssize_t)sizeof(header))
        break;
      uint32_t klen, vlen;
      std::memcpy(&klen, header + 1, sizeof(uint32_t));
      std::memcpy(&vlen, header + 1 + sizeof(uint32_t), sizeof(uint32_t));
      std::vector<uint8_t> buf(header, header + sizeof(header));
      buf.resize(sizeof(header) + klen + vlen);
      if (::read(fd_, buf.data() + sizeof(header), klen + vlen) !=
          (ssize_t)(klen + vlen))
        break;
      WalEntry e;
      if (!WalEntrySerializer::deserialize(
              std::string_view(reinterpret_cast<char *>(buf.data()),
                               buf.size()),
              e))
        break;
      entries.push_back(std::move(e));
    }
    ::lseek(fd_, 0, SEEK_END);
    return entries;
  }

  bool rollSegment() override { return true; }
  bool truncate(uint64_t) override { return true; }
  uint64_t currentSegmentId() const override { return 0; }

private:
  static bool writeAll(int fd, const uint8_t *data, size_t len) {
    while (len > 0) {
      ssize_t w = ::write(fd, data, len);
      if (w <= 0)
        return false;
      data += w;
      len -= w;
    }
    return true;
  }

  int fd_;
  mutable std::mutex mu_;
};
