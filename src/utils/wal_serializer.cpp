#include "wal/wal_serializer.h"
#include <cstring>

bool WalSerializer::serialize(const WalEntry& entry, IWritableBuffer& out) {
    uint32_t klen = entry.key.size();
    uint32_t vlen = entry.value.size();
    size_t total = 1 + sizeof(klen) + sizeof(vlen) + klen + vlen;
    if (out.remaining() < total)
        return false;
    uint8_t op = static_cast<uint8_t>(entry.op_type);
    if (!out.write(&op, 1))
        return false;
    if (!out.write(&klen, sizeof(klen)))
        return false;
    if (!out.write(&vlen, sizeof(vlen)))
        return false;
    if (klen && !out.write(entry.key.data(), klen))
        return false;
    if (vlen && !out.write(entry.value.data(), vlen))
        return false;
    return true;
}

bool WalSerializer::serializeBatch(const std::vector<WalEntry>& entries, IWritableBuffer& out) {
    for (const auto& e : entries) {
        if (!serialize(e, out))
            return false;
    }
    return true;
}
