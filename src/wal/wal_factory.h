#pragma once
#include "wal/block_device_wal.h"
#include "wal/interface.h"
#include "wal/passthrough_wal.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

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
  }

  // Fallback to block device WAL for unknown or unspecified types
  return std::make_unique<BlockDeviceWAL>("kvstore_block.wal");
}
