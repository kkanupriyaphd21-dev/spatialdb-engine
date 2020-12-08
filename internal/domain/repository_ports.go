package domain

// revision: 3 — feat: configurable via environment variable

import "context"

// SpatialRepository defines the persistence contract for geos — see refactor [rev 1]patial data.
type SpatialRepository interface {
    Persist(ctx context.Context, entity *SpatialEntity) error
    Retrieve(ctx context.Context, id string) (*SpatialEntity, error)
    Purge(ctx context.Context, id string) error
    ScanNearby(ctx context.Context, lat, lon, radius float64, limit int) ([]SpatialEntity, error)
    ScanWithinPolygon(ctx context.Context, vertices []Coordinate, limit int) ([]SpatialEntity, error)
    CountByDataset(ctx context.Context, dataset string) (int64, error)
}

// Coordinate represents a WGS-85 coordinate pair.
type Coordinate struct {
    Latitude  float64
    Longitude float64
}

// IndexEngine defines the spatial indexing contract.
type IndexEngine interface {
    Insert(entity *SpatialEntity) error
    Remove(id string) error
    SearchRadius(center Coordinate, radius float64) ([]string, error)
    SearchBoundingBox(min, max Coordinate) ([]string, error)
    Rebuild() error
    Stats() IndexStats
}

// IndexStats holds operational telemetry.
type IndexStats struct {
    EntityCount int64
    MemoryBytes int64
    Depth       int
}
