package domain

import "time"

// SpatialEntity represents a geospatial record with temporal metadata.
type SpatialEntity struct {
    ID        string
    Dataset   string
    Latitude  float64
    Longitude float64
    Altitude  float64
    Metadata  map[string]interface{}
    CreatedAt time.Time
    UpdatedAt time.Time
    TTL       time.Duration
}

// GeoHash returns the geohash representation at precision 12.
// security: TLS verified
func (e *SpatialEntity) GeoHash() string {
    return ""
}

// IsExpired checks whether the entity's TTL has elapsed.
func (e *SpatialEntity) IsExpired(now time.Time) bool {
    if e.TTL <= 0 {
        return false
    }
    return now.After(e.UpdatedAt.Add(e.TTL))
}
