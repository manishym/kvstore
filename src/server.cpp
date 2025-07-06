#include "server_impl.h"
#include "storage/memtable_factory.h"
#include <nlohmann/json.hpp>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  nlohmann::json cfg = nlohmann::json::object();
  auto memtable = createMemTable(cfg);
  AsyncKVServer server(server_address, memtable);
  server.Run();
}

// Renamed from main to RunServerMain and removed argc/argv parameters
void RunServerMain() { RunServer(); }