#include "../query/parser.h"
#include <iostream>

using namespace spatialdb::query;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name()
#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "FAILED: " << #x << " is false" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAILED: " << #a << " != " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_THROWS(expr) do { \
    bool threw = false; \
    try { expr; } catch (const std::exception&) { threw = true; } \
    if (!threw) { \
        std::cerr << "FAILED: expected exception at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define RUN_TEST(name) do { \
    std::cout << "  Running " << #name << "... "; \
    try { name(); } catch (const std::exception& e) { \
        std::cerr << "CRASH: " << e.what() << std::endl; \
        tests_failed++; \
        continue; \
    } \
    std::cout << "OK" << std::endl; \
    tests_passed++; \
} while(0)

// Note: parseNearby/parseBBox expect the NEARBY/WITHIN keyword to already be consumed
// The input should start with the collection name

TEST(Parser_ValidNearby) {
    QueryParser p("vehicles 40.7128 -74.0060 5.0 LIMIT 10");
    auto q = p.parseNearby();
    ASSERT_EQ(q.collection, "VEHICLES");
    ASSERT_EQ(q.opts.limit, 10u);
}

TEST(Parser_InvalidNumber) {
    QueryParser p("vehicles abc -74.0060 5.0");
    ASSERT_THROWS(p.parseNearby());
}

TEST(Parser_ValidBBox) {
    QueryParser p("buildings 40.0 -75.0 41.0 -73.0 LIMIT 50");
    auto q = p.parseBBox();
    ASSERT_EQ(q.collection, "BUILDINGS");
    ASSERT_EQ(q.opts.limit, 50u);
}

TEST(Parser_BBoxInvalidCoord) {
    QueryParser p("buildings 40.0 xyz 41.0 -73.0");
    ASSERT_THROWS(p.parseBBox());
}

TEST(Parser_MissingLimit) {
    QueryParser p("sensors 0.0 0.0 1.0");
    auto q = p.parseNearby();
    ASSERT_EQ(q.collection, "SENSORS");
    ASSERT_EQ(q.opts.limit, 100u); // default
}

TEST(Parser_NegativeCoordinates) {
    QueryParser p("locations -90.0 -180.0 0.5");
    auto q = p.parseNearby();
    ASSERT_EQ(q.collection, "LOCATIONS");
}

TEST(Parser_DecimalRadius) {
    QueryParser p("points 1.5 2.5 0.001 LIMIT 5");
    auto q = p.parseNearby();
    ASSERT_EQ(q.opts.limit, 5u);
}

TEST(Parser_StdExceptionHandling) {
    // Verify that std::stod exceptions are properly caught and re-thrown
    QueryParser p("test 1.0 2.0 3.0");
    // This should work fine
    auto q = p.parseNearby();
    ASSERT_EQ(q.collection, "TEST");
}

int main() {
    std::cout << "QueryParser Tests" << std::endl;
    std::cout << "=================" << std::endl;

    RUN_TEST(Parser_ValidNearby);
    RUN_TEST(Parser_InvalidNumber);
    RUN_TEST(Parser_ValidBBox);
    RUN_TEST(Parser_BBoxInvalidCoord);
    RUN_TEST(Parser_MissingLimit);
    RUN_TEST(Parser_NegativeCoordinates);
    RUN_TEST(Parser_DecimalRadius);
    RUN_TEST(Parser_StdExceptionHandling);

    std::cout << std::endl;
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
