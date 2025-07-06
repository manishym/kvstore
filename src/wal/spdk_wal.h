#pragma once
#include "wal/interface.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class SpdkWAL : public WAL {
public:
  explicit SpdkWAL(const nlohmann::json &deviceConfig) {
    if (deviceConfig.is_string()) {
      // Backward compatibility: treat as simple device name
      spdk_bdev_ = deviceConfig.get<std::string>();
      wal_segment_size_ = 67108864; // 64MB default
      batch_size_ = 32;             // default batch size
    } else if (deviceConfig.is_object()) {
      // New configuration format
      spdk_bdev_ = deviceConfig.value("spdk_bdev", "NVMe0n1");
      wal_segment_size_ = deviceConfig.value("wal_segment_size", 67108864);
      batch_size_ = deviceConfig.value("batch_size", 32);
    }
  }

  ~SpdkWAL() override = default;

  bool append(const WalEntry &) override { return false; }
  bool appendBatch(const std::vector<WalEntry> &) override { return false; }
  bool sync() override { return false; }
  std::vector<WalEntry> replay() override { return {}; }
  bool rollSegment() override { return false; }
  bool truncate(uint64_t) override { return false; }
  uint64_t currentSegmentId() const override { return 0; }

private:
  std::string spdk_bdev_;
  uint64_t wal_segment_size_;
  uint32_t batch_size_;
};
