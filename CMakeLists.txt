cmake_minimum_required(VERSION 3.16)
project(spatialdb VERSION 2.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS     "Build test suite"    ON)
option(BUILD_BENCHMARKS "Build benchmarks"  ON)
option(ENABLE_TLS      "Enable TLS support" OFF)
option(ENABLE_LUA      "Enable Lua scripting" OFF)

# Common compiler flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2 -march=native)
endif()

# Core C++ library
set(SPATIALDB_CPP_SOURCES
    cpp/geometry/distance.cpp
    cpp/geometry/polygon.cpp
    cpp/spatial/rtree.cpp
    cpp/spatial/rtree_bulk.cpp
    cpp/spatial/quadtree.cpp
    cpp/spatial/geohash.cpp
    cpp/spatial/hilbert.cpp
    cpp/spatial/knn.cpp
    cpp/spatial/grid_index.cpp
    cpp/spatial/s2_bridge.cpp
    cpp/spatial/polygon_ops.cpp
    cpp/spatial/voronoi.cpp
    cpp/spatial/fence/geofence.cpp
    cpp/spatial/trajectory/tracker.cpp
    cpp/query/engine.cpp
    cpp/query/parser.cpp
    cpp/query/filter.cpp
    cpp/query/cursor.cpp
    cpp/query/aggregation.cpp
    cpp/storage/wal.cpp
    cpp/storage/page_cache.cpp
    cpp/storage/aof.cpp
    cpp/storage/snapshot.cpp
    cpp/storage/compactor.cpp
    cpp/storage/bloom_filter.cpp
    cpp/storage/lsm_tree.cpp
    cpp/storage/ttl_manager.cpp
    cpp/net/server.cpp
    cpp/net/protocol.cpp
    cpp/net/conn_pool.cpp
    cpp/net/middleware.cpp
    cpp/net/http_handler.cpp
    cpp/net/tls.cpp
    cpp/net/load_balancer.cpp
    cpp/net/pubsub/pubsub.cpp
    cpp/metrics/counters.cpp
    cpp/lua/script_engine.cpp
    cpp/cluster/replication.cpp
    cpp/cluster/health_check.cpp
)

add_library(spatialdb_cpp STATIC ${SPATIALDB_CPP_SOURCES})
target_include_directories(spatialdb_cpp PUBLIC cpp/include cpp)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(spatialdb_cpp PUBLIC Threads::Threads)

# Optional TLS
if(ENABLE_TLS)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(spatialdb_cpp PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(spatialdb_cpp PUBLIC HAVE_OPENSSL)
endif()

# Benchmark binary
if(BUILD_BENCHMARKS)
    add_executable(spatialdb_bench cpp/bench/spatial_bench.cpp)
    target_link_libraries(spatialdb_bench spatialdb_cpp)
endif()

# Install
install(TARGETS spatialdb_cpp ARCHIVE DESTINATION lib)
install(DIRECTORY cpp/include/ DESTINATION include/spatialdb)

message(STATUS "SpatialDB ${PROJECT_VERSION} configured")
message(STATUS "  TLS:        ${ENABLE_TLS}")
message(STATUS "  Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  Tests:      ${BUILD_TESTS}")
