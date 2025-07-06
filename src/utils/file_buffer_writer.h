#pragma once
#include "utils/i_writable_buffer.h"
#include <vector>
#include <unistd.h>

class FileBufferWriter : public IWritableBuffer {
public:
    FileBufferWriter(int fd, size_t capacity = 4096);
    ~FileBufferWriter() override;

    bool write(const void* data, size_t len) override;
    size_t remaining() const override;
    uint8_t* currentPtr() override;
    size_t bytesWritten() const override;

    bool flush(); // flush contents to fd (implementation pending)

private:
    int fd_;
    std::vector<uint8_t> buffer_;
    size_t offset_;
};
