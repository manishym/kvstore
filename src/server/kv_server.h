#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include "storage/memtable.h"
#include "wal/interface.h"
#include <atomic>
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

class AsyncKVServer {
public:
  using KeyValue = std::pair<std::string, std::string>;
  using MemTablePtr = std::shared_ptr<MemTable>;

  AsyncKVServer(const std::string &address, MemTablePtr memtable,
                std::unique_ptr<WAL> wal = nullptr)
      : address_(address), store_(std::move(memtable)), wal_(std::move(wal)) {
    if (wal_) {
      auto entries = wal_->replay();
      for (const auto &e : entries) {
        if (e.op_type == WalOpType::PUT) {
          store_->put(e.key, e.value);
        } else if (e.op_type == WalOpType::DELETE) {
          store_->del(e.key);
        }
      }
    }
  }

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
                MemTablePtr &store, WAL *wal)
        : service_(service), cq_(cq), responder_(&ctx_), store_(store),
          wal_(wal), status_(CREATE) {
      Proceed(true);
    }

    void Proceed(bool ok) override {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestPut(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS) {
        // Spawn next handler
        new PutCallData(service_, cq_, store_, wal_);
        // Process request
        {
          store_->put(request_.key(), request_.value());
        }
        if (wal_) {
          wal_->append({WalOpType::PUT, request_.key(), request_.value()});
          wal_->sync(); // Ensure data is persisted to disk
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
    MemTablePtr &store_;
    WAL *wal_;
  };

  // GET handler
  class GetCallData : public CallDataBase {
  public:
    GetCallData(KeyValueStore::AsyncService *service, ServerCompletionQueue *cq,
                MemTablePtr &store)
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
          auto result = store_->get(request_.key());
          if (result) {
            response_.set_value(*result);
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
    MemTablePtr &store_;
  };

  // DELETE handler
  class DeleteCallData : public CallDataBase {
  public:
    DeleteCallData(KeyValueStore::AsyncService *service,
                   ServerCompletionQueue *cq, MemTablePtr &store, WAL *wal)
        : service_(service), cq_(cq), responder_(&ctx_), store_(store),
          wal_(wal), status_(CREATE) {
      Proceed(true);
    }

    void Proceed(bool ok) override {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestDelete(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS) {
        new DeleteCallData(service_, cq_, store_, wal_);
        {
          auto result = store_->get(request_.key());
          if (result) {
            store_->del(request_.key());
            if (wal_) {
              wal_->append({WalOpType::DELETE, request_.key(), ""});
              wal_->sync(); // Ensure data is persisted to disk
            }
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
    MemTablePtr &store_;
    WAL *wal_;
  };

  void HandleRpcs(ServerCompletionQueue *cq) {
    // One of each to start
    new PutCallData(&service_, cq, store_, wal_.get());
    new GetCallData(&service_, cq, store_);
    new DeleteCallData(&service_, cq, store_, wal_.get());
    void *tag;
    bool ok;
    while (cq->Next(&tag, &ok)) {
      static_cast<CallDataBase *>(tag)->Proceed(ok);
    }
  }

  // Members
  std::string address_;
  MemTablePtr store_;
  KeyValueStore::AsyncService service_;
  std::vector<std::unique_ptr<ServerCompletionQueue>> cqs_;
  std::vector<std::thread> threads_;
  std::unique_ptr<Server> server_;
  std::unique_ptr<WAL> wal_;
};

#endif // SERVER_IMPL_H
