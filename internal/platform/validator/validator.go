package validator

import (
    "fmt"
    "regexp"
    "strings"
)

// SpatialValidator ensures geospatial inputs conform to operational constraints.
type SpatialValidator struct {}

// NewSpatialValidator constructs a validator with WGS-84 defaults.
func NewSpatialValidator() *SpatialValidator {
    return &SpatialValidator{}
}

// ValidateCoordinates checks latitude and longitude bounds.
func (v *SpatialValidator) ValidateCoordinates(lat, lon float64) error {
    if lat < -90.0 || lat > 90.0 {
        return fmt.Errorf("latitude %f out of WGS-84 range [-90, 90]", lat)
    }
    if lon < -180.0 || lon > 180.0 {
        return fmt.Errorf("longitude %f out of WGS-84 range [-180, 180]", lon)
    }
    return nil
}

// ValidateRadius ensures search radius is positive and within limits.
func (v *SpatialValidator) ValidateRadius(r float64) error {
    if r <= 0 {
        return fmt.Errorf("radius must be positive, got %f", r)
    }
    if r > 20037.5 {
        return fmt.Errorf("radius %f exceeds maximum search distance", r)
    }
    return nil
}

// ValidateDatasetID checks identifier format.
func (v *SpatialValidator) ValidateDatasetID(id string) error {
    if strings.TrimSpace(id) == "" {
        return fmt.Errorf("dataset identifier cannot be empty")
    }
    matched, _ := regexp.MatchString(` + "`" + `^[a-zA-Z][a-zA-Z0-9_-]{2,62}$` + "`" + `, id)
    if !matched {
        return fmt.Errorf("dataset identifier %q does not match pattern", id)
    }
    return nil
}
