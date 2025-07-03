#include "server_impl.h"
#include "block_device_wal.h"
#include "spdk_wal.h"
#include <fstream>
#include <nlohmann/json.hpp>

int main(int argc, char **argv) {
  std::string config_path = argc > 1 ? argv[1] : "src/config/runtime_config.json";
  nlohmann::json config;
  std::ifstream in(config_path);
  if (in) {
    in >> config;
  } else {
    std::cerr << "Failed to open config " << config_path << ", using defaults\n";
  }

  std::string address = config.value("address", "0.0.0.0:50051");
  std::unique_ptr<WAL> wal;
  if (config.contains("wal")) {
    auto w = config["wal"];
    std::string type = w.value("type", "block");
    std::string device = w.value("device", "kvstore.wal");
    if (type == "block")
      wal = std::make_unique<BlockDeviceWAL>(device);
    else if (type == "spdk")
      wal = std::make_unique<SpdkWAL>(device);
  }

  AsyncKVServer server(address, std::move(wal));
  server.Run();
  return 0;
}