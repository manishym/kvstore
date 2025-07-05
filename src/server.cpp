#include "server_impl.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  AsyncKVServer server(server_address);

  // When the environment variable `KVSTORE_NO_RUN` is set the server is
  // constructed but not started.  This is used by the unit tests to execute
  // this function without blocking forever.
  if (std::getenv("KVSTORE_NO_RUN") != nullptr) {
    return;
  }

  server.Run();
}

// Renamed from main to RunServerMain and removed argc/argv parameters
void RunServerMain() { RunServer(); }