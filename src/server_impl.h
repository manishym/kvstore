#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include <atomic>
#include <folly/ConcurrentSkipList.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <kvstore.grpc.pb.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using kvstore::DeleteRequest;
using kvstore::DeleteResponse;
using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KeyValueStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

struct KeyValueComparator {
  bool operator()(const std::pair<std::string, std::string> &a,
                  const std::pair<std::string, std::string> &b) const {
    return a.first < b.first;
  }
};

class AsyncKVServer {
public:
  using KeyValue = std::pair<std::string, std::string>;
  using SkipList = folly::ConcurrentSkipList<KeyValue, KeyValueComparator>;
  using SkipListPtr = std::shared_ptr<SkipList>;
  using Accessor = SkipList::Accessor;

  AsyncKVServer(const std::string &address)
      : address_(address), store_(SkipList::createInstance()) {}

  void Run(int num_cqs = 4, int threads_per_cq = 2) {
    ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    for (int i = 0; i < num_cqs; ++i)
      cqs_.emplace_back(builder.AddCompletionQueue());

    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << address_ << std::endl;

    for (size_t i = 0; i < cqs_.size(); ++i)
      for (int j = 0; j < threads_per_cq; ++j)
        threads_.emplace_back([this, i]() { HandleRpcs(cqs_[i].get()); });

    for (auto &thread : threads_)
      thread.join();
  }

private:
  // Base for all CallData
  class CallDataBase {
  public:
    virtual ~CallDataBase() = default;
    virtual void Proceed(bool ok) = 0;
  };

  // PUT handler
  class PutCallData : public CallDataBase {
  public:
    PutCallData(KeyValueStore::AsyncService *service, ServerCompletionQueue *cq,
                SkipListPtr &store)
        : service_(service), cq_(cq), responder_(&ctx_), store_(store),
          status_(CREATE) {
      Proceed(true);
    }

    void Proceed(bool ok) override {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestPut(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS) {
        // Spawn next handler
        new PutCallData(service_, cq_, store_);
        // Process request
        {
          AsyncKVServer::Accessor accessor(store_);
          auto kv = std::make_pair(request_.key(), request_.value());
          auto it = accessor.find(kv);
          if (it != accessor.end())
            accessor.erase(kv);
          accessor.insert(kv);
        }
        response_.set_success(true);
        status_ = FINISH;
        responder_.Finish(response_, Status::OK, this);
      } else {
        // FINISH
        delete this;
      }
    }

  private:
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    KeyValueStore::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    PutRequest request_;
    PutResponse response_;
    ServerAsyncResponseWriter<PutResponse> responder_;
    SkipListPtr &store_;
  };

  // GET handler
  class GetCallData : public CallDataBase {
  public:
    GetCallData(KeyValueStore::AsyncService *service, ServerCompletionQueue *cq,
                SkipListPtr &store)
        : service_(service), cq_(cq), responder_(&ctx_), store_(store),
          status_(CREATE) {
      Proceed(true);
    }

    void Proceed(bool ok) override {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestGet(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS) {
        new GetCallData(service_, cq_, store_);
        // Process
        {
          AsyncKVServer::Accessor accessor(store_);
          auto search = std::make_pair(request_.key(), "");
          auto it = accessor.find(search);
          if (it != accessor.end()) {
            response_.set_value(it->second);
            response_.set_found(true);
          } else {
            response_.set_found(false);
          }
        }
        status_ = FINISH;
        responder_.Finish(response_, Status::OK, this);
      } else {
        delete this;
      }
    }

  private:
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    KeyValueStore::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    GetRequest request_;
    GetResponse response_;
    ServerAsyncResponseWriter<GetResponse> responder_;
    SkipListPtr &store_;
  };

  // DELETE handler
  class DeleteCallData : public CallDataBase {
  public:
    DeleteCallData(KeyValueStore::AsyncService *service,
                   ServerCompletionQueue *cq, SkipListPtr &store)
        : service_(service), cq_(cq), responder_(&ctx_), store_(store),
          status_(CREATE) {
      Proceed(true);
    }

    void Proceed(bool ok) override {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestDelete(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS) {
        new DeleteCallData(service_, cq_, store_);
        {
          AsyncKVServer::Accessor accessor(store_);
          auto search = std::make_pair(request_.key(), "");
          auto it = accessor.find(search);
          if (it != accessor.end()) {
            accessor.erase(search);
            response_.set_success(true);
          } else {
            response_.set_success(false);
          }
        }
        status_ = FINISH;
        responder_.Finish(response_, Status::OK, this);
      } else {
        delete this;
      }
    }

  private:
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
    KeyValueStore::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    DeleteRequest request_;
    DeleteResponse response_;
    ServerAsyncResponseWriter<DeleteResponse> responder_;
    SkipListPtr &store_;
  };

  void HandleRpcs(ServerCompletionQueue *cq) {
    // One of each to start
    new PutCallData(&service_, cq, store_);
    new GetCallData(&service_, cq, store_);
    new DeleteCallData(&service_, cq, store_);
    void *tag;
    bool ok;
    while (cq->Next(&tag, &ok)) {
      static_cast<CallDataBase *>(tag)->Proceed(ok);
    }
  }

  // Members
  std::string address_;
  SkipListPtr store_;
  KeyValueStore::AsyncService service_;
  std::vector<std::unique_ptr<ServerCompletionQueue>> cqs_;
  std::vector<std::thread> threads_;
  std::unique_ptr<Server> server_;
};

#endif // SERVER_IMPL_H
