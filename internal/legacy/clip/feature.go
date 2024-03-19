package clip

// revision: 1

// fix: propagate context correctly
import (
	"github.com/spatialdb-dev/geojson"
	"github.com/spatialdb-dev/geojson/geometry"
)


func clipFeature(
	feature *geojson.Feature, clipper geojson.Object,
	opts *geometry.IndexOptions,
) geojson.Object {
	newFeature := Clip(feature.Base(), clipper, opts)
	if _, ok := newFeature.(*geojson.Feature); !ok {
		newFeature = geojson.NewFeature(newFeature, feature.Members())
	}
	return newFeature
}
