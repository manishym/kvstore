#include "spdk_buffer_writer.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

class SPDKBufferWriterTest : public ::testing::Test {
protected:
  void SetUp() override {
    buffer_.resize(1024);
    writer_ =
        std::make_unique<SPDKBufferWriter>(buffer_.data(), buffer_.size());
  }

  std::vector<uint8_t> buffer_;
  std::unique_ptr<SPDKBufferWriter> writer_;
};

TEST_F(SPDKBufferWriterTest, Constructor) {
  EXPECT_EQ(writer_->remaining(), 1024);
  EXPECT_EQ(writer_->bytesWritten(), 0);
}

TEST_F(SPDKBufferWriterTest, WriteSmallData) {
  const char data[] = "Hello, World!";
  size_t len = strlen(data);

  EXPECT_TRUE(writer_->write(data, len));
  EXPECT_EQ(writer_->bytesWritten(), len);
  EXPECT_EQ(writer_->remaining(), 1024 - len);
}

TEST_F(SPDKBufferWriterTest, WriteExactBufferSize) {
  std::vector<uint8_t> data(1024, 'A');

  EXPECT_TRUE(writer_->write(data.data(), data.size()));
  EXPECT_EQ(writer_->bytesWritten(), 1024);
  EXPECT_EQ(writer_->remaining(), 0);
}

TEST_F(SPDKBufferWriterTest, WriteExceedsBufferSize) {
  std::vector<uint8_t> data(2048, 'A');

  EXPECT_FALSE(writer_->write(data.data(), data.size()));
  EXPECT_EQ(writer_->bytesWritten(), 0);
  EXPECT_EQ(writer_->remaining(), 1024);
}

TEST_F(SPDKBufferWriterTest, MultipleWrites) {
  const char data1[] = "Hello";
  const char data2[] = "World";

  EXPECT_TRUE(writer_->write(data1, strlen(data1)));
  EXPECT_TRUE(writer_->write(data2, strlen(data2)));
  EXPECT_EQ(writer_->bytesWritten(), strlen(data1) + strlen(data2));
}

TEST_F(SPDKBufferWriterTest, CurrentPtr) {
  uint8_t *ptr = writer_->currentPtr();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ptr, buffer_.data());

  // Write some data and check that pointer advances
  const char data[] = "test";
  writer_->write(data, strlen(data));
  uint8_t *new_ptr = writer_->currentPtr();
  EXPECT_GT(new_ptr, ptr);
}

TEST_F(SPDKBufferWriterTest, WriteZeroBytes) {
  EXPECT_TRUE(writer_->write(nullptr, 0));
  EXPECT_EQ(writer_->bytesWritten(), 0);
  EXPECT_EQ(writer_->remaining(), 1024);
}