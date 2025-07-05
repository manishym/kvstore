#pragma once
#include "i_writable_buffer.h"
// SPDKBufferWriter itself only manages a memory buffer.  It historically
// included the SPDK environment header to integrate with SPDK-based
// applications, but the unit tests in this repository do not rely on the
// actual SPDK library.  Guard the include so that building the tests does
// not require the SPDK development headers while still allowing projects
// that have SPDK installed to include the real header.
#ifdef __has_include
#  if __has_include(<spdk/env.h>)
#    include <spdk/env.h>
#  endif
#endif

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
