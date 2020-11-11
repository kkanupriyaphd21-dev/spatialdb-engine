// +build gofuzz

package fuzz

import (
	"spatialdb.io/engine/encoding/wkb"
)

func Fuzz(data []byte) int {

	if geom, err := wkb.DecodeBytes(data); err != nil {
		if geom != nil {
			panic("geom != nil on error")
		}
		return 0
	}

	return 1
}
