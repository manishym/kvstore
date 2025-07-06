#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Operation type for the log
enum class WalOpType : uint8_t { PUT = 1, DELETE = 2 };

// Log entry (you can extend this for compression, timestamp, etc.)
struct WalEntry {
  WalOpType op_type;
  std::string key;
  std::string value; // Optional: for DELETE, this can be empty
};

// Interface for the Write-Ahead Log
class WAL {
public:
  virtual ~WAL() = default;

  // Append a single log entry to the WAL
  virtual bool append(const WalEntry &entry) = 0;

  // Batch append for higher throughput
  virtual bool appendBatch(const std::vector<WalEntry> &entries) = 0;

  // Sync WAL to durable storage (fsync or flush for SPDK)
  virtual bool sync() = 0;

  // Called on startup to replay all WAL entries since last flush
  virtual std::vector<WalEntry> replay() = 0;

  // Roll over to the next WAL segment
  virtual bool rollSegment() = 0;

  // Delete WAL files that are no longer needed
  virtual bool truncate(uint64_t upToSegmentId) = 0;

  // Optional: get current segment ID or WAL state for metrics
  virtual uint64_t currentSegmentId() const = 0;
};
