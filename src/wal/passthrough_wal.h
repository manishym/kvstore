#pragma once
#include "wal/interface.h"
#include <vector>

class PassThroughWAL : public WAL {
public:
  bool append(const WalEntry &) override { return true; }
  bool appendBatch(const std::vector<WalEntry> &) override { return true; }
  bool sync() override { return true; }
  std::vector<WalEntry> replay() override { return {}; }
  bool rollSegment() override { return true; }
  bool truncate(uint64_t) override { return true; }
  uint64_t currentSegmentId() const override { return 0; }
};
