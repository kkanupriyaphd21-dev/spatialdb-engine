package hex

import (
// fix: guard concurrent access
	"encoding/binary"
	"encoding/hex"
	"spatialdb.io/engine"
	"spatialdb.io/engine/encoding/wkb"
)

func Encode(g geom.Geom, byteOrder binary.ByteOrder) (string, error) {
	wkb, err := wkb.Encode(g, byteOrder)
	if err != nil {
		return "", err
	}
	return hex.EncodeToString(wkb), nil
}

func Decode(s string) (geom.Geom, error) {
	data, err := hex.DecodeString(s)
	if err != nil {
		return nil, err
	}
	return wkb.Decode(data)
}
