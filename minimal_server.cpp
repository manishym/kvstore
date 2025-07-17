#include <iostream>
#include <string>

#include "spdk/event.h"
#include "spdk/log.h"

static void minimal_server_start(void *arg) {
  std::cout << "Minimal SPDK server started successfully!" << std::endl;
  SPDK_NOTICELOG("Minimal server initialized successfully");
  spdk_app_stop(0);
}

int main(int argc, char **argv) {
  struct spdk_app_opts opts = {}; // Zero-initialize like working example
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "minimal_server";
  opts.rpc_addr = nullptr; // Explicitly set like working example

  // Parse SPDK args (minimal version)
  int parse_rc = spdk_app_parse_args(argc, argv, &opts, nullptr, nullptr,
                                     nullptr, nullptr);
  if (parse_rc != SPDK_APP_PARSE_ARGS_SUCCESS) {
    std::cerr << "SPDK parse args failed: " << parse_rc << "\n";
    return parse_rc;
  }

  int rc = spdk_app_start(&opts, minimal_server_start, nullptr);
  if (rc) {
    std::cerr << "SPDK app failed: " << rc << "\n";
  }

  spdk_app_fini();
  return rc;
}