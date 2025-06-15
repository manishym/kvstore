#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include <folly/concurrency/ConcurrentSkipList.h>
#include <grpcpp/grpcpp.h>
#include <kvstore.grpc.pb.h>
#include <kvstore.pb.h>
#include <string>

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
  // Define the skip list type with string key and value
  using SkipList = folly::ConcurrentSkipList<std::string>;
  using SkipListAccessor = SkipList::Accessor;

  // Create the skip list with default parameters
  SkipList store_{SkipList::createInstance()};

public:
  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *response) override {
    try {
      SkipListAccessor accessor(store_);
      accessor.addOrUpdate(request->key(), request->value());
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
    try {
      SkipListAccessor accessor(store_);
      auto value = accessor.find(request->key());
      if (value) {
        response->set_value(*value);
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
    try {
      SkipListAccessor accessor(store_);
      bool erased = accessor.erase(request->key());
      response->set_success(erased);
      return Status::OK;
    } catch (const std::exception &e) {
      response->set_success(false);
      response->set_error(e.what());
      return Status(grpc::StatusCode::INTERNAL, "Failed to delete key");
    }
  }
};

#endif // SERVER_IMPL_H