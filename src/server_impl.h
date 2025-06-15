#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include <folly/ConcurrentSkipList.h>
#include <grpcpp/grpcpp.h>
#include <kvstore.grpc.pb.h>
#include <kvstore.pb.h>
#include <memory>
#include <string>
#include <utility>

using grpc::ServerContext;
using grpc::Status;
using kvstore::DeleteRequest;
using kvstore::DeleteResponse;
using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KeyValueStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

// Custom comparator for KeyValue pairs that only compares keys
struct KeyValueComparator {
  bool operator()(const std::pair<std::string, std::string> &a,
                  const std::pair<std::string, std::string> &b) const {
    return a.first < b.first;
  }
};

class KeyValueStoreServiceImpl final : public KeyValueStore::Service {
private:
  using KeyValue = std::pair<std::string, std::string>;
  using SkipList = folly::ConcurrentSkipList<KeyValue, KeyValueComparator>;
  using SkipListPtr = std::shared_ptr<SkipList>;
  using Accessor = SkipList::Accessor;

  SkipListPtr store_ = SkipList::createInstance();

public:
  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *response) override {
    KeyValue kv{request->key(), request->value()};
    {
      Accessor accessor(store_);
      auto it = accessor.find(kv);
      if (it != accessor.end()) {
        accessor.erase(kv);
      }
      accessor.insert(kv);
    }
    response->set_success(true);
    return Status::OK;
  }

  Status Get(ServerContext *context, const GetRequest *request,
             GetResponse *response) override {
    KeyValue search_key{request->key(), ""};
    Accessor accessor(store_);
    auto it = accessor.find(search_key);
    if (it != accessor.end()) {
      response->set_value(it->second);
      response->set_found(true);
    } else {
      response->set_found(false);
    }
    return Status::OK;
  }

  Status Delete(ServerContext *context, const DeleteRequest *request,
                DeleteResponse *response) override {
    KeyValue search_key{request->key(), ""};
    Accessor accessor(store_);
    auto it = accessor.find(search_key);
    if (it != accessor.end()) {
      accessor.erase(search_key);
      response->set_success(true);
    } else {
      response->set_success(false);
    }
    return Status::OK;
  }
};

#endif // SERVER_IMPL_H