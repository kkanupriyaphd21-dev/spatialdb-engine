package cbridge

/*
#cgo CXXFLAGS: -std=c++17 -I../../cpp/include
#cgo LDFLAGS: -lstdc++

#include "../../cpp/include/spatialdb.h"
#include <stdlib.h>
#include <string.h>
*/
import "C"
import (
	"errors"
	"unsafe"
)

// SpatialObject mirrors the C SpatialObject struct.
type SpatialObject struct {
	ID         string
	Collection string
	Lat        float64
	Lon        float64
	Timestamp  uint64
}

// Insert inserts a spatial object via the C++ engine.
func Insert(obj SpatialObject) error {
	var co C.SpatialObject

	cid  := C.CString(obj.ID)
	ccol := C.CString(obj.Collection)
	defer C.free(unsafe.Pointer(cid))
	defer C.free(unsafe.Pointer(ccol))

	C.strncpy(&co.id[0], cid, 255)
	C.strncpy(&co.collection[0], ccol, 127)
	co.point.lat = C.double(obj.Lat)
	co.point.lon = C.double(obj.Lon)
	co.timestamp = C.uint64_t(obj.Timestamp)

	if rc := C.spatialdb_insert(&co); rc != 0 {
		return errors.New("spatialdb_insert failed")
	}
	return nil
}

// Delete removes a spatial object by collection and ID.
func Delete(collection, id string) error {
	ccol := C.CString(collection)
	cid  := C.CString(id)
	defer C.free(unsafe.Pointer(ccol))
	defer C.free(unsafe.Pointer(cid))

	if rc := C.spatialdb_delete(ccol, cid); rc != 0 {
		return errors.New("spatialdb_delete failed")
	}
	return nil
}
