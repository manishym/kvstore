#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/stdinc.h"

static void minimal_start(void *arg1) {
  SPDK_NOTICELOG("Successfully started the minimal SPDK application\n");
  spdk_app_stop(0);
}

int main(int argc, char **argv) {
  struct spdk_app_opts opts = {};
  int rc = 0;

  /* Set default values in opts structure - exactly like hello_bdev */
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "minimal_test";
  opts.rpc_addr = NULL;

  /* Parse SPDK command line parameters - exactly like hello_bdev */
  if ((rc = spdk_app_parse_args(argc, argv, &opts, "", NULL, NULL, NULL)) !=
      SPDK_APP_PARSE_ARGS_SUCCESS) {
    return rc;
  }

  /* Start SPDK - exactly like hello_bdev */
  rc = spdk_app_start(&opts, minimal_start, NULL);
  if (rc) {
    SPDK_ERRLOG("ERROR starting application\n");
  }

  /* Gracefully close out all of the SPDK subsystems */
  spdk_app_fini();
  return rc;
}