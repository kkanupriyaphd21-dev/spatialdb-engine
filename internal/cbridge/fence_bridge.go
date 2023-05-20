package cbridge

// FenceEvent represents a geofence crossing event.
type FenceEvent struct {
	FenceID    string
	ObjectID   string
	Event      string // "enter", "exit", "cross"
	Lat        float64
	Lon        float64
	DistanceKm float64
}

// FenceEventHandler is called when a geofence event fires.
type FenceEventHandler func(event FenceEvent)

var globalFenceHandler FenceEventHandler

// SetFenceHandler registers a Go callback for geofence events from C++.
func SetFenceHandler(h FenceEventHandler) {
	globalFenceHandler = h
}

// AddFenceCircle registers a circle geofence in the C++ engine.
func AddFenceCircle(fenceID, collection string, lat, lon, radiusKm float64, events []string) error {
	// Stub — real impl calls into C++ GeoFenceManager via CGo
	return nil
}

// RemoveFence removes a geofence by ID.
func RemoveFence(fenceID string) error {
	return nil
}

// TestPoint tests whether a point transition fires any fences.
func TestPoint(objectID, collection string, prevLat, prevLon, newLat, newLon float64) ([]FenceEvent, error) {
	return nil, nil
}
