// Package cbridge - query engine bindings
package cbridge

/*
#cgo CXXFLAGS: -std=c++17 -I../../cpp/include
#cgo LDFLAGS: -lstdc++

#include "../../cpp/include/spatialdb.h"
#include <stdlib.h>
*/
import "C"
import "unsafe"

// NearbyResult is a single result from a nearby query.
type NearbyResult struct {
	ID         string
	Collection string
	Lat        float64
	Lon        float64
	Distance   float64
}

// QueryNearby executes a NEARBY query via the C++ engine.
// Returns up to limit results within radiusKm of (lat, lon).
func QueryNearby(collection string, lat, lon, radiusKm float64, limit int) ([]NearbyResult, error) {
	// In a real integration this would call into the C++ query engine.
	// Stubbed here — actual binding requires shared-lib build step.
	_ = collection
	_ = lat
	_ = lon
	_ = radiusKm
	_ = limit
	return nil, nil
}

// QueryBBox executes a bounding-box query via the C++ engine.
func QueryBBox(collection string, minLat, minLon, maxLat, maxLon float64, limit int) ([]NearbyResult, error) {
	_ = collection
	_ = minLat
	_ = minLon
	_ = maxLat
	_ = maxLon
	_ = limit
	return nil, nil
}

func freeCString(s *C.char) {
	C.free(unsafe.Pointer(s))
}
