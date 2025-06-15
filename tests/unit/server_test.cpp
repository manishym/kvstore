#include "server_impl.h"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <kvstore.grpc.pb.h>
#include <memory>
#include <string>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using kvstore::DeleteRequest;
using kvstore::DeleteResponse;
using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KeyValueStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

// --- Async Server test fixture ---
class KeyValueStoreTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    // Start server
    server_thread_ = std::thread([]() {
      async_server_ = std::make_unique<AsyncKVServer>("0.0.0.0:50051");
      async_server_->Run(4, 2); // 4 CQs, 2 threads each (adjust for your CPU)
    });
    // Wait for server to start (could add a health check here)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create client
    auto channel = grpc::CreateChannel("localhost:50051",
                                       grpc::InsecureChannelCredentials());
    stub_ = KeyValueStore::NewStub(channel);
  }

  static void TearDownTestSuite() {
    // In production you'd signal the server to stop and join thread.
    // For test runs, just detach/kill the server (or modify AsyncKVServer to
    // support shutdown). Here we detach to avoid blocking on server_->Run().
    // server_thread_.detach();
    // Or use std::terminate if you want to forcibly exit after tests.
  }

  static std::unique_ptr<KeyValueStore::Stub> stub_;
  static std::unique_ptr<AsyncKVServer> async_server_;
  static std::thread server_thread_;
};

std::unique_ptr<KeyValueStore::Stub> KeyValueStoreTest::stub_;
std::unique_ptr<AsyncKVServer> KeyValueStoreTest::async_server_;
std::thread KeyValueStoreTest::server_thread_;

// ---- Tests ----

// ... (all your tests from before: PutAndGet, GetNonExistentKey, Delete, etc.)
// ...

// Example for brevity:
TEST_F(KeyValueStoreTest, PutAndGet) {
  std::string key = "test_key";
  std::string value = "test_value";

  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  GetRequest get_request;
  get_request.set_key(key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), value);
}

// (Paste the rest of your test cases as before...)

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();
  // Optionally terminate to kill async server after tests complete
  std::exit(rc);
}
