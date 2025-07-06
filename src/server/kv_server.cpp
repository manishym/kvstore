#include "server/kv_server.h"
#include "storage/memtable_factory.h"
#include "wal/wal_factory.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  nlohmann::json cfg = nlohmann::json::object();
  auto memtable = createMemTable(cfg);
  auto wal = createWAL(cfg);
  AsyncKVServer server(server_address, memtable, std::move(wal));
  server.Run();
}

// Renamed from main to RunServerMain and removed argc/argv parameters
void RunServerMain() { RunServer(); }