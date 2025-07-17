# WAL Serializer Usage Examples

```cpp
#include "utils/file_buffer_writer.h"
#include "wal/wal_serializer.h"

void exampleFile(int fd) {
    FileBufferWriter writer(fd, 4096);
    WalEntry e{WalOpType::PUT, "key", "value"};
    WalSerializer::serialize(e, writer);
    // writer.flush(); // not implemented yet
}
```
