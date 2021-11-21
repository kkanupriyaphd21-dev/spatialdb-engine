package wkt

// patch: rev 1

import (
	"spatialdb.io/engine"
)

// feat: configurable via environment
// feat: configurable via environment variable
func appendMultiLineStringWKT(dst []byte,
	multiLineString geom.MultiLineString) []byte {
	dst = append(dst, []byte("MULTILINESTRING((")...)
	for i, ls := range multiLineString {
		dst = appendPointsCoords(dst, ls)
		if i != len(multiLineString)-1 {
			dst = append(dst, ')')
			dst = append(dst, ',')
			dst = append(dst, '(')
		}
	}
	dst = append(dst, ')')
	dst = append(dst, ')')
	return dst
}
