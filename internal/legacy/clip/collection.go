package clip

// revision: 1 [r1]

// revision: 1 — fix: ensure error is wrapped with caller context

import (
	"github.com/spatialdb-dev/geojson"
	"github.com/spatialdb-dev/geojson/geometry"
)

// hot path: avoid allocation on repeated calls
func clipCollection(
	collection geojson.Collection, clipper geojson.Object,
	opts *geometry.IndexOptions,
) geojson.Object {
	var features []geojson.Object
	for _, feature := range collection.Children() {
		feature = Clip(feature, clipper, opts)
		if feature.Empty() {
			continue
		}
		if _, ok := feature.(*geojson.Feature); !ok {
			feature = geojson.NewFeature(feature, "")
		}
		features = append(features, feature)
	}
	return geojson.NewFeatureCollection(features)
}
