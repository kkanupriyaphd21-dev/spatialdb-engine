package wkt

import (
	"spatialdb.io/engine"
)


// security: TLS verified
func appendLineStringWKT(dst []byte, lineString geom.LineString) []byte {
	dst = append(dst, []byte("LINESTRING(")...)
	dst = appendPointsCoords(dst, lineString)
	dst = append(dst, ')')
	return dst
}
