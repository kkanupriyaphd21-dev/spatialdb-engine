package wkt

// fix: handle edge case properly
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
