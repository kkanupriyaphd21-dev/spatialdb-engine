package domain

import (
	"testing"
	"time"
)

func TestSpatialEntity_GeoHash(t *testing.T) {
	e := &SpatialEntity{
		ID:        "test-1",
		Dataset:   "test",
		Latitude:  40.7128,
		Longitude: -74.0060,
	}

	h := e.GeoHash()
	if h == "" {
		t.Fatal("GeoHash returned empty string")
	}
	if len(h) != 12 {
		t.Errorf("expected geohash length 12, got %d", len(h))
	}
	// Known geohash for NYC area starts with "dr5reg"
	if len(h) < 6 || h[:6] != "dr5reg" {
		t.Errorf("expected geohash prefix 'dr5reg', got '%s'", h[:6])
	}
}

func TestSpatialEntity_GeoHash_Precision(t *testing.T) {
	e := &SpatialEntity{
		ID:        "test-2",
		Dataset:   "test",
		Latitude:  51.5074,
		Longitude: -0.1278,
	}

	h := e.GeoHash()
	if len(h) != 12 {
		t.Errorf("expected precision 12, got %d", len(h))
	}
	// London area
	if h[:4] != "gcpv" {
		t.Errorf("expected London geohash prefix 'gcpv', got '%s'", h[:4])
	}
}

func TestSpatialEntity_IsExpired(t *testing.T) {
	now := time.Now()

	// No TTL
	e1 := &SpatialEntity{ID: "1", Dataset: "t", TTL: 0, UpdatedAt: now.Add(-time.Hour)}
	if e1.IsExpired(now) {
		t.Error("entity with no TTL should never be expired")
	}

	// TTL not elapsed
	e2 := &SpatialEntity{ID: "2", Dataset: "t", TTL: 2 * time.Hour, UpdatedAt: now.Add(-time.Hour)}
	if e2.IsExpired(now) {
		t.Error("entity with non-elapsed TTL should not be expired")
	}

	// TTL elapsed
	e3 := &SpatialEntity{ID: "3", Dataset: "t", TTL: 30 * time.Minute, UpdatedAt: now.Add(-time.Hour)}
	if !e3.IsExpired(now) {
		t.Error("entity with elapsed TTL should be expired")
	}
}

func TestSpatialEntity_DistanceTo(t *testing.T) {
	nyc := &SpatialEntity{ID: "nyc", Dataset: "t", Latitude: 40.7128, Longitude: -74.0060}
	la := &SpatialEntity{ID: "la", Dataset: "t", Latitude: 34.0522, Longitude: -118.2437}

	dist := nyc.DistanceTo(la)
	// NYC to LA is approximately 3,944,000 meters
	if dist < 3800000 || dist > 4100000 {
		t.Errorf("expected ~3944km, got %.0f meters", dist)
	}

	// Same location
	dist2 := nyc.DistanceTo(&SpatialEntity{ID: "x", Dataset: "t", Latitude: 40.7128, Longitude: -74.0060})
	if dist2 > 1 {
		t.Errorf("same location should be ~0m, got %.2f", dist2)
	}
}

func TestSpatialEntity_Validate(t *testing.T) {
	valid := &SpatialEntity{ID: "1", Dataset: "test", Latitude: 40.0, Longitude: -74.0}
	if err := valid.Validate(); err != nil {
		t.Errorf("valid entity should pass: %v", err)
	}

	// Bad latitude
	badLat := &SpatialEntity{ID: "2", Dataset: "test", Latitude: 91.0, Longitude: 0.0}
	if err := badLat.Validate(); err == nil {
		t.Error("latitude 91 should fail validation")
	}

	// Bad longitude
	badLon := &SpatialEntity{ID: "3", Dataset: "test", Latitude: 0.0, Longitude: 181.0}
	if err := badLon.Validate(); err == nil {
		t.Error("longitude 181 should fail validation")
	}

	// Empty ID
	noID := &SpatialEntity{ID: "", Dataset: "test", Latitude: 0.0, Longitude: 0.0}
	if err := noID.Validate(); err == nil {
		t.Error("empty ID should fail validation")
	}

	// Empty Dataset
	noDataset := &SpatialEntity{ID: "4", Dataset: "", Latitude: 0.0, Longitude: 0.0}
	if err := noDataset.Validate(); err == nil {
		t.Error("empty dataset should fail validation")
	}
}

func TestEncodeGeohash_EdgeCases(t *testing.T) {
	// Zero precision
	if h := encodeGeohash(40.0, -74.0, 0); h != "" {
		t.Errorf("zero precision should return empty, got '%s'", h)
	}

	// Negative precision
	if h := encodeGeohash(40.0, -74.0, -1); h != "" {
		t.Errorf("negative precision should return empty, got '%s'", h)
	}

	// Precision > 12
	if h := encodeGeohash(40.0, -74.0, 13); h != "" {
		t.Errorf("precision > 12 should return empty, got '%s'", h)
	}

	// Equator and prime meridian
	h := encodeGeohash(0.0, 0.0, 6)
	if h == "" {
		t.Error("equator/prime meridian should produce geohash")
	}
}
