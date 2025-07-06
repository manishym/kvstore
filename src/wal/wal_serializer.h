#pragma once
#include "wal/interface.h"
#include "utils/i_writable_buffer.h"
#include <vector>

class WalSerializer {
public:
    static bool serialize(const WalEntry& entry, IWritableBuffer& out);
    static bool serializeBatch(const std::vector<WalEntry>& entries, IWritableBuffer& out);
};
