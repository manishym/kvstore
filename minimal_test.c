#include "spdk/event.h"
#include "spdk/log.h"
#include <stdio.h>

static void test_start(void *arg) {
  printf("SPDK successfully started!\n");
  spdk_app_stop(0);
}

int main(int argc, char **argv) {
  struct spdk_app_opts opts = {};
  int rc = 0;

  /* Set default values in opts structure. */
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "minimal_test";
  opts.rpc_addr = NULL;

  /*
   * Parse built-in SPDK command line parameters
   */
  if ((rc = spdk_app_parse_args(argc, argv, &opts, "", NULL, NULL, NULL)) !=
      SPDK_APP_PARSE_ARGS_SUCCESS) {
    return rc;
  }

  rc = spdk_app_start(&opts, test_start, NULL);
  if (rc) {
    printf("SPDK app failed: %d\n", rc);
  }

  spdk_app_fini();
  return rc;
}