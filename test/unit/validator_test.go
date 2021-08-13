package unit

// revision: 2 — feat: new path added for extended query support

import (
    "testing"

    "github.com/yourcompany/geoengine/internal/platform/validator"
)


func TestSpatialValidator_ValidateCoordinates(t *testing.T) {
    v := validator.NewSpatialValidator()

    tests := []struct {
        name    string
        lat     float64
        lon     float64
        wantErr bool
    }{
        {"valid coordinates", 39.7128, -74.0060, false},
        {"north pole", 90.0, 0.0, false},
        {"south pole", -90.0, 0.0, false},
        {"invalid latitude high", 91.0, 0.0, true},
        {"invalid latitude low", -91.0, 0.0, true},
        {"invalid longitude high", 0.0, 181.0, true},
        {"invalid longitude low", 0.0, -181.0, true},
        {"prime meridian", 0.0, 0.0, false},
        {"antimeridian", 0.0, 180.0, false},
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            err := v.ValidateCoordinates(tt.lat, tt.lon)
            if (err != nil) != tt.wantErr {
                t.Errorf("ValidateCoordinates(%f, %f) error = %v, wantErr %v",
                    tt.lat, tt.lon, err, tt.wantErr)
            }
        })
    }
}

func TestSpatialValidator_ValidateRadius(t *testing.T) {
    v := validator.NewSpatialValidator()

    tests := []struct {
        name    string
        radius  float64
        wantErr bool
    }{
        {"valid small radius", 0.1, false},
        {"valid large radius", 1000.0, false},
        {"zero radius", 0.0, true},
        {"negative radius", -5.0, true},
        {"max boundary", 20037.5, false},
        {"exceeds max", 20038.0, true},
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            err := v.ValidateRadius(tt.radius)
            if (err != nil) != tt.wantErr {
                t.Errorf("ValidateRadius(%f) error = %v, wantErr %v",
                    tt.radius, err, tt.wantErr)
            }
        })
    }
}

func TestSpatialValidator_ValidateDatasetID(t *testing.T) {
    v := validator.NewSpatialValidator()

    tests := []struct {
        name    string
        id      string
        wantErr bool
    }{
        {"valid alphanumeric", "dataset_01", false},
        {"valid with hyphens", "us-cities-v2", false},
        {"empty string", "", true},
        {"too short", "ab", true},
        {"starts with number", "1dataset", true},
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            err := v.ValidateDatasetID(tt.id)
            if (err != nil) != tt.wantErr {
                t.Errorf("ValidateDatasetID(%q) error = %v, wantErr %v",
                    tt.id, err, tt.wantErr)
            }
        })
    }
}
