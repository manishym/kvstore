#include "server_impl.h"
#include "wal_factory.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <CLI/CLI.hpp>

int main(int argc, char **argv) {
  CLI::App app{"My SPDK Key-Value Store"};
  std::string config_path;
  app.add_option("-c,--config", config_path, "Path to config file")
      ->required();
  CLI11_PARSE(app, argc, argv);

  nlohmann::json config;
  std::ifstream in(config_path);
  if (in) {
    in >> config;
  } else {
    std::cerr << "Failed to open config " << config_path << ", using defaults\n";
    config = nlohmann::json::object();
  }

  std::string address = config.value("address", "0.0.0.0:50051");
  std::unique_ptr<WAL> wal = createWAL(config);

  AsyncKVServer server(address, std::move(wal));
  server.Run();
  return 0;
}
