#include "wal/spdk_wal.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <vector>

class SpdkWALTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Common setup if needed
  }

  void TearDown() override {
    // Common cleanup if needed
  }
};

// Test configuration parsing with string device name (backward compatibility)
TEST_F(SpdkWALTest, ConstructorWithStringDevice) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  // Since the current implementation is a stub, we can only test that it
  // constructs without throwing
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with JSON object
TEST_F(SpdkWALTest, ConstructorWithJsonObject) {
  nlohmann::json config = {{"spdk_bdev", "NVMe1n1"},
                           {"wal_segment_size", 134217728}, // 128MB
                           {"batch_size", 64}};

  SpdkWAL wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with default values
TEST_F(SpdkWALTest, ConstructorWithPartialConfig) {
  nlohmann::json config = {
      {"spdk_bdev", "NVMe2n1"}
      // Missing wal_segment_size and batch_size should use defaults
  };

  SpdkWAL wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with empty object
TEST_F(SpdkWALTest, ConstructorWithEmptyObject) {
  nlohmann::json config = {};

  SpdkWAL wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test append method (currently returns false)
TEST_F(SpdkWALTest, AppendReturnsFalse) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  WalEntry entry{WalOpType::PUT, "test_key", "test_value"};
  EXPECT_FALSE(wal.append(entry));
}

// Test appendBatch method (currently returns false)
TEST_F(SpdkWALTest, AppendBatchReturnsFalse) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  std::vector<WalEntry> entries = {{WalOpType::PUT, "key1", "value1"},
                                   {WalOpType::DELETE, "key2", ""},
                                   {WalOpType::PUT, "key3", "value3"}};

  EXPECT_FALSE(wal.appendBatch(entries));
}

// Test sync method (currently returns false)
TEST_F(SpdkWALTest, SyncReturnsFalse) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  EXPECT_FALSE(wal.sync());
}

// Test replay method (currently returns empty vector)
TEST_F(SpdkWALTest, ReplayReturnsEmpty) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  auto entries = wal.replay();
  EXPECT_TRUE(entries.empty());
}

// Test rollSegment method (currently returns false)
TEST_F(SpdkWALTest, RollSegmentReturnsFalse) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  EXPECT_FALSE(wal.rollSegment());
}

// Test truncate method (currently returns false)
TEST_F(SpdkWALTest, TruncateReturnsFalse) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  EXPECT_FALSE(wal.truncate(123));
}

// Test currentSegmentId method (currently returns 0)
TEST_F(SpdkWALTest, CurrentSegmentIdReturnsZero) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  EXPECT_EQ(wal.currentSegmentId(), 0);
}

// Test with different operation types
TEST_F(SpdkWALTest, DifferentOperationTypes) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  WalEntry putEntry{WalOpType::PUT, "put_key", "put_value"};
  WalEntry deleteEntry{WalOpType::DELETE, "delete_key", ""};

  EXPECT_FALSE(wal.append(putEntry));
  EXPECT_FALSE(wal.append(deleteEntry));
}

// Test with empty key and value
TEST_F(SpdkWALTest, EmptyKeyAndValue) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  WalEntry emptyEntry{WalOpType::PUT, "", ""};
  EXPECT_FALSE(wal.append(emptyEntry));
}

// Test with large values
TEST_F(SpdkWALTest, LargeValues) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  std::string largeValue(1000, 'x'); // 1000 character value
  WalEntry largeEntry{WalOpType::PUT, "large_key", largeValue};
  EXPECT_FALSE(wal.append(largeEntry));
}

// Test batch operations with empty batch
TEST_F(SpdkWALTest, EmptyBatch) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  std::vector<WalEntry> emptyBatch;
  EXPECT_FALSE(wal.appendBatch(emptyBatch));
}

// Test multiple configurations
TEST_F(SpdkWALTest, MultipleConfigurations) {
  std::vector<nlohmann::json> configs = {
      "NVMe0n1",
      {{"spdk_bdev", "NVMe1n1"}},
      {{"spdk_bdev", "NVMe2n1"}, {"wal_segment_size", 33554432}},
      {{"spdk_bdev", "NVMe3n1"}, {"batch_size", 128}},
      {{"spdk_bdev", "NVMe4n1"},
       {"wal_segment_size", 16777216},
       {"batch_size", 16}}};

  for (const auto &config : configs) {
    SpdkWAL wal(config);
    WalEntry entry{WalOpType::PUT, "test_key", "test_value"};
    EXPECT_FALSE(wal.append(entry));
  }
}

// Test configuration edge cases
TEST_F(SpdkWALTest, ConfigurationEdgeCases) {
  // Test with null JSON
  nlohmann::json nullConfig = nullptr;
  SpdkWAL wal1(nullConfig);
  EXPECT_FALSE(wal1.append({WalOpType::PUT, "key", "value"}));

  // Test with array JSON (should be treated as invalid)
  nlohmann::json arrayConfig = {"item1", "item2"};
  SpdkWAL wal2(arrayConfig);
  EXPECT_FALSE(wal2.append({WalOpType::PUT, "key", "value"}));
}

// Test that WAL interface is properly implemented
TEST_F(SpdkWALTest, ImplementsWALInterface) {
  nlohmann::json config = "NVMe0n1";
  SpdkWAL wal(config);

  // Test that we can call all virtual methods
  WalEntry entry{WalOpType::PUT, "key", "value"};
  wal.append(entry);

  std::vector<WalEntry> batch = {entry};
  wal.appendBatch(batch);

  wal.sync();
  wal.replay();
  wal.rollSegment();
  wal.truncate(0);
  wal.currentSegmentId();

  // If we get here without compilation errors, the interface is properly
  // implemented
  EXPECT_TRUE(true);
}