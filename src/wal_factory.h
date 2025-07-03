#pragma once
#include "block_device_wal.h"
#include "passthrough_wal.h"
#include "spdk_wal.h"
#include <memory>
#include <nlohmann/json.hpp>

inline std::unique_ptr<WAL> createWAL(const nlohmann::json &config) {
  if (!config.contains("wal")) {
    return std::make_unique<PassThroughWAL>();
  }
  auto walConfig = config["wal"];
  std::string type = walConfig.value("type", "none");
  std::string device = walConfig.value("device", "kvstore.wal");
  if (type == "block") {
    return std::make_unique<BlockDeviceWAL>(device);
  } else if (type == "spdk") {
    return std::make_unique<SpdkWAL>(device);
  }
  return std::make_unique<PassThroughWAL>();
}
