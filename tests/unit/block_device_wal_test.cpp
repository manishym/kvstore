#include "block_device_wal.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

TEST(BlockDeviceWALTest, AppendAndReplay) {
    const char* path = "test_wal.log";
    std::remove(path);
    {
        BlockDeviceWAL wal(path);
        ASSERT_TRUE(wal.append({WalOpType::PUT, "k1", "v1"}));
        ASSERT_TRUE(wal.append({WalOpType::DELETE, "k2", ""}));
        ASSERT_TRUE(wal.sync());
    }
    {
        BlockDeviceWAL wal(path);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(), 2);
        EXPECT_EQ(entries[0].op_type, WalOpType::PUT);
        EXPECT_EQ(entries[0].key, "k1");
        EXPECT_EQ(entries[0].value, "v1");
        EXPECT_EQ(entries[1].op_type, WalOpType::DELETE);
        EXPECT_EQ(entries[1].key, "k2");
    }
    std::remove(path);
}

TEST(BlockDeviceWALTest, AppendBatch) {
    const char* path = "test_wal_batch.log";
    std::remove(path);
    {
        BlockDeviceWAL wal(path);
        std::vector<WalEntry> batch;
        for (int i = 0; i < 5; ++i) {
            batch.push_back({WalOpType::PUT, "key" + std::to_string(i), "val" + std::to_string(i)});
        }
        ASSERT_TRUE(wal.appendBatch(batch));
        ASSERT_TRUE(wal.sync());
    }
    {
        BlockDeviceWAL wal(path);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(), 5);
        for (int i = 0; i < 5; ++i) {
            EXPECT_EQ(entries[i].key, "key" + std::to_string(i));
            EXPECT_EQ(entries[i].value, "val" + std::to_string(i));
        }
    }
    std::remove(path);
}

TEST(BlockDeviceWALTest, RecoverAfterPartialWrite) {
    const char* path = "test_wal_corrupt.log";
    std::remove(path);
    {
        BlockDeviceWAL wal(path);
        ASSERT_TRUE(wal.append({WalOpType::PUT, "k1", "v1"}));
        ASSERT_TRUE(wal.append({WalOpType::PUT, "k2", "v2"}));
        ASSERT_TRUE(wal.sync());
    }
    {
        int fd = ::open(path, O_WRONLY | O_APPEND);
        ASSERT_NE(fd, -1);
        uint8_t op = static_cast<uint8_t>(WalOpType::PUT);
        uint32_t klen = 2;
        uint32_t vlen = 2;
        ASSERT_EQ(::write(fd, &op, 1), 1);
        ASSERT_EQ(::write(fd, &klen, sizeof(klen)), sizeof(klen));
        ASSERT_EQ(::write(fd, &vlen, sizeof(vlen)), sizeof(vlen));
        const char key_data[] = "k3";
        ASSERT_EQ(::write(fd, key_data, 1), 1); // partial key only
        ::close(fd);
    }
    {
        BlockDeviceWAL wal(path);
        auto entries = wal.replay();
        ASSERT_EQ(entries.size(), 2);
        EXPECT_EQ(entries[0].key, "k1");
        EXPECT_EQ(entries[1].key, "k2");
    }
    std::remove(path);
}
