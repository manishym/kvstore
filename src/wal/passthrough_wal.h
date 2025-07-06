#pragma once
#include "wal/interface.h"
#include <mutex>
#include <vector>

class PassThroughWAL : public WAL {
public:
  bool append(const WalEntry &entry) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(entry);
    return true;
  }

  bool appendBatch(const std::vector<WalEntry> &batch) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.insert(entries_.end(), batch.begin(), batch.end());
    return true;
  }

  bool sync() override { return true; }

  std::vector<WalEntry> replay() override {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_; // Return all stored entries
  }

  bool rollSegment() override { return true; }
  bool truncate(uint64_t) override { return true; }
  uint64_t currentSegmentId() const override { return 0; }

private:
  std::vector<WalEntry> entries_;
  mutable std::mutex mutex_;
};
