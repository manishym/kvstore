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

  if (type == "block") {
    if (walConfig.contains("device")) {
      auto device = walConfig["device"];
      if (device.is_string()) {
        return std::make_unique<BlockDeviceWAL>(device.get<std::string>());
      } else if (device.is_object()) {
        return std::make_unique<BlockDeviceWAL>(device);
      } else {
        return std::make_unique<BlockDeviceWAL>("kvstore_block.wal");
      }
    } else {
      return std::make_unique<BlockDeviceWAL>("kvstore_block.wal");
    }
  } else if (type == "spdk") {
    if (walConfig.contains("device")) {
      return std::make_unique<SpdkWAL>(walConfig["device"]);
    } else {
      // Default SPDK configuration
      nlohmann::json defaultSpdkConfig = {{"spdk_bdev", "NVMe0n1"},
                                          {"wal_segment_size", 67108864},
                                          {"batch_size", 32}};
      return std::make_unique<SpdkWAL>(defaultSpdkConfig);
    }
  }
  return std::make_unique<PassThroughWAL>();
}
