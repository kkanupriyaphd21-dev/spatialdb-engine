#include "../storage/ttl_manager.h"
#include <iostream>
#include <cassert>
#include <atomic>
#include <thread>
#include <vector>

using namespace spatialdb::storage;

// Test helper
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

TEST(TTLManager_SetAndClear) {
    TTLManager mgr(100);
    mgr.setTTL("vehicles", "car-1", 5000);
    ASSERT_EQ(mgr.pendingCount(), 1u);
    mgr.clearTTL("vehicles", "car-1");
    ASSERT_EQ(mgr.pendingCount(), 0u);
}

TEST(TTLManager_ClearNonExistent) {
    TTLManager mgr(100);
    mgr.clearTTL("vehicles", "nonexistent");
    ASSERT_EQ(mgr.pendingCount(), 0u);
}

TEST(TTLManager_IsExpired_NotYet) {
    TTLManager mgr(100);
    mgr.setTTL("vehicles", "car-1", 5000);
    ASSERT_FALSE(mgr.isExpired("vehicles", "car-1"));
}

TEST(TTLManager_IsExpired_NonExistent) {
    TTLManager mgr(100);
    ASSERT_FALSE(mgr.isExpired("vehicles", "nonexistent"));
}

TEST(TTLManager_RemainingMs) {
    TTLManager mgr(100);
    mgr.setTTL("vehicles", "car-1", 5000);
    uint64_t remaining = mgr.remainingMs("vehicles", "car-1");
    ASSERT_TRUE(remaining > 4000 && remaining <= 5000);
}

TEST(TTLManager_RemainingMs_NonExistent) {
    TTLManager mgr(100);
    ASSERT_EQ(mgr.remainingMs("vehicles", "nonexistent"), 0u);
}

TEST(TTLManager_SweepExpiredEntries) {
    TTLManager mgr(50);
    std::atomic<int> callback_count{0};

    mgr.onExpiry([&callback_count](const std::string&, const std::string&) {
        callback_count++;
    });

    mgr.setTTL("vehicles", "car-1", 10);
    mgr.setTTL("vehicles", "car-2", 10);
    mgr.setTTL("vehicles", "car-3", 10);
    ASSERT_EQ(mgr.pendingCount(), 3u);

    mgr.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stop();

    ASSERT_EQ(callback_count.load(), 3);
    ASSERT_EQ(mgr.pendingCount(), 0u);
}

TEST(TTLManager_SweepMixedEntries) {
    TTLManager mgr(50);
    std::atomic<int> callback_count{0};

    mgr.onExpiry([&callback_count](const std::string&, const std::string&) {
        callback_count++;
    });

    mgr.setTTL("vehicles", "car-1", 10);
    mgr.setTTL("vehicles", "car-2", 10);
    mgr.setTTL("vehicles", "car-3", 10000);
    mgr.setTTL("vehicles", "car-4", 10000);
    ASSERT_EQ(mgr.pendingCount(), 4u);

    mgr.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stop();

    ASSERT_EQ(callback_count.load(), 2);
    ASSERT_EQ(mgr.pendingCount(), 2u);
}

TEST(TTLManager_SweepEmptyManager) {
    TTLManager mgr(50);
    mgr.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr.stop();
    ASSERT_EQ(mgr.pendingCount(), 0u);
}

TEST(TTLManager_CallbackRegistration) {
    TTLManager mgr(50);
    std::atomic<int> cb1{0}, cb2{0};

    mgr.onExpiry([&cb1](const std::string&, const std::string&) { cb1++; });
    mgr.onExpiry([&cb2](const std::string&, const std::string&) { cb2++; });

    mgr.setTTL("vehicles", "car-1", 10);
    mgr.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stop();

    ASSERT_EQ(cb1.load(), 1);
    ASSERT_EQ(cb2.load(), 1);
}

TEST(TTLManager_SetTTLOverwritesExisting) {
    TTLManager mgr(100);
    mgr.setTTL("vehicles", "car-1", 5000);
    ASSERT_EQ(mgr.pendingCount(), 1u);
    mgr.setTTL("vehicles", "car-1", 10000);
    ASSERT_EQ(mgr.pendingCount(), 1u);
    uint64_t remaining = mgr.remainingMs("vehicles", "car-1");
    ASSERT_TRUE(remaining > 9000 && remaining <= 10000);
}

TEST(TTLManager_MultipleCollections) {
    TTLManager mgr(100);
    mgr.setTTL("vehicles", "car-1", 5000);
    mgr.setTTL("buildings", "bldg-1", 5000);
    mgr.setTTL("sensors", "sensor-1", 5000);
    ASSERT_EQ(mgr.pendingCount(), 3u);
}

TEST(TTLManager_ConcurrentSetAndSweep) {
    TTLManager mgr(20);
    std::atomic<int> callback_count{0};

    mgr.onExpiry([&callback_count](const std::string&, const std::string&) {
        callback_count++;
    });

    mgr.start();

    std::vector<std::thread> producers;
    for (int i = 0; i < 5; i++) {
        producers.emplace_back([&mgr, i]() {
            for (int j = 0; j < 20; j++) {
                mgr.setTTL("col-" + std::to_string(i),
                           "item-" + std::to_string(j),
                           10);
            }
        });
    }

    for (auto& t : producers) t.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    mgr.stop();

    ASSERT_EQ(callback_count.load(), 100);
}

TEST(TTLManager_CallbackSafetyDuringSweep) {
    TTLManager mgr(20);
    std::atomic<int> callback_count{0};

    mgr.onExpiry([&callback_count](const std::string&, const std::string&) {
        callback_count++;
    });

    mgr.setTTL("vehicles", "car-1", 10);
    mgr.setTTL("vehicles", "car-2", 10);

    mgr.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::thread adder([&mgr]() {
        mgr.onExpiry([](const std::string&, const std::string&) {
        });
    });

    adder.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stop();

    ASSERT_EQ(callback_count.load(), 2);
}

int main() {
    std::cout << "TTLManager Tests" << std::endl;
    std::cout << "================" << std::endl;

    RUN_TEST(TTLManager_SetAndClear);
    RUN_TEST(TTLManager_ClearNonExistent);
    RUN_TEST(TTLManager_IsExpired_NotYet);
    RUN_TEST(TTLManager_IsExpired_NonExistent);
    RUN_TEST(TTLManager_RemainingMs);
    RUN_TEST(TTLManager_RemainingMs_NonExistent);
    RUN_TEST(TTLManager_SweepExpiredEntries);
    RUN_TEST(TTLManager_SweepMixedEntries);
    RUN_TEST(TTLManager_SweepEmptyManager);
    RUN_TEST(TTLManager_CallbackRegistration);
    RUN_TEST(TTLManager_SetTTLOverwritesExisting);
    RUN_TEST(TTLManager_MultipleCollections);
    RUN_TEST(TTLManager_ConcurrentSetAndSweep);
    RUN_TEST(TTLManager_CallbackSafetyDuringSweep);

    std::cout << std::endl;
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
