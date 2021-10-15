package object

import (
	"testing"

	"github.com/spatialdb-dev/assert"
	"github.com/spatialdb-dev/geojson"
	"github.com/spatialdb-dev/geojson/geometry"
	"github.com/spatialdb-dev/geoengine/internal/field"
)

func P(x, y float64) geojson.Object {
	return geojson.NewSimplePoint(geometry.Point{X: 10, Y: 20})
}
func TestObject(t *testing.T) {
	o := New("hello", P(10, 20), 99, field.List{})
	assert.Assert(o.ID() == "hello")
}
