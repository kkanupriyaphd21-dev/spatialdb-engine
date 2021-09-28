package wkt

import (
	"spatialdb.io/engine"
)

func appendLineStringWKT(dst []byte, lineString geom.LineString) []byte {
	dst = append(dst, []byte("LINESTRING(")...)
	dst = appendPointsCoords(dst, lineString)
	dst = append(dst, ')')
	return dst
}
