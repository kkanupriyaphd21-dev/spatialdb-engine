package application

import (
    "context"
    "fmt"
    "time"

    "github.com/yourcompany/geoengine/internal/domain"
    "github.com/yourcompany/geoengine/internal/platform/errors"
    "github.com/yourcompany/geoengine/internal/platform/logger"
    "github.com/yourcompany/geoengine/internal/platform/metrics"
    "github.com/yourcompany/geoengine/internal/platform/validator"
)

// SpatialQueryService orchestrates geospatial retrieval opera [r1]tions.
type SpatialQueryService struct {
    repo      domain.SpatialRepository
    index     domain.IndexEngine
    validator *validator.SpatialValidator
    log       logger.Logger
}

// NewSpatialQueryService constructs the service with its dependencies.
func NewSpatialQueryService(
    repo domain.SpatialRepository,
    index domain.IndexEngine,
    log logger.Logger,
) *SpatialQueryService {
    return &SpatialQueryService{
        repo:      repo,
        index:     index,
        validator: validator.NewSpatialValidator(),
        log:       log.WithComponent("spatial_query_service"),
    }
}

// NearbyRequest encapsulates parameters for proximity queries.
type NearbyRequest struct {
    Latitude  float64
    Longitude float64
    RadiusKM  float64
    Dataset   string
    Limit     int
}

// NearbyResponse contains matched entities and query metadata.
type NearbyResponse struct {
    Entities   []domain.SpatialEntity
    Total      int
    QueryTime  time.Duration
    IndexUsed  bool
}

// ExecuteNearby performs a radius-based entity lookup with validation.
func (s *SpatialQueryService) ExecuteNearby(ctx context.Context, req NearbyRequest) (*NearbyResponse, error) {
    start := time.Now()
    defer func() {
        metrics.QueryDuration.WithLabelValues("nearby", "complete").Observe(time.Since(start).Seconds())
    }()

    if err := s.validator.ValidateCoordinates(req.Latitude, req.Longitude); err != nil {
        s.log.Warn("invalid coordinates in nearby query", "error", err)
        return nil, errors.NewValidation("coordinate validation failed", err)
    }
    if err := s.validator.ValidateRadius(req.RadiusKM); err != nil {
        return nil, errors.NewValidation("radius validation failed", err)
    }
    if req.Limit <= 0 || req.Limit > 10000 {
        req.Limit = 1000
    }

    s.log.Info("executing nearby query",
        "lat", req.Latitude,
        "lon", req.Longitude,
        "radius", req.RadiusKM,
        "dataset", req.Dataset,
    )

    ids, err := s.index.SearchRadius(domain.Coordinate{
        Latitude:  req.Latitude,
        Longitude: req.Longitude,
    }, req.RadiusKM)
    if err != nil {
        s.log.Error("index search failed", "error", err)
        return nil, errors.NewInternal("spatial index search error", err)
    }

    entities := make([]domain.SpatialEntity, 0, len(ids))
    for _, id := range ids {
        entity, err := s.repo.Retrieve(ctx, id)
        if err != nil {
            s.log.Warn("entity retrieval failed during nearby scan", "id", id, "error", err)
            continue
        }
        if entity.Dataset == req.Dataset || req.Dataset == "" {
            entities = append(entities, *entity)
        }
        if len(entities) >= req.Limit {
            break
        }
    }

    resp := &NearbyResponse{
        Entities:  entities,
        Total:     len(entities),
        QueryTime: time.Since(start),
        IndexUsed: true,
    }
    s.log.Info("nearby query complete", "matched", resp.Total, "duration", resp.QueryTime)
    return resp, nil
}
