#pragma once
#include "wal.h"
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <stdexcept>
#include <vector>

class BlockDeviceWAL : public WAL {
public:
  explicit BlockDeviceWAL(const std::string &path) {
    fd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);
    if (fd_ < 0)
      throw std::runtime_error("Failed to open WAL device");
  }

  ~BlockDeviceWAL() override {
    if (fd_ >= 0)
      ::close(fd_);
  }

  bool append(const WalEntry &entry) override {
    std::lock_guard<std::mutex> lock(mu_);
    return writeEntry(entry);
  }

  bool appendBatch(const std::vector<WalEntry> &entries) override {
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto &e : entries)
      if (!writeEntry(e))
        return false;
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
      WalEntry e;
      uint8_t op;
      uint32_t klen, vlen;
      ssize_t r = ::read(fd_, &op, 1);
      if (r == 0)
        break;
      if (r != 1)
        break;
      if (::read(fd_, &klen, sizeof(klen)) != sizeof(klen))
        break;
      if (::read(fd_, &vlen, sizeof(vlen)) != sizeof(vlen))
        break;
      e.op_type = static_cast<WalOpType>(op);
      e.key.resize(klen);
      if (::read(fd_, e.key.data(), klen) != (ssize_t)klen)
        break;
      e.value.resize(vlen);
      if (vlen > 0 && ::read(fd_, e.value.data(), vlen) != (ssize_t)vlen)
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
  bool writeEntry(const WalEntry &entry) {
    uint8_t op = static_cast<uint8_t>(entry.op_type);
    uint32_t klen = entry.key.size();
    uint32_t vlen = entry.value.size();
    if (::write(fd_, &op, 1) != 1)
      return false;
    if (::write(fd_, &klen, sizeof(klen)) != sizeof(klen))
      return false;
    if (::write(fd_, &vlen, sizeof(vlen)) != sizeof(vlen))
      return false;
    if (::write(fd_, entry.key.data(), klen) != (ssize_t)klen)
      return false;
    if (vlen && ::write(fd_, entry.value.data(), vlen) != (ssize_t)vlen)
      return false;
    return true;
  }

  int fd_;
  mutable std::mutex mu_;
};
