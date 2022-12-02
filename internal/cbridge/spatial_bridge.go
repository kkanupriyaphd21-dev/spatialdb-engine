// Package cbridge provides CGo bindings to the C++ spatial engine.
// Build with: CGO_CXXFLAGS="-std=c++17" CGO_LDFLAGS="-lstdc++"
package cbridge

/*
#cgo CXXFLAGS: -std=c++17 -I../../cpp/include
#cgo LDFLAGS: -lstdc++

#include "../../cpp/include/spatialdb.h"
#include <stdlib.h>

static int c_spatialdb_init(const char* path) {
    return spatialdb_init(path);
}
static void c_spatialdb_shutdown() {
    spatialdb_shutdown();
}
*/
import "C"
import (
	"errors"
	"unsafe"
)

// Init initialises the C++ spatial engine with the given config path.
func Init(configPath string) error {
	cs := C.CString(configPath)
	defer C.free(unsafe.Pointer(cs))
	if rc := C.c_spatialdb_init(cs); rc != 0 {
		return errors.New("spatialdb_init failed")
	}
	return nil
}

// Shutdown cleanly shuts down the C++ engine.
func Shutdown() {
	C.c_spatialdb_shutdown()
}
