#pragma once
#include "wal.h"
#include <vector>

class SpdkWAL : public WAL {
public:
  explicit SpdkWAL(const std::string &device) { (void)device; }
  ~SpdkWAL() override = default;

  bool append(const WalEntry &) override { return false; }
  bool appendBatch(const std::vector<WalEntry> &) override { return false; }
  bool sync() override { return false; }
  std::vector<WalEntry> replay() override { return {}; }
  bool rollSegment() override { return false; }
  bool truncate(uint64_t) override { return false; }
  uint64_t currentSegmentId() const override { return 0; }
};
