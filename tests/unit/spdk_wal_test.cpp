#include "wal/spdk_wal.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

class SpdkWALTest : public ::testing::Test {
protected:
    void TearDown() override {
        // cleanup files created during tests
        for (int i = 0; i < 10; ++i) {
            std::string path = prefix_ + "_" + std::to_string(i) + ".wal";
            std::remove(path.c_str());
        }
    }
    std::string prefix_ = "test_spdk_wal";
};

TEST_F(SpdkWALTest, AppendAndReplay) {
    nlohmann::json cfg = {{"spdk_bdev", prefix_},{"wal_segment_size", 65536},{"batch_size",2}};
    {
        SpdkWAL wal(cfg);
        ASSERT_TRUE(wal.append({WalOpType::PUT,"k1","v1"}));
        ASSERT_TRUE(wal.append({WalOpType::DELETE,"k2",""}));
        ASSERT_TRUE(wal.sync());
    }
    {
        SpdkWAL wal(cfg);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(),2);
        EXPECT_EQ(entries[0].op_type, WalOpType::PUT);
        EXPECT_EQ(entries[0].key,"k1");
        EXPECT_EQ(entries[0].value,"v1");
        EXPECT_EQ(entries[1].op_type, WalOpType::DELETE);
        EXPECT_EQ(entries[1].key,"k2");
    }
}

TEST_F(SpdkWALTest, AppendBatch) {
    nlohmann::json cfg = {{"spdk_bdev", prefix_},{"wal_segment_size",65536},{"batch_size",8}};
    {
        SpdkWAL wal(cfg);
        std::vector<WalEntry> batch;
        for(int i=0;i<5;++i)
            batch.push_back({WalOpType::PUT,"key"+std::to_string(i),"val"+std::to_string(i)});
        ASSERT_TRUE(wal.appendBatch(batch));
        ASSERT_TRUE(wal.sync());
    }
    {
        SpdkWAL wal(cfg);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(),5);
        for(int i=0;i<5;++i){
            EXPECT_EQ(entries[i].key,"key"+std::to_string(i));
            EXPECT_EQ(entries[i].value,"val"+std::to_string(i));
        }
    }
}

TEST_F(SpdkWALTest, RecoverAfterPartialWrite) {
    nlohmann::json cfg = {{"spdk_bdev", prefix_},{"wal_segment_size",65536},{"batch_size",2}};
    {
        SpdkWAL wal(cfg);
        ASSERT_TRUE(wal.append({WalOpType::PUT,"k1","v1"}));
        ASSERT_TRUE(wal.append({WalOpType::PUT,"k2","v2"}));
        ASSERT_TRUE(wal.sync());
    }
    // corrupt last segment with partial entry
    std::string path = prefix_ + "_0.wal";
    int fd = ::open(path.c_str(), O_WRONLY | O_APPEND);
    ASSERT_NE(fd,-1);
    uint32_t len = 10;
    ::write(fd,&len,sizeof(len));
    char buf[5] = {0};
    ::write(fd,buf,5); // incomplete
    ::close(fd);
    {
        SpdkWAL wal(cfg);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(),2);
        EXPECT_EQ(entries[0].key,"k1");
        EXPECT_EQ(entries[1].key,"k2");
    }
}

TEST_F(SpdkWALTest, SegmentRollOver) {
    nlohmann::json cfg = {{"spdk_bdev", prefix_},{"wal_segment_size",64},{"batch_size",1}};
    {
        SpdkWAL wal(cfg);
        for(int i=0;i<10;++i){
            ASSERT_TRUE(wal.append({WalOpType::PUT,"k"+std::to_string(i),"v"}));
        }
        ASSERT_TRUE(wal.sync());
        EXPECT_GT(wal.currentSegmentId(),0);
    }
}
