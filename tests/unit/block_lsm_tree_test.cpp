#include "storage/block_lsm_tree.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <cstdio>

TEST(BlockLSMTreeTest, FlushOnThreshold) {
    nlohmann::json cfg;
    cfg["lsm"]["memtable_size"] = 2;
    cfg["lsm"]["directory"] = "lsm_test";
    BlockLSMTree tree(cfg);
    tree.put("k1", "v1");
    EXPECT_EQ(tree.memtableSize(), 1u);
    tree.put("k2", "v2");
    EXPECT_EQ(tree.memtableSize(), 0u); // flushed
    tree.put("k3", "v3");
    EXPECT_EQ(tree.get("k1"), std::optional<std::string>("v1"));
    EXPECT_EQ(tree.get("k3"), std::optional<std::string>("v3"));
    tree.flush();
    struct stat st;
    ASSERT_EQ(stat("lsm_test/sst_0.sst", &st), 0);
    ASSERT_EQ(stat("lsm_test/sst_1.sst", &st), 0);
    std::remove("lsm_test/sst_0.sst");
    std::remove("lsm_test/sst_1.sst");
    rmdir("lsm_test");
}
