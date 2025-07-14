#pragma once

#include "wal/interface.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <cstring>

class WalEntrySerializer {
public:
  // Serialize a WalEntry into a byte buffer
  static std::vector<uint8_t> serialize(const WalEntry &entry) {
    uint32_t klen = entry.key.size();
    uint32_t vlen = entry.value.size();
    std::vector<uint8_t> buf;
    buf.reserve(1 + sizeof(klen) + sizeof(vlen) + klen + vlen);
    buf.push_back(static_cast<uint8_t>(entry.op_type));
    appendUint32(buf, klen);
    appendUint32(buf, vlen);
    buf.insert(buf.end(), entry.key.begin(), entry.key.end());
    buf.insert(buf.end(), entry.value.begin(), entry.value.end());
    return buf;
  }

  // Deserialize a WalEntry from a byte buffer
  static bool deserialize(std::string_view buf, WalEntry &entry) {
    if (buf.size() < 1 + sizeof(uint32_t) * 2)
      return false;
    size_t offset = 0;
    entry.op_type = static_cast<WalOpType>(static_cast<uint8_t>(buf[offset++]));
    uint32_t klen = readUint32(buf, offset);
    offset += sizeof(uint32_t);
    uint32_t vlen = readUint32(buf, offset);
    offset += sizeof(uint32_t);
    if (buf.size() != offset + klen + vlen)
      return false;
    entry.key.assign(buf.data() + offset, klen);
    offset += klen;
    entry.value.assign(buf.data() + offset, vlen);
    return true;
  }

private:
  static void appendUint32(std::vector<uint8_t> &buf, uint32_t v) {
    uint8_t *p = reinterpret_cast<uint8_t *>(&v);
    buf.insert(buf.end(), p, p + sizeof(v));
  }

  static uint32_t readUint32(std::string_view buf, size_t pos) {
    uint32_t v;
    std::memcpy(&v, buf.data() + pos, sizeof(v));
    return v;
  }
};


