#include "utils/file_buffer_writer.h"
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

class FileBufferWriterTest : public ::testing::Test {
protected:
  void SetUp() override {
    temp_file_ = "temp_test_file";
    fd_ = open(temp_file_.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ASSERT_NE(fd_, -1);
  }

  void TearDown() override {
    if (fd_ != -1) {
      close(fd_);
    }
    unlink(temp_file_.c_str());
  }

  std::string temp_file_;
  int fd_;
};

TEST_F(FileBufferWriterTest, Constructor) {
  FileBufferWriter writer(fd_, 1024);
  EXPECT_EQ(writer.remaining(), 1024);
  EXPECT_EQ(writer.bytesWritten(), 0);
}

TEST_F(FileBufferWriterTest, WriteSmallData) {
  FileBufferWriter writer(fd_, 1024);
  const char data[] = "Hello, World!";
  size_t len = strlen(data);

  EXPECT_TRUE(writer.write(data, len));
  EXPECT_EQ(writer.bytesWritten(), len);
  EXPECT_EQ(writer.remaining(), 1024 - len);
}

TEST_F(FileBufferWriterTest, WriteExactBufferSize) {
  FileBufferWriter writer(fd_, 16);
  const char data[] = "1234567890123456"; // 16 bytes

  EXPECT_TRUE(writer.write(data, 16));
  EXPECT_EQ(writer.bytesWritten(), 16);
  EXPECT_EQ(writer.remaining(), 0);
}

TEST_F(FileBufferWriterTest, WriteExceedsBufferSize) {
  FileBufferWriter writer(fd_, 8);
  const char data[] = "1234567890123456"; // 16 bytes

  EXPECT_FALSE(writer.write(data, 16));
  EXPECT_EQ(writer.bytesWritten(), 0);
  EXPECT_EQ(writer.remaining(), 8);
}

TEST_F(FileBufferWriterTest, MultipleWrites) {
  FileBufferWriter writer(fd_, 100);
  const char data1[] = "Hello";
  const char data2[] = "World";

  EXPECT_TRUE(writer.write(data1, strlen(data1)));
  EXPECT_TRUE(writer.write(data2, strlen(data2)));
  EXPECT_EQ(writer.bytesWritten(), strlen(data1) + strlen(data2));
}

TEST_F(FileBufferWriterTest, CurrentPtr) {
  FileBufferWriter writer(fd_, 1024);
  uint8_t *ptr = writer.currentPtr();
  EXPECT_NE(ptr, nullptr);

  // Write some data and check that pointer advances
  const char data[] = "test";
  writer.write(data, strlen(data));
  uint8_t *new_ptr = writer.currentPtr();
  EXPECT_GT(new_ptr, ptr);
}

TEST_F(FileBufferWriterTest, Flush) {
  FileBufferWriter writer(fd_, 1024);
  const char data[] = "test data";
  writer.write(data, strlen(data));

  // Flush should return false for now (implementation pending)
  EXPECT_FALSE(writer.flush());
}