#include "file_buffer_writer.h"
#include "wal_entry_serializer.h"
#include "wal_serializer.h"
#include <gtest/gtest.h>

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
