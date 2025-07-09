#pragma once
#include "wal/interface.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class SpdkWAL : public WAL {
public:
  explicit SpdkWAL(const nlohmann::json &deviceConfig);
  ~SpdkWAL() override;

  bool append(const WalEntry &entry) override;
  bool appendBatch(const std::vector<WalEntry> &entries) override;
  bool sync() override;
  std::vector<WalEntry> replay() override;
  bool rollSegment() override;
  bool truncate(uint64_t upToSegmentId) override;
  uint64_t currentSegmentId() const override;

private:
  struct WriteTask {
    std::vector<uint8_t> data;
  };

  bool writeBuffer(const uint8_t *data, size_t len);
  void workerThread();

  std::string spdk_bdev_;
  uint64_t wal_segment_size_{};
  uint32_t batch_size_{};

  struct spdk_bdev *bdev_{nullptr};
  struct spdk_bdev_desc *desc_{nullptr};
  struct spdk_io_channel *io_channel_{nullptr};
  uint32_t block_size_{0};
  uint64_t current_segment_{0};
  uint64_t current_offset_{0};
  size_t bytes_in_segment_{0};

  std::thread worker_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::deque<WriteTask> queue_;
  std::atomic<bool> stop_{false};
  std::atomic<uint64_t> outstanding_{0};
  std::mutex completion_mu_;
  std::condition_variable completion_cv_;
};
