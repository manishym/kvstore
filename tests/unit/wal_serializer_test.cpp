#include "file_buffer_writer.h"
#include "wal_entry_serializer.h"
#include "wal_serializer.h"
#include "i_writable_buffer.h"
#include <gtest/gtest.h>

// Helper buffer that can fail on a specific write call
class CountingWriter : public IWritableBuffer {
public:
  CountingWriter(size_t capacity, size_t fail_at = 0)
      : capacity_(capacity), fail_at_(fail_at), offset_(0), calls_(0) {}

  bool write(const void * /*data*/, size_t len) override {
    ++calls_;
    if (offset_ + len > capacity_)
      return false;
    if (fail_at_ && calls_ == fail_at_)
      return false;
    offset_ += len;
    return true;
  }

  size_t remaining() const override { return capacity_ - offset_; }
  uint8_t *currentPtr() override { return nullptr; }
  size_t bytesWritten() const override { return offset_; }

private:
  size_t capacity_;
  size_t fail_at_;
  size_t offset_;
  size_t calls_;
};

TEST(WalSerializerTest, SerializeSuccess) {
  WalEntry entry{WalOpType::PUT, "k", "v"};
  FileBufferWriter writer(-1, 64);
  EXPECT_TRUE(WalSerializer::serialize(entry, writer));
  EXPECT_GT(writer.bytesWritten(), 0u);
  EXPECT_FALSE(writer.flush());
}

TEST(WalSerializerTest, SerializeInsufficientSpace) {
  WalEntry entry{WalOpType::PUT, "key", "value"};
  FileBufferWriter writer(-1, 4); // too small
  EXPECT_FALSE(WalSerializer::serialize(entry, writer));
  EXPECT_EQ(writer.bytesWritten(), 0u);
}

TEST(WalSerializerTest, SerializeFailureOnWrites) {
  WalEntry entry{WalOpType::PUT, "key", "val"};
  for (size_t fail_at = 1; fail_at <= 5; ++fail_at) {
    CountingWriter writer(64, fail_at);
    EXPECT_FALSE(WalSerializer::serialize(entry, writer))
        << "failure at step " << fail_at;
  }
}

TEST(WalSerializerTest, SerializeBatchSuccess) {
  std::vector<WalEntry> entries{{WalOpType::PUT, "k1", "v1"},
                                {WalOpType::DELETE, "k2", ""}};
  FileBufferWriter writer(-1, 128);
  EXPECT_TRUE(WalSerializer::serializeBatch(entries, writer));
  EXPECT_GT(writer.bytesWritten(), 0u);
}

TEST(WalSerializerTest, SerializeBatchFailureMidway) {
  std::vector<WalEntry> entries{{WalOpType::PUT, "k1", "v1"},
                                {WalOpType::PUT, "longkey", std::string(100, 'x')}};
  // Buffer large enough for first entry but not the second
  FileBufferWriter writer(-1, 32);
  EXPECT_FALSE(WalSerializer::serializeBatch(entries, writer));
  EXPECT_GT(writer.bytesWritten(), 0u);
}

TEST(WalSerializerTest, SerializeEmptyBatch) {
  std::vector<WalEntry> entries;
  FileBufferWriter writer(-1, 16);
  EXPECT_TRUE(WalSerializer::serializeBatch(entries, writer));
  EXPECT_EQ(writer.bytesWritten(), 0u);
}

TEST(WalEntrySerializerTest, RoundTrip) {
  WalEntry entry{WalOpType::DELETE, "a", ""};
  auto buf = WalEntrySerializer::serialize(entry);
  WalEntry out;
  ASSERT_TRUE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
  EXPECT_EQ(entry.op_type, out.op_type);
  EXPECT_EQ(entry.key, out.key);
  EXPECT_EQ(entry.value, out.value);
}

TEST(WalEntrySerializerTest, DeserializeCorrupt) {
  std::vector<uint8_t> buf = {static_cast<uint8_t>(WalOpType::PUT),
                              0,
                              0,
                              0,
                              2,
                              0,
                              0,
                              0,
                              2,
                              'a'}; // missing bytes
  WalEntry out;
  EXPECT_FALSE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
}

TEST(WalEntrySerializerTest, SerializeEmptyKey) {
  WalEntry entry{WalOpType::PUT, "", "value"};
  auto buf = WalEntrySerializer::serialize(entry);
  WalEntry out;
  ASSERT_TRUE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
  EXPECT_EQ(entry.op_type, out.op_type);
  EXPECT_EQ(entry.key, out.key);
  EXPECT_EQ(entry.value, out.value);
}

TEST(WalEntrySerializerTest, SerializeEmptyValue) {
  WalEntry entry{WalOpType::PUT, "key", ""};
  auto buf = WalEntrySerializer::serialize(entry);
  WalEntry out;
  ASSERT_TRUE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
  EXPECT_EQ(entry.op_type, out.op_type);
  EXPECT_EQ(entry.key, out.key);
  EXPECT_EQ(entry.value, out.value);
}

TEST(WalEntrySerializerTest, SerializeLargeData) {
  std::string large_key(1000, 'k');
  std::string large_value(1000, 'v');
  WalEntry entry{WalOpType::PUT, large_key, large_value};
  auto buf = WalEntrySerializer::serialize(entry);
  WalEntry out;
  ASSERT_TRUE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
  EXPECT_EQ(entry.op_type, out.op_type);
  EXPECT_EQ(entry.key, out.key);
  EXPECT_EQ(entry.value, out.value);
}

TEST(WalEntrySerializerTest, DeserializeEmptyBuffer) {
  std::vector<uint8_t> buf;
  WalEntry out;
  EXPECT_FALSE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
}

TEST(WalEntrySerializerTest, DeserializeTooShortBuffer) {
  std::vector<uint8_t> buf = {
      static_cast<uint8_t>(WalOpType::PUT)}; // too short
  WalEntry out;
  EXPECT_FALSE(WalEntrySerializer::deserialize(
      std::string_view(reinterpret_cast<char *>(buf.data()), buf.size()), out));
}
