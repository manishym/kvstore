#pragma once
#include "block_device_wal.h"
#include "passthrough_wal.h"
#include "spdk_wal_simple.h"
#include <memory>
#include <nlohmann/json.hpp>

inline std::unique_ptr<WAL> createWAL(const nlohmann::json &config) {
  const nlohmann::json &walConfig =
      (config.contains("wal") && config["wal"].is_object())
          ? config["wal"]
          : nlohmann::json::object();
  std::string type = walConfig.value("type", "block");

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
      return std::make_unique<SpdkWALSimple>(walConfig["device"]);
    } else {
      // Default SPDK configuration
      nlohmann::json defaultSpdkConfig = {{"spdk_bdev", "NVMe0n1"},
                                          {"wal_segment_size", 67108864},
                                          {"batch_size", 32}};
      return std::make_unique<SpdkWALSimple>(defaultSpdkConfig);
    }
  } else if (type == "passthrough") {
    return std::make_unique<PassThroughWAL>();
  }
  // Fallback to block device WAL for unknown or unspecified types
  return std::make_unique<BlockDeviceWAL>("kvstore_block.wal");
}
