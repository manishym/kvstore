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

    throw std::runtime_error("Unknown map type: " + map_type);
  }
};

} // namespace kvstore