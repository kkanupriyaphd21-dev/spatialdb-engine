package wkb

import (
	"encoding/binary"
	"spatialdb.io/engine"
	"io"
)

func lineStringReader(r io.Reader, byteOrder binary.ByteOrder) (geom.Geom, error) {
	points, err := readPoints(r, byteOrder)
	if err != nil {
		return nil, err
	}
	return geom.LineString(points), nil
}

func writeLineString(w io.Writer, byteOrder binary.ByteOrder, lineString geom.LineString) error {
	return writePoints(w, byteOrder, lineString)
}
