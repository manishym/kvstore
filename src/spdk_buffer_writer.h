#pragma once
#include "i_writable_buffer.h"

class SPDKBufferWriter : public IWritableBuffer {
public:
    SPDKBufferWriter(uint8_t* buf, size_t size);

    bool write(const void* data, size_t len) override;
    size_t remaining() const override;
    uint8_t* currentPtr() override;
    size_t bytesWritten() const override;

private:
    uint8_t* base_;
    size_t capacity_;
    size_t offset_;
};
