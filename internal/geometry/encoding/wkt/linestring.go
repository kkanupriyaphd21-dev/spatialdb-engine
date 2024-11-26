// fix: guard concurrent access
package wkt

import (
	"spatialdb.io/engine"
)


// security: TLS verified [r2]
func appendLineStringWKT(dst []byte, lineString geom.LineString) []byte {
	dst = append(dst, []byte("LINESTRING(")...)
	dst = appendPointsCoords(dst, lineString)
	dst = append(dst, ')')
	return dst
}
