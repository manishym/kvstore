#include "wal/wal_factory.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(WALFactoryTest, NoneCreatesBlockDevice) {
  nlohmann::json cfg;
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<BlockDeviceWAL *>(wal.get()) != nullptr);
}

TEST(WALFactoryTest, BlockCreatesBlockDevice) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "block"}, {"device", "test.log"}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<BlockDeviceWAL *>(wal.get()) != nullptr);
}

TEST(WALFactoryTest, SpdkCreatesSpdkWAL) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "spdk"}, {"device", "NVMe0n1"}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<SpdkWAL *>(wal.get()) != nullptr);
}

TEST(WALFactoryTest, SpdkWithComplexConfig) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "spdk"},
                {"device",
                 {{"spdk_bdev", "NVMe1n1"},
                  {"wal_segment_size", 134217728},
                  {"batch_size", 64}}}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<SpdkWAL *>(wal.get()) != nullptr);
}

TEST(WALFactoryTest, SpdkWithDefaultConfig) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "spdk"}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<SpdkWAL *>(wal.get()) != nullptr);
}

TEST(WALFactoryTest, InvalidTypeCreatesBlockDevice) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "invalid_type"}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<BlockDeviceWAL *>(wal.get()) != nullptr);
}
