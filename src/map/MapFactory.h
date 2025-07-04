#pragma once

#include "BoostMap.h"
#include "IMap.h"
#include "StdMap.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace kvstore {

template <typename K, typename V> class MapFactory {
public:
  static std::unique_ptr<IMap<K, V>> createMap(const nlohmann::json &config) {
    // Check if map_type exists in config
    if (!config.contains("map_type")) {
      return nullptr;
    }

    std::string map_type = config["map_type"];

    if (map_type == "boost_map") {
      const auto &options = config["map_options"]["boost_map"];
      return std::make_unique<BoostMap<K, V>>(
          options["initial_size"].get<size_t>(),
          options["load_factor"].get<float>());
    } else if (map_type == "std_map") {
      const auto &options = config["map_options"]["std_map"];
      return std::make_unique<StdMap<K, V>>(
          options["initial_size"].get<size_t>());
    }

    // Return nullptr for unknown map types instead of throwing
    return nullptr;
  }
};

} // namespace kvstore