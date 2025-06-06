#include <grpcpp/grpcpp.h>
#include <iostream>
#include <kvstore.grpc.pb.h>
#include <kvstore.pb.h>
#include <memory>
#include <string>

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

class KeyValueStoreClient {
public:
  KeyValueStoreClient(std::shared_ptr<Channel> channel)
      : stub_(KeyValueStore::NewStub(channel)) {}

  bool Put(const std::string &key, const std::string &value) {
    PutRequest request;
    request.set_key(key);
    request.set_value(value);

    PutResponse response;
    ClientContext context;

    Status status = stub_->Put(&context, request, &response);
    if (status.ok()) {
      return response.success();
    } else {
      std::cout << "Put failed: " << status.error_code() << ": "
                << status.error_message() << std::endl;
      return false;
    }
  }

  bool Get(const std::string &key, std::string *value) {
    GetRequest request;
    request.set_key(key);

    GetResponse response;
    ClientContext context;

    Status status = stub_->Get(&context, request, &response);
    if (status.ok()) {
      if (response.found()) {
        *value = response.value();
        return true;
      } else {
        std::cout << "Key not found." << std::endl;
        return false;
      }
    } else {
      std::cout << "Get failed: " << status.error_code() << ": "
                << status.error_message() << std::endl;
      return false;
    }
  }

  bool Delete(const std::string &key) {
    DeleteRequest request;
    request.set_key(key);

    DeleteResponse response;
    ClientContext context;

    Status status = stub_->Delete(&context, request, &response);
    if (status.ok()) {
      return response.success();
    } else {
      std::cout << "Delete failed: " << status.error_code() << ": "
                << status.error_message() << std::endl;
      return false;
    }
  }

private:
  std::unique_ptr<KeyValueStore::Stub> stub_;
};

int main(int argc, char **argv) {
  std::string target_address("localhost:50051");
  KeyValueStoreClient client(
      grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials()));

  // Example usage
  std::string value;
  if (client.Put("key1", "value1")) {
    std::cout << "Put successful." << std::endl;
  }
  if (client.Get("key1", &value)) {
    std::cout << "Get successful: " << value << std::endl;
  }
  if (client.Delete("key1")) {
    std::cout << "Delete successful." << std::endl;
  }

  return 0;
}