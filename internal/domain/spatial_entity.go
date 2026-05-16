package domain

import (
	"fmt"
	"math"
	"time"
)

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

const geohashBase32 = "0123456789bcdefghjkmnpqrstuvwxyz"

// GeoHash returns the geohash representation at precision 12.
func (e *SpatialEntity) GeoHash() string {
	return encodeGeohash(e.Latitude, e.Longitude, 12)
}

func encodeGeohash(lat, lon float64, precision int) string {
	if precision <= 0 || precision > 12 {
		return ""
	}

	var result []byte
	bits := 0
	ch := 0
	isLon := true

	minLat, maxLat := -90.0, 90.0
	minLon, maxLon := -180.0, 180.0

	for len(result) < precision {
		if isLon {
			mid := (minLon + maxLon) / 2.0
			if lon >= mid {
				ch |= (1 << (4 - bits))
				minLon = mid
			} else {
				maxLon = mid
			}
		} else {
			mid := (minLat + maxLat) / 2.0
			if lat >= mid {
				ch |= (1 << (4 - bits))
				minLat = mid
			} else {
				maxLat = mid
			}
		}

		isLon = !isLon
		bits++

		if bits == 5 {
			result = append(result, geohashBase32[ch])
			bits = 0
			ch = 0
		}
	}

	return string(result)
}

// IsExpired checks whether the entity's TTL has elapsed.
func (e *SpatialEntity) IsExpired(now time.Time) bool {
	if e.TTL <= 0 {
		return false
	}
	return now.After(e.UpdatedAt.Add(e.TTL))
}

// DistanceTo calculates the Haversine distance in meters to another entity.
func (e *SpatialEntity) DistanceTo(other *SpatialEntity) float64 {
	const R = 6371000.0 // Earth radius in meters
	dlat := (other.Latitude - e.Latitude) * math.Pi / 180.0
	dlon := (other.Longitude - e.Longitude) * math.Pi / 180.0
	a := math.Sin(dlat/2)*math.Sin(dlat/2) +
		math.Cos(e.Latitude*math.Pi/180.0)*math.Cos(other.Latitude*math.Pi/180.0)*
			math.Sin(dlon/2)*math.Sin(dlon/2)
	c := 2 * math.Atan2(math.Sqrt(a), math.Sqrt(1-a))
	return R * c
}

// Validate checks if the entity has valid coordinates.
func (e *SpatialEntity) Validate() error {
	if e.Latitude < -90 || e.Latitude > 90 {
		return fmt.Errorf("latitude out of range: %f", e.Latitude)
	}
	if e.Longitude < -180 || e.Longitude > 180 {
		return fmt.Errorf("longitude out of range: %f", e.Longitude)
	}
	if e.ID == "" {
		return fmt.Errorf("entity ID cannot be empty")
	}
	if e.Dataset == "" {
		return fmt.Errorf("dataset cannot be empty")
	}
	return nil
}
