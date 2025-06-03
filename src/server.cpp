#include "kvstore.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
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

class KeyValueStoreServiceImpl final : public KeyValueStore::Service {
private:
  std::map<std::string, std::string> store_;
  std::mutex mutex_;

public:
  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *response) override {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      store_[request->key()] = request->value();
      response->set_success(true);
      return Status::OK;
    } catch (const std::exception &e) {
      response->set_success(false);
      response->set_error(e.what());
      return Status(grpc::StatusCode::INTERNAL, "Failed to put key-value pair");
    }
  }

  Status Get(ServerContext *context, const GetRequest *request,
             GetResponse *response) override {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      auto it = store_.find(request->key());
      if (it != store_.end()) {
        response->set_value(it->second);
        response->set_found(true);
      } else {
        response->set_found(false);
      }
      return Status::OK;
    } catch (const std::exception &e) {
      response->set_found(false);
      response->set_error(e.what());
      return Status(grpc::StatusCode::INTERNAL, "Failed to get value");
    }
  }

  Status Delete(ServerContext *context, const DeleteRequest *request,
                DeleteResponse *response) override {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      size_t erased = store_.erase(request->key());
      response->set_success(erased > 0);
      return Status::OK;
    } catch (const std::exception &e) {
      response->set_success(false);
      response->set_error(e.what());
      return Status(grpc::StatusCode::INTERNAL, "Failed to delete key");
    }
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  KeyValueStoreServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  RunServer();
  return 0;
}