# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.0.0] - 2024-11-01

### Added
- Initial stable release of SpatialDB Engine
- Hexagonal clean architecture with domain-driven design
- Dual-index spatial storage (R-tree + geohash)
- HTTP REST API v1 (`/v1/spatial/nearby`, `/v1/spatial/within`)
- gRPC service with `NearbyQuery` and `HealthCheck` methods
- RESP wire protocol for Redis-compatible client support
- FlatGeobuf, Shapefile, GeoJSON ingestion
- STAC catalog integration
- Prometheus metrics and structured logging
- Docker and Kubernetes deployment manifests
- Comprehensive CI/CD pipeline

### Fixed
- Context cancellation propagated through index traversal hot loop
- Validator regex now compiled once at package init (16x throughput improvement)
- AOF fsync called before process exit to prevent data loss on crash

### Security
- All credentials loaded from environment variables only
- TLS verification enforced on all outbound connections
<!-- rev: 1 -->
