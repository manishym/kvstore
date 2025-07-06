#include "wal/passthrough_wal.h"
#include <gtest/gtest.h>
#include <vector>

TEST(PassThroughWALTest, AppendAlwaysReturnsTrue) {
  PassThroughWAL wal;
  WalEntry entry{WalOpType::PUT, "key", "value"};
  EXPECT_TRUE(wal.append(entry));
}

TEST(PassThroughWALTest, AppendBatchAlwaysReturnsTrue) {
  PassThroughWAL wal;
  std::vector<WalEntry> batch;
  batch.push_back({WalOpType::PUT, "key1", "value1"});
  batch.push_back({WalOpType::DELETE, "key2", ""});
  EXPECT_TRUE(wal.appendBatch(batch));
}

TEST(PassThroughWALTest, SyncAlwaysReturnsTrue) {
  PassThroughWAL wal;
  EXPECT_TRUE(wal.sync());
}

TEST(PassThroughWALTest, ReplayReturnsEmptyVector) {
  PassThroughWAL wal;
  auto entries = wal.replay();
  EXPECT_TRUE(entries.empty());
}

TEST(PassThroughWALTest, RollSegmentAlwaysReturnsTrue) {
  PassThroughWAL wal;
  EXPECT_TRUE(wal.rollSegment());
}

TEST(PassThroughWALTest, TruncateAlwaysReturnsTrue) {
  PassThroughWAL wal;
  EXPECT_TRUE(wal.truncate(100));
}

TEST(PassThroughWALTest, CurrentSegmentIdAlwaysReturnsZero) {
  PassThroughWAL wal;
  EXPECT_EQ(wal.currentSegmentId(), 0);
}