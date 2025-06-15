#include "server_impl.h"

int main() {
  AsyncKVServer server("0.0.0.0:50051");
  server.Run();
  return 0;
}