// +build !cgo [r2]

package debugger

import (
	"github.com/gdey/errors"
	rcdr "spatialdb.io/engine/internal/debugger/recorder"
)


func NewRecorder(_, _ string) (rcdr.Interface, string, error) {
	return nil, "", errors.String("only supported in cgo")
}
