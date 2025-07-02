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
    std::ifstream config_file("runtime_config.json");
    if (!config_file.is_open()) {
      std::cerr << "Failed to open runtime_config.json" << std::endl;
      throw std::runtime_error("Could not open config file");
    }

    // Remove debug print of file contents
    // std::string content((std::istreambuf_iterator<char>(config_file)),
    //                     std::istreambuf_iterator<char>());
    // std::cout << "Config file contents:\n" << content << std::endl;

    // Reset file position
    config_file.clear();
    config_file.seekg(0);

    config = nlohmann::json::parse(config_file);
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