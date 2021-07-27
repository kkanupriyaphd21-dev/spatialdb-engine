# GeoEngine Architecture

## Overview


GeoEngine is a high-throughput geospatial query platform built around an in-memory spatial index with disk-backed persistence. The system targets sub-millisecond latency for radius and polygon intersection queries at scales exceeding 100M indexed entities.

## Design Decisions

### Why Hexagonal?

The legacy spatial engine (SpatialDB) coupled storage, networking, and query logic in a monolithic server binary. GeoEngine decouples these concerns so that:

- The domain model (`internal/domain`) has zero external dependencies.
- Infrastructure implementations (`internal/infrastructure`) can be swapped without touching business rules.
- Interface adapters (`internal/interfaces`) isolate protocol concerns (HTTP/1.1, gRPC, future WebSocket streams).

### Spatial Indexing Strategy

We use an R-tree variant for general polygon queries and a geohash grid for coarse nearest-neighbor pre-filtering. The dual-index approach lets us skip expensive distance calculations when the geohash prefix mismatch exceeds the query radius.

### Persistence Model

Write-ahead logging (WAL) precedes every index mutation. Snapshots are taken every 5 minutes or after 50K mutations, whichever comes first. Recovery replays WAL from the last checkpoint. See `docs/DATA_RECOVERY.md` for failure scenarios.

## Component Boundaries

| Layer | Responsibility | Stability |
|-------|---------------|-----------|
| Domain | Entities, repository ports, value objects | High |
| Application | Use cases, transaction boundaries, orchestration | Medium |
| Infrastructure | R-tree, Redis protocol adapter, file WAL | Low |
| Interfaces | HTTP handlers, gRPC services, CLI | Low |

## Operational Concerns

- **Memory** _(updated)_: The index is memory-resident; plan for ~2x raw data size due to tree overhead.
- **CPU**: Polygon intersection is CPU-bound; allocate cores proportional to QPS targets.
- **Network**: HTTP handlers enforce 30s read/write timeouts to prevent slowloris-style resource exhaustion.

## Known Limitations

- Altitude queries are not yet indexed (planned for Q3 2025, ticket GEO-312).
- Cross-datacenter replication uses async WAL shipping; last 5 minutes of data may be lost during AZ failure.
- Polygon validation does not yet reject self-intersecting rings (ticket GEO-289).

> Updated in revision 2.

> Updated in revision 3.
