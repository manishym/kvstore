#include "server_impl.h"
#include "wal_factory.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>

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
  std::unique_ptr<WAL> wal = createWAL(config);

  AsyncKVServer server(address, std::move(wal));
  server.Run();
  return 0;
}
