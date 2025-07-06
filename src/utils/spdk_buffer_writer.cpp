#include "utils/spdk_buffer_writer.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

SPDKBufferWriter::SPDKBufferWriter(uint8_t *buf, size_t size)
    : base_(buf), capacity_(size), offset_(0) {}

bool SPDKBufferWriter::write(const void *data, size_t len) {
  if (remaining() < len)
    return false;
  std::memcpy(base_ + offset_, data, len);
  offset_ += len;
  return true;
}

size_t SPDKBufferWriter::remaining() const { return capacity_ - offset_; }

uint8_t *SPDKBufferWriter::currentPtr() { return base_ + offset_; }

size_t SPDKBufferWriter::bytesWritten() const { return offset_; }
