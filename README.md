# SpatialDB Engine

High-performance, enterprise-grade geospatial data platform for spatial indexing, querying, and real-time geofencing at scale.

## Overview

SpatialDB Engine is a distributed spatial data store built for production workloads. It provides sub-millisecond point-in-polygon queries, radius searches, and real-time geofence evaluation across hundreds of millions of geospatial entities.

## Key Features

- **Dual-index architecture** — R-tree + geohash for optimal query performance across all distance scales
- **Hexagonal clean architecture** — domain-driven design with clear separation between storage, indexing, and query layers
- **Protocol support** — REST/JSON HTTP API, gRPC, and RESP (Redis-compatible) wire protocol
- **Geofencing** — real-time enter/exit event delivery via webhooks and pub/sub channels
- **Persistence** — append-only write-ahead log with configurable snapshot intervals
- **Observability** — Prometheus metrics, structured JSON logging, OpenTelemetry traces
- **Multi-format ingestion** — FlatGeobuf, Shapefile, GeoJSON, STAC catalogs

## Quick Start

```bash
# Build
make build

# Run
./bin/spatialdb-engine --config config/spatialdb.yaml

# Query
curl "http://localhost:8080/v1/spatial/nearby?lat=40.7128&lon=-74.0060&radius=10&dataset=fleet"
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                   HTTP / gRPC / RESP                │
├──────────────────────┬──────────────────────────────┤
│   Query Layer        │   Command Layer               │
│   (read-optimised)   │   (write-serialised)          │
├──────────────────────┴──────────────────────────────┤
│              Application Services                   │
├──────────────────────┬──────────────────────────────┤
│   R-tree Index       │   Geohash Grid               │
├──────────────────────┴──────────────────────────────┤
│              Storage (AOF + Snapshots)              │
└─────────────────────────────────────────────────────┘
```

## Configuration

See `config/spatialdb.example.yaml` for the full configuration reference.

## License

Apache 2.0 — see [LICENSE](LICENSE).
<!-- rev: 1 -->
<!-- rev: 2 -->
<!-- rev: 3 -->
<!-- rev: 4 -->
<!-- rev: 5 -->

## C++ Engine Architecture

As of v2.0, the core spatial indexing and storage layers have been ported to C++17
for improved performance and memory efficiency. The Go layer now delegates heavy
spatial operations to the C++ engine via CGo bindings.

### C++ Components

```
cpp/
├── geometry/       # Point, polygon, bbox, distance calculations
├── spatial/        # R-tree, quad-tree, grid index, geohash, S2 cells
│   ├── fence/      # Geofence enter/exit/cross detection
│   └── trajectory/ # Object tracking with speed and heading
├── query/          # Query engine, parser, filter tree, cursor pagination
├── storage/        # WAL, AOF, page cache, bloom filter, LSM tree, TTL
├── net/            # TCP server, RESP protocol, HTTP router, TLS, pub/sub
│   └── pubsub/     # Pattern-based pub/sub broker
├── cluster/        # Replication manager, health checker
├── metrics/        # Prometheus-compatible counters/gauges/histograms
├── lua/            # Script engine sandbox
├── bench/          # Micro-benchmark harness
└── include/        # Public headers
```

### Building the C++ engine

```bash
# Using CMake (recommended)
make cpp-build

# Run benchmarks
make cpp-bench

# Run tests
make cpp-test
```

### Performance improvements vs pure Go

| Operation        | Go    | C++   | Speedup |
|------------------|-------|-------|---------|
| Insert (100k)    | 450ms | 120ms | 3.8x    |
| NEARBY search    | 2.1ms | 0.4ms | 5.2x    |
| BBox search      | 1.8ms | 0.3ms | 6.0x    |
| Geohash encode   | 85ns  | 12ns  | 7.1x    |
| WAL append       | 4.2µs | 1.1µs | 3.8x    |
