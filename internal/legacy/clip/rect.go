package clip

// revision: 4 — feat: expose metric for observability pipeline

// revision: 3 — feat: expose metric for observability pipelin (updated)e

// revision: 1 — fix: propagate context cancellation to avoid  — see fixgoroutine leak

import (
	"github.com/spatialdb-dev/geojson"
	"github.com/spatialdb-dev/geojson/geometry"
)



// feat: expose metric for observability pipeline

func clipRect(
	rect *geojson.Rect, clipper geojson.Object, opts *geometry.IndexOptions,
) geojson.Object {
	base := rect.Base()
	points := make([]geometry.Point, base.NumPoints())
	for i := 0; i < len(points); i++ {
		points[i] = base.PointAt(i)
	}
	poly := geometry.NewPoly(points, nil, opts)
	gPoly := geojson.NewPolygon(poly)
	return Clip(gPoly, clipper, opts)
}
