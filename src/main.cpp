#include "server/kv_server.h"
#include "wal/wal_factory.h"
#include "storage/memtable_factory.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>

#include "spdk/event.h"
#include "spdk/log.h"

// Holds CLI-parsed config path
static std::string g_config_path;

struct AppContext {
  std::string config_path;
};

static void start_server(void* arg) {
  auto* ctx = static_cast<AppContext*>(arg);

  try {
    // Load JSON config
    nlohmann::json config;
    std::ifstream in(ctx->config_path);
    if (in) {
      in >> config;
    } else {
      std::cerr << "Failed to open config file: " << ctx->config_path << ", using empty config\n";
      config = nlohmann::json::object();
    }

    std::string address = config.value("address", "0.0.0.0:50051");

    // Construct WAL + Memtable
    std::unique_ptr<WAL> wal = createWAL(config);
    auto memtable = createMemTable(config);

    // Run the gRPC server
    AsyncKVServer server(address, memtable, std::move(wal));
    server.Run();

  } catch (const std::exception& e) {
    std::cerr << "Exception in start_server(): " << e.what() << "\n";
    spdk_app_stop(-1);
  }
}

int main(int argc, char** argv) {
  CLI::App app{"SPDK KVStore Server"};
  app.add_option("-c,--config", g_config_path, "Path to SPDK JSON config file")->required();
  CLI11_PARSE(app, argc, argv);

  struct spdk_app_opts opts;
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "kvstore_server";
  opts.json_config_file = g_config_path.c_str();  // SPDK will parse and execute JSON RPCs here
  opts.mem_size = 1024;
  

  AppContext ctx{g_config_path};

  int rc = spdk_app_start(&opts, start_server, &ctx);
  if (rc) {
    std::cerr << "SPDK app failed: " << rc << "\n";
  }

  spdk_app_fini();
  return rc;
}
