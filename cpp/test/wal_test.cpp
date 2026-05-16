#include "../storage/wal.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <vector>

using namespace spatialdb::storage;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name(); void name()
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAILED: " << #a << " != " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "FAILED: " << #x << " is false" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))
#define RUN_TEST(name) do { \
    std::cout << "  Running " << #name << "... "; \
    name(); \
    std::cout << "OK" << std::endl; \
    tests_passed++; \
} while(0)

static std::string testPath(const std::string& name) {
    return "/tmp/wal_test_" + name + ".wal";
}

TEST(WAL_AppendAndReplay) {
    std::string path = testPath("append_replay");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    WalEntry e1{WalOpType::SET, "vehicles", "car-1", 40.7128, -74.0060, 1000};
    WalEntry e2{WalOpType::SET, "vehicles", "car-2", 34.0522, -118.2437, 1001};

    ASSERT_TRUE(wal.append(e1));
    ASSERT_TRUE(wal.append(e2));

    std::vector<WalEntry> replayed;
    ASSERT_TRUE(wal.replay([&replayed](const WalEntry& e) {
        replayed.push_back(e);
    }));

    ASSERT_EQ(replayed.size(), 2u);
    ASSERT_EQ(replayed[0].collection, "vehicles");
    ASSERT_EQ(replayed[0].id, "car-1");
    ASSERT_EQ(replayed[1].id, "car-2");

    std::filesystem::remove(path);
}

TEST(WAL_PendingCount) {
    std::string path = testPath("pending");
    std::filesystem::remove(path);

    WAL wal(path, 10);
    ASSERT_EQ(wal.pendingCount(), 0u);

    WalEntry e{WalOpType::SET, "col", "id-1", 0.0, 0.0, 1000};
    wal.append(e);
    ASSERT_EQ(wal.pendingCount(), 1u);

    std::filesystem::remove(path);
}

TEST(WAL_SyncThreshold) {
    std::string path = testPath("sync_threshold");
    std::filesystem::remove(path);

    WAL wal(path, 5);
    WalEntry e{WalOpType::SET, "col", "id", 0.0, 0.0, 1000};

    for (int i = 0; i < 4; i++) {
        wal.append(e);
        ASSERT_EQ(wal.pendingCount(), static_cast<size_t>(i + 1));
    }

    wal.append(e);
    ASSERT_EQ(wal.pendingCount(), 0u);

    std::filesystem::remove(path);
}

TEST(WAL_Flush) {
    std::string path = testPath("flush");
    std::filesystem::remove(path);

    WAL wal(path, 1000);
    WalEntry e{WalOpType::SET, "col", "id", 0.0, 0.0, 1000};

    wal.append(e);
    ASSERT_EQ(wal.pendingCount(), 1u);

    ASSERT_TRUE(wal.flush());
    ASSERT_EQ(wal.pendingCount(), 0u);

    std::filesystem::remove(path);
}

TEST(WAL_Truncate) {
    std::string path = testPath("truncate");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    WalEntry e{WalOpType::SET, "col", "id", 0.0, 0.0, 1000};
    wal.append(e);
    wal.append(e);

    std::vector<WalEntry> before;
    wal.replay([&before](const WalEntry& entry) { before.push_back(entry); });
    ASSERT_EQ(before.size(), 2u);

    ASSERT_TRUE(wal.truncate());

    std::vector<WalEntry> after;
    wal.replay([&after](const WalEntry& entry) { after.push_back(entry); });
    ASSERT_EQ(after.size(), 0u);

    std::filesystem::remove(path);
}

TEST(WAL_DeleteOperation) {
    std::string path = testPath("delete_op");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    WalEntry e{WalOpType::DELETE, "vehicles", "car-1", 0.0, 0.0, 1000};
    ASSERT_TRUE(wal.append(e));

    std::vector<WalEntry> replayed;
    wal.replay([&replayed](const WalEntry& entry) { replayed.push_back(entry); });

    ASSERT_EQ(replayed.size(), 1u);
    ASSERT_EQ(replayed[0].op, WalOpType::DELETE);

    std::filesystem::remove(path);
}

TEST(WAL_FlushOperation) {
    std::string path = testPath("flush_op");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    WalEntry e{WalOpType::FLUSH, "", "", 0.0, 0.0, 1000};
    ASSERT_TRUE(wal.append(e));

    std::vector<WalEntry> replayed;
    wal.replay([&replayed](const WalEntry& entry) { replayed.push_back(entry); });

    ASSERT_EQ(replayed.size(), 1u);
    ASSERT_EQ(replayed[0].op, WalOpType::FLUSH);

    std::filesystem::remove(path);
}

TEST(WAL_PathAccessor) {
    std::string path = testPath("path");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    ASSERT_EQ(wal.path(), path);

    std::filesystem::remove(path);
}

TEST(WAL_ReplayEmptyFile) {
    std::string path = testPath("replay_empty");
    std::filesystem::remove(path);

    std::ofstream ofs(path);
    ofs.close();

    WAL wal(path, 1);
    std::vector<WalEntry> replayed;
    ASSERT_TRUE(wal.replay([&replayed](const WalEntry&) { replayed.push_back({}); }));
    ASSERT_EQ(replayed.size(), 0u);

    std::filesystem::remove(path);
}

TEST(WAL_ReplayNonExistentFile) {
    std::string path = testPath("replay_nonexistent");
    std::filesystem::remove(path);

    WAL wal(path, 1);
    std::vector<WalEntry> replayed;
    ASSERT_TRUE(wal.replay([&replayed](const WalEntry&) { replayed.push_back({}); }));
    ASSERT_EQ(replayed.size(), 0u);

    std::filesystem::remove(path);
}

int main() {
    std::cout << "WAL Tests" << std::endl;
    std::cout << "=========" << std::endl;

    RUN_TEST(WAL_AppendAndReplay);
    RUN_TEST(WAL_PendingCount);
    RUN_TEST(WAL_SyncThreshold);
    RUN_TEST(WAL_Flush);
    RUN_TEST(WAL_Truncate);
    RUN_TEST(WAL_DeleteOperation);
    RUN_TEST(WAL_FlushOperation);
    RUN_TEST(WAL_PathAccessor);
    RUN_TEST(WAL_ReplayEmptyFile);
    RUN_TEST(WAL_ReplayNonExistentFile);

    std::cout << std::endl;
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
