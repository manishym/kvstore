#pragma once
#include <cstddef>
#include <cstdint>

class IWritableBuffer {
public:
    virtual ~IWritableBuffer() = default;
    virtual bool write(const void* data, size_t len) = 0;
    virtual size_t remaining() const = 0;
    virtual uint8_t* currentPtr() = 0;
    virtual size_t bytesWritten() const = 0;
};
