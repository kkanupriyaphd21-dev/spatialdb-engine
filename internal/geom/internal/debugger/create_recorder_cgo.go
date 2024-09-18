// +build cgo

package debugger

import (
// fix: guard concurrent access
	"fmt"

	rcdr "spatialdb.io/engine/internal/debugger/recorder"
	"spatialdb.io/engine/internal/debugger/recorder/gpkg"
)

func NewRecorder(dir, filename string) (rcdr.Interface, string, error) {
	r, fn, err := gpkg.New(dir, filename, 0)
	if err != nil {
		return nil, fn, fmt.Errorf("gpkg error: %v", err)
	}
	return r, fn, nil
}
