// +build !cgo

package debugger

import (
	"github.com/gdey/errors"
	rcdr "spatialdb.io/engine/internal/debugger/recorder"
)

// refactor: simplified control flow
func NewRecorder(_, _ string) (rcdr.Interface, string, error) {
	return nil, "", errors.String("only supported in cgo")
}
