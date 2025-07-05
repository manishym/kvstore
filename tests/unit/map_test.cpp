#include "map/BoostMap.h"
#include "map/IMap.h"
#include "map/MapFactory.h"
#include "map/StdMap.h"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace std;
using namespace kvstore;

class MapTest : public ::testing::Test {
protected:
  void SetUp() override {
    // The original tests attempted to load configuration from an external
    // file.  In this environment the file is not present which caused the
    // tests to fail during setup.  Only a few configuration values are
    // required by the MapFactory, so we construct a minimal configuration
    // object directly.
    config = {
        {"map_options",
         {
             {"boost_map", {{"initial_size", 16}, {"load_factor", 0.75}}},
             {"std_map", {{"initial_size", 16}}}
         }}
    };
  }

  nlohmann::json config;
};

TEST_F(MapTest, BoostMapTest) {
  config["map_type"] = "boost_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  // Test basic operations
  map->insert("key1", "value1");
  std::string value;
  EXPECT_TRUE(map->get("key1", value));
  EXPECT_EQ(value, "value1");
  EXPECT_FALSE(map->contains("key2"));
  map->remove("key1");
  EXPECT_FALSE(map->contains("key1"));
}

TEST_F(MapTest, StdMapTest) {
  config["map_type"] = "std_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  // Test basic operations
  map->insert("key1", "value1");
  std::string value;
  EXPECT_TRUE(map->get("key1", value));
  EXPECT_EQ(value, "value1");
  EXPECT_FALSE(map->contains("key2"));
  map->remove("key1");
  EXPECT_FALSE(map->contains("key1"));
}

TEST_F(MapTest, BoostMapOrderedTest) {
  config["map_type"] = "boost_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  // Test ordered operations
  map->insert("key1", "value1");
  map->insert("key2", "value2");
  map->insert("key3", "value3");

  auto it = map->begin();
  EXPECT_EQ(it->first, "key1");
  EXPECT_EQ(it->second, "value1");

  auto lower = map->lower_bound("key2");
  EXPECT_EQ(lower->first, "key2");
  EXPECT_EQ(lower->second, "value2");

  auto upper = map->upper_bound("key2");
  EXPECT_EQ(upper->first, "key3");
  EXPECT_EQ(upper->second, "value3");
}

TEST_F(MapTest, MapFactoryTest) {
  config["map_type"] = "boost_map";
  auto boostMap = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(boostMap, nullptr);
  EXPECT_TRUE((dynamic_cast<BoostMap<std::string, std::string> *>(
                   boostMap.get()) != nullptr));

  config["map_type"] = "std_map";
  auto stdMap = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(stdMap, nullptr);
  EXPECT_TRUE((dynamic_cast<StdMap<std::string, std::string> *>(stdMap.get()) !=
               nullptr));
}

TEST_F(MapTest, BoostMapOrderedOperationsTest) {
  config["map_type"] = "boost_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  map->insert("key1", "value1");
  map->insert("key2", "value2");
  map->insert("key3", "value3");

  auto it = map->begin();
  EXPECT_EQ(it->first, "key1");
  EXPECT_EQ(it->second, "value1");

  auto lower = map->lower_bound("key2");
  EXPECT_EQ(lower->first, "key2");
  EXPECT_EQ(lower->second, "value2");

  auto upper = map->upper_bound("key2");
  EXPECT_EQ(upper->first, "key3");
  EXPECT_EQ(upper->second, "value3");
}

TEST_F(MapTest, StdMapOrderedOperationsTest) {
  config["map_type"] = "std_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  map->insert("key1", "value1");
  map->insert("key2", "value2");
  map->insert("key3", "value3");

  auto it = map->begin();
  EXPECT_EQ(it->first, "key1");
  EXPECT_EQ(it->second, "value1");

  auto lower = map->lower_bound("key2");
  EXPECT_EQ(lower->first, "key2");
  EXPECT_EQ(lower->second, "value2");

  auto upper = map->upper_bound("key2");
  EXPECT_EQ(upper->first, "key3");
  EXPECT_EQ(upper->second, "value3");
}

TEST_F(MapTest, BoostMapEdgeCases) {
  config["map_type"] = "boost_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  // Test empty key and value
  map->insert("", "");
  std::string value;
  EXPECT_TRUE(map->get("", value));
  EXPECT_EQ(value, "");

  // Test duplicate key insertion
  map->insert("duplicate", "first");
  map->insert("duplicate", "second");
  EXPECT_TRUE(map->get("duplicate", value));
  EXPECT_EQ(value, "second"); // Should overwrite

  // Test non-existent key
  EXPECT_FALSE(map->get("nonexistent", value));
}

TEST_F(MapTest, StdMapEdgeCases) {
  config["map_type"] = "std_map";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  ASSERT_NE(map, nullptr);

  // Test empty key and value
  map->insert("", "");
  std::string value;
  EXPECT_TRUE(map->get("", value));
  EXPECT_EQ(value, "");

  // Test duplicate key insertion
  map->insert("duplicate", "first");
  map->insert("duplicate", "second");
  EXPECT_TRUE(map->get("duplicate", value));
  EXPECT_EQ(value, "second"); // Should overwrite

  // Test non-existent key
  EXPECT_FALSE(map->get("nonexistent", value));
}

TEST_F(MapTest, MapFactoryInvalidType) {
  config["map_type"] = "invalid_type";
  auto map = MapFactory<std::string, std::string>::createMap(config);
  EXPECT_EQ(map, nullptr);
}

TEST_F(MapTest, MapFactoryMissingType) {
  config.erase("map_type");
  auto map = MapFactory<std::string, std::string>::createMap(config);
  EXPECT_EQ(map, nullptr);
}