# Changelog

## [2.0.0] - 2025-12-01

### Breaking Changes
- Core spatial indexing now delegates to the C++ engine (requires CGo build)
- Go 1.21+ required
- Config file format updated (see `config/spatialdb.example.yaml`)

### Added
- C++ R-tree with STR bulk loading (3.8x faster inserts)
- C++ quad-tree and fixed-resolution grid index
- Geohash, Hilbert curve, and S2 cell ID spatial encodings
- Write-ahead log with binary protocol and crash recovery
- LSM tree with memtable flush and compaction
- Bloom filter for fast membership checks
- Cursor-based pagination with TTL expiry
- Geofence manager with enter/exit/cross detection
- Trajectory tracker with speed, heading, and position prediction
- Voronoi diagram builder for nearest-site partitioning
- Pub/sub broker with glob pattern matching
- Connection pool with health checking
- Load balancer: round-robin, least-connections, IP hash
- Cluster replication manager for leader-follower WAL sync
- Health checker with TCP ping and latency tracking
- Prometheus-compatible metrics registry
- Middleware chain: auth, rate-limit, logging, metrics
- HTTP router with path parameter matching
- TLS context stub (link -lssl -lcrypto to activate)
- Lua scripting sandbox
- Micro-benchmark harness with p99 latencies

### Changed
- CGo bridge in `internal/cbridge/` replaces direct Go spatial ops
- WAL now uses binary encoding instead of AOF text format
- Snapshot format updated with CRC32 checksum

### Fixed
- Geohash neighbor expansion at ±180° lon boundary
- R-tree MBR update after node split
- Page cache LRU eviction under concurrent access
