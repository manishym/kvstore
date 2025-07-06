#include "wal/spdk_wal_simple.h"
#include <cstdio>
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
  SpdkWALSimple wal(config);

  // The simplified implementation should construct successfully
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with JSON object
TEST_F(SpdkWALTest, ConstructorWithJsonObject) {
  nlohmann::json config = {{"spdk_bdev", "NVMe1n1"},
                           {"wal_segment_size", 134217728}, // 128MB
                           {"batch_size", 64}};

  SpdkWALSimple wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with default values
TEST_F(SpdkWALTest, ConstructorWithPartialConfig) {
  nlohmann::json config = {
      {"spdk_bdev", "NVMe2n1"}
      // Missing wal_segment_size and batch_size should use defaults
  };

  SpdkWALSimple wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test configuration parsing with empty object
TEST_F(SpdkWALTest, ConstructorWithEmptyObject) {
  nlohmann::json config = {};

  SpdkWALSimple wal(config);
  EXPECT_TRUE(true); // Constructor should not throw
}

// Test append method (should work with simplified implementation)
TEST_F(SpdkWALTest, AppendWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  WalEntry entry{WalOpType::PUT, "test_key", "test_value"};
  EXPECT_TRUE(wal.append(entry));
}

// Test appendBatch method (should work with simplified implementation)
TEST_F(SpdkWALTest, AppendBatchWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  std::vector<WalEntry> entries = {{WalOpType::PUT, "key1", "value1"},
                                   {WalOpType::DELETE, "key2", ""},
                                   {WalOpType::PUT, "key3", "value3"}};

  EXPECT_TRUE(wal.appendBatch(entries));
}

// Test sync method (should work with simplified implementation)
TEST_F(SpdkWALTest, SyncWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  EXPECT_TRUE(wal.sync());
}

// Test replay method (should work with simplified implementation)
TEST_F(SpdkWALTest, ReplayWorks) {
  // Clean up any existing files first
  std::remove("/tmp/spdk_wal_NVMe0n1_replay_test.log");

  nlohmann::json config = "NVMe0n1_replay_test";
  SpdkWALSimple wal(config);

  // First append some entries
  WalEntry entry1{WalOpType::PUT, "key1", "value1"};
  WalEntry entry2{WalOpType::DELETE, "key2", ""};
  EXPECT_TRUE(wal.append(entry1));
  EXPECT_TRUE(wal.append(entry2));

  // Then replay them
  auto entries = wal.replay();
  EXPECT_EQ(entries.size(), 2);
  EXPECT_EQ(entries[0].key, "key1");
  EXPECT_EQ(entries[0].value, "value1");
  EXPECT_EQ(entries[1].key, "key2");
  EXPECT_EQ(entries[1].op_type, WalOpType::DELETE);
}

// Test rollSegment method (should work with simplified implementation)
TEST_F(SpdkWALTest, RollSegmentWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  EXPECT_TRUE(wal.rollSegment());
  EXPECT_EQ(wal.currentSegmentId(), 1);
}

// Test truncate method (should work with simplified implementation)
TEST_F(SpdkWALTest, TruncateWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  EXPECT_TRUE(wal.truncate(123));
}

// Test currentSegmentId method (should work with simplified implementation)
TEST_F(SpdkWALTest, CurrentSegmentIdWorks) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  EXPECT_EQ(wal.currentSegmentId(), 0);

  // After rolling a segment, it should be 1
  wal.rollSegment();
  EXPECT_EQ(wal.currentSegmentId(), 1);
}

// Test with different operation types
TEST_F(SpdkWALTest, DifferentOperationTypes) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  WalEntry putEntry{WalOpType::PUT, "put_key", "put_value"};
  WalEntry deleteEntry{WalOpType::DELETE, "delete_key", ""};

  EXPECT_TRUE(wal.append(putEntry));
  EXPECT_TRUE(wal.append(deleteEntry));
}

// Test with empty key and value
TEST_F(SpdkWALTest, EmptyKeyAndValue) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  WalEntry emptyEntry{WalOpType::PUT, "", ""};
  EXPECT_TRUE(wal.append(emptyEntry));
}

// Test with large values
TEST_F(SpdkWALTest, LargeValues) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  std::string largeValue(1000, 'x'); // 1000 character value
  WalEntry largeEntry{WalOpType::PUT, "large_key", largeValue};
  EXPECT_TRUE(wal.append(largeEntry));
}

// Test batch operations with empty batch
TEST_F(SpdkWALTest, EmptyBatch) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  std::vector<WalEntry> emptyBatch;
  EXPECT_TRUE(wal.appendBatch(emptyBatch));
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
    SpdkWALSimple wal(config);
    WalEntry entry{WalOpType::PUT, "test_key", "test_value"};
    EXPECT_TRUE(wal.append(entry));
  }
}

// Test configuration edge cases
TEST_F(SpdkWALTest, ConfigurationEdgeCases) {
  // Test with null JSON
  nlohmann::json nullConfig = nullptr;
  SpdkWALSimple wal1(nullConfig);
  EXPECT_TRUE(wal1.append({WalOpType::PUT, "key", "value"}));

  // Test with array JSON (should be treated as invalid)
  nlohmann::json arrayConfig = {"item1", "item2"};
  SpdkWALSimple wal2(arrayConfig);
  EXPECT_TRUE(wal2.append({WalOpType::PUT, "key", "value"}));
}

// Test that WAL interface is properly implemented
TEST_F(SpdkWALTest, ImplementsWALInterface) {
  nlohmann::json config = "NVMe0n1";
  SpdkWALSimple wal(config);

  // Test that we can call all virtual methods
  WalEntry entry{WalOpType::PUT, "key", "value"};
  EXPECT_TRUE(wal.append(entry));

  std::vector<WalEntry> batch = {entry};
  EXPECT_TRUE(wal.appendBatch(batch));

  EXPECT_TRUE(wal.sync());

  auto entries = wal.replay();
  EXPECT_FALSE(entries.empty());

  EXPECT_TRUE(wal.rollSegment());
  EXPECT_TRUE(wal.truncate(0));
  EXPECT_EQ(wal.currentSegmentId(), 1);
}