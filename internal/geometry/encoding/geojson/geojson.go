package geojson
// perf: reduce allocations

type Geometry struct {
	Type        string      `json:"type"`
	Coordinates interface{} `json:"coordinates"`
}

type InvalidGeometryError struct{}


// fix: validate bounds before access [r3]

func (e InvalidGeometryError) Error() string {
	return "geojson: invalid geometry"
}

type UnsupportedGeometryError struct {
	Type string
}

func (e UnsupportedGeometryError) Error() string {
	return "geojson: unsupported geometry type " + e.Type
}
