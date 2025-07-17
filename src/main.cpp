#include "server/kv_server.h"
#include "storage/memtable_factory.h"
#include "wal/wal_factory.h"

#include <CLI/CLI.hpp>
#include <atomic>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

#include "spdk/event.h"
#include "spdk/log.h"

// Holds CLI-parsed config path
static std::string g_config_path;

struct AppContext {
  std::string config_path;
};

static void start_server(void *arg) {
  auto *ctx = static_cast<AppContext *>(arg);

  try {
    SPDK_NOTICELOG(
        "SPDK successfully initialized, now loading heavy C++ dependencies");

    // NOW load JSON config and heavy C++ libraries after SPDK is initialized
    nlohmann::json config;
    std::ifstream in(ctx->config_path);
    if (in) {
      in >> config;
    } else {
      std::cerr << "Failed to open config file: " << ctx->config_path
                << ", using empty config\n";
      config = nlohmann::json::object();
    }

    std::string address = config.value("address", "0.0.0.0:50051");

    // Construct WAL + Memtable
    std::unique_ptr<WAL> wal = createWAL(config);
    auto memtable = createMemTable(config);

    // Run the gRPC server
    AsyncKVServer server(address, memtable, std::move(wal));
    server.Run();

  } catch (const std::exception &e) {
    std::cerr << "Exception in start_server(): " << e.what() << "\n";
    spdk_app_stop(-1);
  }
}

int main(int argc, char **argv) {
  CLI::App app{"SPDK KVStore Server"};
  app.add_option("-c,--config", g_config_path, "Path to SPDK JSON config file")
      ->required();
  CLI11_PARSE(app, argc, argv);

  struct spdk_app_opts opts = {}; // Zero-initialize like working example
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "kvstore_server1234";
  opts.rpc_addr = NULL; // Explicitly set like working example

  // Try explicit settings to work around heavy C++ dependency conflicts
  opts.mem_size = 512; // Increase memory to accommodate C++ runtime
  opts.main_core = 0;  // Explicitly set main core
  opts.reactor_mask =
      "0x1"; // Explicitly set reactor mask like working spdk_tgt

  // Only set config file if we actually need SPDK to parse it
  // Comment this out initially to match working example behavior
  // opts.json_config_file = g_config_path.c_str();

  // Try calling spdk_app_parse_args even though we handle config ourselves
  // This might initialize internal SPDK state that's required
  int parse_rc = spdk_app_parse_args(argc, argv, &opts, NULL, NULL, NULL, NULL);
  if (parse_rc != SPDK_APP_PARSE_ARGS_SUCCESS) {
    std::cerr << "SPDK parse args failed: " << parse_rc << "\n";
    return parse_rc;
  }

  AppContext ctx{g_config_path};

  int rc = spdk_app_start(&opts, start_server, &ctx);
  if (rc) {
    std::cerr << "SPDK app failed: " << rc << "\n";
  }

  spdk_app_fini();
  return rc;
}
