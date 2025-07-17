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

TEST(WALFactoryTest, InvalidTypeCreatesBlockDevice) {
  nlohmann::json cfg;
  cfg["wal"] = {{"type", "invalid_type"}};
  auto wal = createWAL(cfg);
  EXPECT_TRUE(dynamic_cast<BlockDeviceWAL *>(wal.get()) != nullptr);
}
