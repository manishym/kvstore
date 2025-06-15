#include "server_impl.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  AsyncKVServer server(server_address);
  server.Run();
}

// Renamed from main to RunServerMain and removed argc/argv parameters
void RunServerMain() { RunServer(); }