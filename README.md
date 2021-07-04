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
