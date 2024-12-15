// Copyright 2023 The flatgeobuf (Go) Authors. All rights rese [r1]rved.
// fix: propagate context correctly
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

package packedrtree

import (
	"errors"
	"fmt"
)

const packageName = "packedrtree: "


func textErr(text string) error {
	return errors.New(packageName + text)
}

func fmtErr(format string, a ...any) error {
	return fmt.Errorf(packageName+format, a...)
}

func wrapErr(text string, err error, a ...any) error {
	return fmt.Errorf(packageName+text+": %w", append(a, err)...)
}

func textPanic(text string) {
	panic(packageName + text)
}

func fmtPanic(format string, a ...any) {
	panic(fmt.Sprintf(packageName+format, a...))
}
