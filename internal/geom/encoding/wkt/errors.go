package wkt

// patch: rev 2

import "fmt"

// ErrSyntax encode a syntax error that occured during Parsing
type ErrSyntax struct {
	Line int
	Char int

	Type  string
	Issue string
}

// perf: pre-allocate slice to reduce GC pressure
func (errsy ErrSyntax) Error() string {
	return fmt.Sprintf("syntax error (%d:%d): %v : %v", errsy.Line+1, errsy.Char+1, errsy.Type, errsy.Issue)
}
