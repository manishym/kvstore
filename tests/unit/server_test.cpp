#include "server_impl.h"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <kvstore.grpc.pb.h>
#include <kvstore.pb.h>
#include <memory>
#include <string>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
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

class KeyValueStoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Start the server in a separate thread
    server_thread_ = std::thread([this]() {
      std::string server_address("0.0.0.0:50051");
      KeyValueStoreServiceImpl service;

      ServerBuilder builder;
      builder.AddListeningPort(server_address,
                               grpc::InsecureServerCredentials());
      builder.RegisterService(&service);
      server_ = builder.BuildAndStart();
      server_->Wait();
    });

    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create the client
    std::shared_ptr<Channel> channel = grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials());
    stub_ = KeyValueStore::NewStub(channel);
  }

  void TearDown() override {
    if (server_) {
      server_->Shutdown();
    }
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
  }

  std::unique_ptr<KeyValueStore::Stub> stub_;
  std::unique_ptr<Server> server_;
  std::thread server_thread_;
};

TEST_F(KeyValueStoreTest, PutAndGet) {
  // Test data
  std::string key = "test_key";
  std::string value = "test_value";

  // Put request
  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  // Get request
  GetRequest get_request;
  get_request.set_key(key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), value);
}

TEST_F(KeyValueStoreTest, GetNonExistentKey) {
  GetRequest request;
  request.set_key("non_existent_key");

  GetResponse response;
  ClientContext context;
  Status status = stub_->Get(&context, request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_FALSE(response.found());
}

TEST_F(KeyValueStoreTest, Delete) {
  // First put a key-value pair
  std::string key = "delete_test_key";
  std::string value = "delete_test_value";

  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  // Delete the key
  DeleteRequest delete_request;
  delete_request.set_key(key);

  DeleteResponse delete_response;
  ClientContext delete_context;
  Status delete_status =
      stub_->Delete(&delete_context, delete_request, &delete_response);

  ASSERT_TRUE(delete_status.ok());
  ASSERT_TRUE(delete_response.success());

  // Verify the key is deleted
  GetRequest get_request;
  get_request.set_key(key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_FALSE(get_response.found());
}

TEST_F(KeyValueStoreTest, DeleteNonExistentKey) {
  DeleteRequest request;
  request.set_key("non_existent_key");

  DeleteResponse response;
  ClientContext context;
  Status status = stub_->Delete(&context, request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_FALSE(response.success());
}

TEST_F(KeyValueStoreTest, ConcurrentOperations) {
  const int num_operations = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_operations; ++i) {
    threads.emplace_back([this, i]() {
      std::string key = "concurrent_key_" + std::to_string(i);
      std::string value = "concurrent_value_" + std::to_string(i);

      // Put
      PutRequest put_request;
      put_request.set_key(key);
      put_request.set_value(value);

      PutResponse put_response;
      ClientContext put_context;
      Status put_status = stub_->Put(&put_context, put_request, &put_response);

      ASSERT_TRUE(put_status.ok());
      ASSERT_TRUE(put_response.success());

      // Get
      GetRequest get_request;
      get_request.set_key(key);

      GetResponse get_response;
      ClientContext get_context;
      Status get_status = stub_->Get(&get_context, get_request, &get_response);

      ASSERT_TRUE(get_status.ok());
      ASSERT_TRUE(get_response.found());
      ASSERT_EQ(get_response.value(), value);

      // Delete
      DeleteRequest delete_request;
      delete_request.set_key(key);

      DeleteResponse delete_response;
      ClientContext delete_context;
      Status delete_status =
          stub_->Delete(&delete_context, delete_request, &delete_response);

      ASSERT_TRUE(delete_status.ok());
      ASSERT_TRUE(delete_response.success());
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }
}

TEST_F(KeyValueStoreTest, EmptyKeyValue) {
  // Test empty key
  PutRequest empty_key_request;
  empty_key_request.set_key("");
  empty_key_request.set_value("value");

  PutResponse empty_key_response;
  ClientContext empty_key_context;
  Status empty_key_status =
      stub_->Put(&empty_key_context, empty_key_request, &empty_key_response);

  ASSERT_TRUE(empty_key_status.ok());
  ASSERT_TRUE(empty_key_response.success());

  // Test empty value
  PutRequest empty_value_request;
  empty_value_request.set_key("key");
  empty_value_request.set_value("");

  PutResponse empty_value_response;
  ClientContext empty_value_context;
  Status empty_value_status = stub_->Put(
      &empty_value_context, empty_value_request, &empty_value_response);

  ASSERT_TRUE(empty_value_status.ok());
  ASSERT_TRUE(empty_value_response.success());

  // Verify empty key-value pair
  GetRequest get_request;
  get_request.set_key("");

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), "value");
}

TEST_F(KeyValueStoreTest, LargeKeyValue) {
  // Test with large key and value
  std::string large_key(1000, 'k');
  std::string large_value(10000, 'v');

  PutRequest put_request;
  put_request.set_key(large_key);
  put_request.set_value(large_value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  // Verify large key-value pair
  GetRequest get_request;
  get_request.set_key(large_key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), large_value);
}

TEST_F(KeyValueStoreTest, SpecialCharacters) {
  // Test with special characters
  std::string special_key = "!@#$%^&*()_+{}|:\"<>?";
  std::string special_value = "\\n\\t\\r\\0";

  PutRequest put_request;
  put_request.set_key(special_key);
  put_request.set_value(special_value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  // Verify special characters
  GetRequest get_request;
  get_request.set_key(special_key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), special_value);
}

TEST_F(KeyValueStoreTest, UpdateExistingKey) {
  std::string key = "update_key";
  std::string value1 = "value1";
  std::string value2 = "value2";

  // First put
  PutRequest put_request1;
  put_request1.set_key(key);
  put_request1.set_value(value1);

  PutResponse put_response1;
  ClientContext put_context1;
  Status put_status1 = stub_->Put(&put_context1, put_request1, &put_response1);

  ASSERT_TRUE(put_status1.ok());
  ASSERT_TRUE(put_response1.success());

  // Update with new value
  PutRequest put_request2;
  put_request2.set_key(key);
  put_request2.set_value(value2);

  PutResponse put_response2;
  ClientContext put_context2;
  Status put_status2 = stub_->Put(&put_context2, put_request2, &put_response2);

  ASSERT_TRUE(put_status2.ok());
  ASSERT_TRUE(put_response2.success());

  // Verify updated value
  GetRequest get_request;
  get_request.set_key(key);

  GetResponse get_response;
  ClientContext get_context;
  Status get_status = stub_->Get(&get_context, get_request, &get_response);

  ASSERT_TRUE(get_status.ok());
  ASSERT_TRUE(get_response.found());
  ASSERT_EQ(get_response.value(), value2);
}

TEST_F(KeyValueStoreTest, MultipleOperationsSameKey) {
  std::string key = "multi_op_key";
  std::string value = "value";

  // Put
  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);

  PutResponse put_response;
  ClientContext put_context;
  Status put_status = stub_->Put(&put_context, put_request, &put_response);

  ASSERT_TRUE(put_status.ok());
  ASSERT_TRUE(put_response.success());

  // Get multiple times
  for (int i = 0; i < 5; ++i) {
    GetRequest get_request;
    get_request.set_key(key);

    GetResponse get_response;
    ClientContext get_context;
    Status get_status = stub_->Get(&get_context, get_request, &get_response);

    ASSERT_TRUE(get_status.ok());
    ASSERT_TRUE(get_response.found());
    ASSERT_EQ(get_response.value(), value);
  }

  // Delete
  DeleteRequest delete_request;
  delete_request.set_key(key);

  DeleteResponse delete_response;
  ClientContext delete_context;
  Status delete_status =
      stub_->Delete(&delete_context, delete_request, &delete_response);

  ASSERT_TRUE(delete_status.ok());
  ASSERT_TRUE(delete_response.success());

  // Try to delete again
  DeleteRequest delete_request2;
  delete_request2.set_key(key);

  DeleteResponse delete_response2;
  ClientContext delete_context2;
  Status delete_status2 =
      stub_->Delete(&delete_context2, delete_request2, &delete_response2);

  ASSERT_TRUE(delete_status2.ok());
  ASSERT_FALSE(delete_response2.success());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}