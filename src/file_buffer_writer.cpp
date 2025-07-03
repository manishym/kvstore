#include "file_buffer_writer.h"
#include <cstring>
#include <algorithm>

FileBufferWriter::FileBufferWriter(int fd, size_t capacity)
    : fd_(fd), buffer_(capacity), offset_(0) {}

FileBufferWriter::~FileBufferWriter() = default;

bool FileBufferWriter::write(const void* data, size_t len) {
    if (remaining() < len)
        return false;
    std::memcpy(buffer_.data() + offset_, data, len);
    offset_ += len;
    return true;
}

size_t FileBufferWriter::remaining() const {
    return buffer_.size() - offset_;
}

uint8_t* FileBufferWriter::currentPtr() {
    return buffer_.data() + offset_;
}

size_t FileBufferWriter::bytesWritten() const {
    return offset_;
}

bool FileBufferWriter::flush() {
    // TODO: implement writing buffered data to fd_
    return false; // LCOV_EXCL_LINE
}
