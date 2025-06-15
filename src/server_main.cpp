#include "server_impl.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kvstore::DeleteRequest;
using kvstore::DeleteResponse;
using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KeyValueStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

void RunServer(const std::string &server_address) {
  KeyValueStoreServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

void PrintUsage(const char *program) {
  std::cout << "Usage: " << program << " [options]\n"
            << "Options:\n"
            << "  -h, --help     Show this help message\n"
            << "  -a, --address  Server address (default: 0.0.0.0:50051)\n"
            << "  -p, --port     Server port (default: 50051)\n";
}

int main(int argc, char **argv) {
  std::string server_address = "0.0.0.0:50051";
  int port = 50051;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    } else if (arg == "-a" || arg == "--address") {
      if (i + 1 < argc) {
        server_address = argv[++i];
      } else {
        std::cerr << "Error: --address requires a value\n";
        PrintUsage(argv[0]);
        return 1;
      }
    } else if (arg == "-p" || arg == "--port") {
      if (i + 1 < argc) {
        try {
          port = std::stoi(argv[++i]);
          server_address = "0.0.0.0:" + std::to_string(port);
        } catch (const std::exception &e) {
          std::cerr << "Error: Invalid port number\n";
          PrintUsage(argv[0]);
          return 1;
        }
      } else {
        std::cerr << "Error: --port requires a value\n";
        PrintUsage(argv[0]);
        return 1;
      }
    } else {
      std::cerr << "Error: Unknown option " << arg << "\n";
      PrintUsage(argv[0]);
      return 1;
    }
  }

  RunServer(server_address);
  return 0;
}