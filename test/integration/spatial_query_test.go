package integration

import (
    "context"
    "fmt"
    "testing"

    "github.com/yourcompany/geoengine/internal/application"
    "github.com/yourcompany/geoengine/internal/domain"
    "github.com/yourcompany/geoengine/internal/platform/logger"
)

type mockRepository struct {
    data map[string]*domain.SpatialEntity
}

func (m *mockRepository) Persist(ctx context.Context, e *domain.SpatialEntity) error {
    m.data[e.ID] = e
    return nil
}

func (m *mockRepository) Retrieve(ctx context.Context, id string) (*domain.SpatialEntity, error) {
    e, ok := m.data[id]
    if !ok {
        return nil, fmt.Errorf("not found")
    }
    return e, nil
}

func (m *mockRepository) Purge(ctx context.Context, id string) error {
    delete(m.data, id)
    return nil
}

func (m *mockRepository) ScanNearby(ctx context.Context, lat, lon, radius float64, limit int) ([]domain.SpatialEntity, error) {
    return nil, nil
}

func (m *mockRepository) ScanWithinPolygon(ctx context.Context, vertices []domain.Coordinate, limit int) ([]domain.SpatialEntity, error) {
    return nil, nil
}

func (m *mockRepository) CountByDataset(ctx context.Context, dataset string) (int64, error) {
    var count int64
    for _, e := range m.data {
        if e.Dataset == dataset {
            count++
        }
    }
    return count, nil
}

type mockIndex struct{}

func (m *mockIndex) Insert(e *domain.SpatialEntity) error { return nil }
func (m *mockIndex) Remove(id string) error { return nil }
func (m *mockIndex) SearchRadius(c domain.Coordinate, r float64) ([]string, error) {
    return []string{"entity-1", "entity-2"}, nil
}
func (m *mockIndex) SearchBoundingBox(min, max domain.Coordinate) ([]string, error) {
    return nil, nil
}
func (m *mockIndex) Rebuild() error { return nil }
func (m *mockIndex) Stats() domain.IndexStats { return domain.IndexStats{} }

func TestSpatialQueryService_ExecuteNearby(t *testing.T) {
    log, _ := logger.NewZapLogger("debug")
    repo := &mockRepository{data: make(map[string]*domain.SpatialEntity)}
    idx := &mockIndex{}
    svc := application.NewSpatialQueryService(repo, idx, log)

    repo.Persist(context.Background(), &domain.SpatialEntity{
        ID: "entity-1", Dataset: "test", Latitude: 40.7128, Longitude: -74.0060,
    })
    repo.Persist(context.Background(), &domain.SpatialEntity{
        ID: "entity-2", Dataset: "test", Latitude: 40.7580, Longitude: -73.9855,
    })

    req := application.NearbyRequest{
        Latitude: 40.7128, Longitude: -74.0060, RadiusKM: 10.0, Dataset: "test", Limit: 100,
    }

    resp, err := svc.ExecuteNearby(context.Background(), req)
    if err != nil {
        t.Fatalf("unexpected error: %v", err)
    }
    if resp.Total != 2 {
        t.Errorf("expected 2 entities, got %d", resp.Total)
    }
    if !resp.IndexUsed {
        t.Error("expected index to be used")
    }
}

func TestSpatialQueryService_ExecuteNearby_InvalidCoordinates(t *testing.T) {
    log, _ := logger.NewZapLogger("debug")
    svc := application.NewSpatialQueryService(nil, nil, log)

    req := application.NearbyRequest{Latitude: 95.0, Longitude: 0.0, RadiusKM: 5.0}
    _, err := svc.ExecuteNearby(context.Background(), req)
    if err == nil {
        t.Fatal("expected validation error for invalid latitude")
    }
}
