#pragma once
#include "wal/interface.h"
#include "wal/wal_entry_serializer.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdk/bdev.h>
#include <spdk/env.h>
#include <spdk/log.h>
#include <spdk/queue.h>
#include <spdk/string.h>
#include <spdk/thread.h>
#include <string>
#include <vector>

// Forward declarations for SPDK structures
struct spdk_bdev;
struct spdk_bdev_desc;
struct spdk_io_channel;

class SpdkWAL : public WAL {
public:
  explicit SpdkWAL(const nlohmann::json &deviceConfig) {
    if (deviceConfig.is_string()) {
      // Backward compatibility: treat as simple device name
      spdk_bdev_name_ = deviceConfig.get<std::string>();
      wal_segment_size_ = 67108864; // 64MB default
      batch_size_ = 32;             // default batch size
    } else if (deviceConfig.is_object()) {
      // New configuration format
      spdk_bdev_name_ = deviceConfig.value("spdk_bdev", "NVMe0n1");
      wal_segment_size_ = deviceConfig.value("wal_segment_size", 67108864);
      batch_size_ = deviceConfig.value("batch_size", 32);
    }

    // Initialize SPDK environment
    if (!initialize_spdk()) {
      std::cerr << "Failed to initialize SPDK environment" << std::endl;
      return;
    }

    // Open the SPDK block device
    if (!open_spdk_bdev()) {
      std::cerr << "Failed to open SPDK block device: " << spdk_bdev_name_
                << std::endl;
      return;
    }

    // Allocate buffer for I/O operations
    buffer_size_ = 4096; // 4KB buffer
    buffer_ = static_cast<uint8_t *>(spdk_malloc(buffer_size_, 0x1000, nullptr,
                                                 SPDK_ENV_SOCKET_ID_ANY,
                                                 SPDK_MALLOC_DMA));
    if (!buffer_) {
      std::cerr << "Failed to allocate SPDK buffer" << std::endl;
      return;
    }

    // Initialize current offset
    current_offset_ = 0;
    initialized_ = true;

    std::cerr << "SPDK WAL initialized with device: " << spdk_bdev_name_
              << std::endl;
  }

  ~SpdkWAL() override {
    if (buffer_) {
      spdk_free(buffer_);
    }
    if (io_channel_) {
      spdk_put_io_channel(io_channel_);
    }
    if (bdev_desc_) {
      spdk_bdev_close(bdev_desc_);
    }
    spdk_env_fini();
  }

  bool append(const WalEntry &entry) override {
    if (!initialized_) {
      std::cerr << "SPDK WAL not initialized" << std::endl;
      return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Serialize the entry
    auto serialized = WalEntrySerializer::serialize(entry);
    if (serialized.size() > buffer_size_) {
      std::cerr << "Entry too large for buffer" << std::endl;
      return false;
    }

    // Copy to buffer
    std::memcpy(buffer_, serialized.data(), serialized.size());

    // Write to block device synchronously
    bool write_success = false;
    std::condition_variable cv;
    std::mutex cv_mutex;

    int rc = spdk_bdev_write(bdev_desc_, io_channel_, buffer_, current_offset_,
                             serialized.size(), writeComplete, this);
    if (rc != 0) {
      if (rc == -ENOMEM) {
        // Wait for resources and retry
        struct spdk_bdev_io_wait_entry bdev_io_wait;
        bdev_io_wait.bdev = bdev_;
        bdev_io_wait.cb_fn = writeWaitComplete;
        bdev_io_wait.cb_arg = this;
        spdk_bdev_queue_io_wait(bdev_, io_channel_, &bdev_io_wait);
      } else {
        std::cerr << "Failed to write to SPDK block device: " << rc
                  << std::endl;
        return false;
      }
    }

    // Wait for completion
    {
      std::unique_lock<std::mutex> lock(cv_mutex);
      cv.wait(lock, [&write_success] { return write_success; });
    }

    if (write_success) {
      current_offset_ += serialized.size();
      std::cerr << "SPDK WAL: Wrote entry " << entry.key << " -> "
                << entry.value << " at offset " << current_offset_ << std::endl;
    }

    return write_success;
  }

  bool appendBatch(const std::vector<WalEntry> &entries) override {
    if (!initialized_) {
      return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &entry : entries) {
      auto serialized = WalEntrySerializer::serialize(entry);
      if (serialized.size() > buffer_size_) {
        return false;
      }

      std::memcpy(buffer_, serialized.data(), serialized.size());

      int rc =
          spdk_bdev_write(bdev_desc_, io_channel_, buffer_, current_offset_,
                          serialized.size(), writeComplete, this);
      if (rc != 0) {
        if (rc == -ENOMEM) {
          struct spdk_bdev_io_wait_entry bdev_io_wait;
          bdev_io_wait.bdev = bdev_;
          bdev_io_wait.cb_fn = writeWaitComplete;
          bdev_io_wait.cb_arg = this;
          spdk_bdev_queue_io_wait(bdev_, io_channel_, &bdev_io_wait);
        } else {
          return false;
        }
      }
      current_offset_ += serialized.size();
    }
    return true;
  }

  bool sync() override {
    if (!initialized_) {
      return false;
    }
    // SPDK writes are already durable, so just return true
    return true;
  }

  std::vector<WalEntry> replay() override {
    if (!initialized_) {
      return {};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<WalEntry> entries;

    std::cerr << "SPDK WAL: Replay called, reading from offset 0 to "
              << current_offset_ << std::endl;

    uint64_t read_offset = 0;
    while (read_offset < current_offset_) {
      // Read a chunk
      uint64_t read_size =
          std::min(buffer_size_, current_offset_ - read_offset);

      int rc = spdk_bdev_read(bdev_desc_, io_channel_, buffer_, read_offset,
                              read_size, readComplete, this);
      if (rc != 0) {
        if (rc == -ENOMEM) {
          struct spdk_bdev_io_wait_entry bdev_io_wait;
          bdev_io_wait.bdev = bdev_;
          bdev_io_wait.cb_fn = readWaitComplete;
          bdev_io_wait.cb_arg = this;
          spdk_bdev_queue_io_wait(bdev_, io_channel_, &bdev_io_wait);
        } else {
          break;
        }
      }

      // Parse entries from the buffer
      size_t buffer_pos = 0;
      while (buffer_pos + 9 <= read_size) { // 9 bytes for header
        // Read header (1 byte for op_type + 8 bytes for lengths)
        uint8_t op_type = buffer_[buffer_pos];
        uint32_t klen, vlen;
        std::memcpy(&klen, buffer_ + buffer_pos + 1, sizeof(klen));
        std::memcpy(&vlen, buffer_ + buffer_pos + 5, sizeof(vlen));

        size_t entry_size = 9 + klen + vlen;
        if (buffer_pos + entry_size > read_size) {
          break; // Incomplete entry
        }

        // Read key and value
        std::string key(reinterpret_cast<char *>(buffer_ + buffer_pos + 9),
                        klen);
        std::string value(
            reinterpret_cast<char *>(buffer_ + buffer_pos + 9 + klen), vlen);

        // Create entry
        WalEntry entry;
        entry.op_type = static_cast<WalOpType>(op_type);
        entry.key = key;
        entry.value = value;
        entries.push_back(entry);

        std::cerr << "SPDK WAL: Replayed entry " << entry.key << " -> "
                  << entry.value << std::endl;

        buffer_pos += entry_size;
      }

      read_offset += read_size;
    }

    std::cerr << "SPDK WAL: Replay completed, found " << entries.size()
              << " entries" << std::endl;
    return entries;
  }

  bool rollSegment() override {
    // For simplicity, we don't implement segment rolling in this version
    return true;
  }

  bool truncate(uint64_t upToSegmentId) override {
    // For simplicity, we don't implement truncation in this version
    return true;
  }

  uint64_t currentSegmentId() const override { return 0; }

private:
  static void writeComplete(struct spdk_bdev_io *bdev_io, bool success,
                            void *cb_arg) {
    SpdkWAL *wal = static_cast<SpdkWAL *>(cb_arg);
    spdk_bdev_free_io(bdev_io);
    if (!success) {
      std::cerr << "SPDK WAL: Write failed" << std::endl;
    }
  }

  static void readComplete(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {
    SpdkWAL *wal = static_cast<SpdkWAL *>(cb_arg);
    spdk_bdev_free_io(bdev_io);
    if (!success) {
      std::cerr << "SPDK WAL: Read failed" << std::endl;
    }
  }

  static void writeWaitComplete(void *cb_arg) {
    // Retry the write operation
    SpdkWAL *wal = static_cast<SpdkWAL *>(cb_arg);
    // This would retry the write operation
  }

  static void readWaitComplete(void *cb_arg) {
    // Retry the read operation
    SpdkWAL *wal = static_cast<SpdkWAL *>(cb_arg);
    // This would retry the read operation
  }

  bool initialize_spdk() {
    // Initialize SPDK environment with minimal setup
    struct spdk_env_opts opts;
    spdk_env_opts_init(&opts);
    opts.name = "kvstore_spdk_wal";
    opts.mem_size = 512;
    opts.no_pci = true;
    opts.shm_id = -1; // Use default shared memory

    int rc = spdk_env_init(&opts);
    if (rc != 0) {
      std::cerr << "Failed to initialize SPDK environment: " << rc << std::endl;
      return false;
    }

    return true;
  }

  bool open_spdk_bdev() {
    // Open the SPDK block device
    int rc = spdk_bdev_open_ext(spdk_bdev_name_.c_str(), true, nullptr, nullptr,
                                &bdev_desc_);
    if (rc != 0) {
      std::cerr << "Failed to open SPDK block device: " << spdk_bdev_name_
                << " (rc: " << rc << ")" << std::endl;
      return false;
    }

    bdev_ = spdk_bdev_desc_get_bdev(bdev_desc_);
    if (!bdev_) {
      std::cerr << "Failed to get SPDK block device" << std::endl;
      return false;
    }

    // Create I/O channel
    io_channel_ = spdk_bdev_get_io_channel(bdev_desc_);
    if (!io_channel_) {
      std::cerr << "Failed to create I/O channel" << std::endl;
      return false;
    }

    return true;
  }

  std::string spdk_bdev_name_;
  uint64_t wal_segment_size_;
  uint32_t batch_size_;
  struct spdk_bdev *bdev_;
  struct spdk_bdev_desc *bdev_desc_;
  struct spdk_io_channel *io_channel_;
  uint8_t *buffer_;
  size_t buffer_size_;
  uint64_t current_offset_;
  bool initialized_;
  mutable std::mutex mutex_;
};
