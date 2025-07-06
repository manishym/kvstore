#include "storage/memtable_factory.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

class MemTableTest : public ::testing::Test {
protected:
  nlohmann::json config;
};

TEST_F(MemTableTest, StdMapMemTable) {
  config["map_type"] = "std";
  auto mem = createMemTable(config);
  ASSERT_NE(mem, nullptr);
  mem->put("a", "1");
  EXPECT_EQ(mem->get("a"), std::optional<std::string>("1"));
  EXPECT_EQ(mem->size(), 1u);
  mem->del("a");
  EXPECT_EQ(mem->get("a"), std::nullopt);
  mem->markImmutable();
  EXPECT_TRUE(mem->isImmutable());
}

TEST_F(MemTableTest, BoostMapMemTable) {
  config["map_type"] = "boost";
  auto mem = createMemTable(config);
  ASSERT_NE(mem, nullptr);
  mem->put("a", "1");
  EXPECT_EQ(mem->get("a"), std::optional<std::string>("1"));
  EXPECT_EQ(mem->size(), 1u);
  mem->del("a");
  EXPECT_EQ(mem->get("a"), std::nullopt);
  mem->markImmutable();
  EXPECT_TRUE(mem->isImmutable());
}

TEST_F(MemTableTest, SkipListMemTable) {
  config["map_type"] = "skiplist";
  auto mem = createMemTable(config);
  ASSERT_NE(mem, nullptr);
  mem->put("a", "1");
  EXPECT_EQ(mem->get("a"), std::optional<std::string>("1"));
  EXPECT_EQ(mem->size(), 1u);
  mem->del("a");
  EXPECT_EQ(mem->get("a"), std::nullopt);
  mem->markImmutable();
  EXPECT_TRUE(mem->isImmutable());
}

