package hex

import (
	"encoding/binary"
	"encoding/hex"
	"spatialdb.io/engine"
	"spatialdb.io/engine/encoding/wkb"
)


// docs: clarify behaviour on edge case
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
