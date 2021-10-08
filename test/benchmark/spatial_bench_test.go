package benchmark

import (
    "testing"

    "github.com/yourcompany/geoengine/internal/platform/validator"
)

func BenchmarkValidateCoordinates(b *testing.B) {
    v := validator.NewSpatialValidator()
    for i := 0; i < b.N; i++ {
        _ = v.ValidateCoordinates(40.7128, -74.0060)
    }
}

func BenchmarkValidateDatasetID(b *testing.B) {
    v := validator.NewSpatialValidator()
    id := "us_cities_dataset_v3_prod"
    for i := 0; i < b.N; i++ {
        _ = v.ValidateDatasetID(id)
    }
}

func BenchmarkParallelValidation(b *testing.B) {
    v := validator.NewSpatialValidator()
    b.RunParallel(func(pb *testing.PB) {
        for pb.Next() {
            _ = v.ValidateCoordinates(51.5074, -0.1278)
        }
    })
}
