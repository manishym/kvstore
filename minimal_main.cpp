#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/stdinc.h"

static void minimal_start(void *arg) {
  SPDK_NOTICELOG("SPDK initialization successful!\n");
  spdk_app_stop(0);
}

int main(int argc, char **argv) {
  struct spdk_app_opts opts;
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "minimal_kvstore";
  opts.mem_size = 256;
  opts.reactor_mask = "0x1";
  opts.print_level = SPDK_LOG_DEBUG;

  int rc = spdk_app_parse_args(argc, argv, &opts, NULL, NULL, NULL, NULL);
  if (rc != SPDK_APP_PARSE_ARGS_SUCCESS) {
    return rc;
  }

  SPDK_NOTICELOG(">>> Before spdk_app_start\n");
  rc = spdk_app_start(&opts, minimal_start, NULL);
  if (rc) {
    SPDK_ERRLOG("SPDK app failed: %d\n", rc);
  }

  spdk_app_fini();
  return rc;
}