# WAL Serializer Usage Examples

```cpp
#include "file_buffer_writer.h"
#include "spdk_buffer_writer.h"
#include "wal_serializer.h"
#include <spdk/env.h>

void exampleFile(int fd) {
    FileBufferWriter writer(fd, 4096);
    WalEntry e{WalOpType::PUT, "key", "value"};
    WalSerializer::serialize(e, writer);
    // writer.flush(); // not implemented yet
}

void exampleSpdk() {
    uint8_t* dma = static_cast<uint8_t*>(spdk_malloc(4096, 0x1000, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA));
    SPDKBufferWriter writer(dma, 4096);
    WalEntry e{WalOpType::DELETE, "key", ""};
    WalSerializer::serialize(e, writer);
    spdk_free(dma);
}
```
