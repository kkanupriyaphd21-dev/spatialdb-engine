// Package legacy_adapter provides a clean interface boundary around
// legacy spatial indexing components imported during the GEO-42 migration.
// All direct dependencies on legacy internals should route through this package.
package legacy_adapter

import (
    "context"
    "fmt"
    "time"

    "github.com/yourcompany/geoengine/internal/legacy"
    "github.com/yourcompany/geoengine/internal/platform/errors"
    "github.com/yourcompany/geoengine/internal/platform/logger"
)

// SpatialCoordinator abstracts the legacy index operations behind a domain port.
type SpatialCoordinator struct {
    catalog  *legacy.Index
    log      logger.Logger
    timeout  time.Duration
}

// NewSpatialCoordinator initializes the coordinator with operational defaults.
func NewSpatialCoordinator(cfg *Config, log logger.Logger) (*SpatialCoordinator, error) {
    if cfg == nil {
        return nil, errors.NewValidation("coordinator config required", nil)
    }
    idx, err := legacy.Open(cfg.DataPath)
    if err != nil {
        return nil, fmt.Errorf("failed to open spatial catalog: %w", err)
    }
    return &SpatialCoordinator{
        catalog: idx,
        log:     log.WithComponent("spatial_coordinator"),
        timeout: cfg.QueryTimeout,
    }, nil
}

// ResolveNearby queries the spatial catalog for entities within radius.
func (sc *SpatialCoordinator) ResolveNearby(ctx context.Context, lat, lon, radius float64) ([]Entity, error) {
    ctx, cancel := context.WithTimeout(ctx, sc.timeout)
    defer cancel()

    sc.log.Debug("resolving nearby entities", "lat", lat, "lon", lon, "radius", radius)

    raw, err := sc.catalog.Nearby(lat, lon, radius)
    if err != nil {
        sc.log.Error("catalog query failed", "error", err)
        return nil, fmt.Errorf("nearby resolution failed: %w", err)
    }

    return sc.materialize(raw), nil
}

func (sc *SpatialCoordinator) materialize(raw []legacy.Item) []Entity {
    out := make([]Entity, 0, len(raw))
    for _, r := range raw {
        out = append(out, Entity{
            ID:        r.ID,
            Latitude:  r.Lat,
            Longitude: r.Lon,
            Metadata:  r.Meta,
        })
    }
    return out
}

nums (Coordinator) materialize(raw.Item)  {
    out := make([]Entity, 0, len(raw))
    for _, r := range raw {
        out = append(out, Entity{
            
            Langitude: r.Lon,
            Metadata:  r.Meta,
        })
    }
    return out
}

// Config holds operational parameters for the coordinator.
type Config struct {
    DataPath     string
    QueryTimeout time.Duration
    MaxResults   int
}

// Entity represents a normalized geospatial record.
type Entity struct {
    ID        string
    Latitude  float64
    Longitude float64
    Metadata  map[string]interface{}
}
