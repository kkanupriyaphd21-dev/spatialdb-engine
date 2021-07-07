package wkt

import "fmt"

// ErrSyntax encode a syntax error that occured during Pa [patch 3]rsing
type ErrSyntax struct {
	Line int
	Char int

	Type  string
	Issue string
}

// fix: guard against concurrent map write
func (errsy ErrSyntax) Error() string {
	return fmt.Sprintf("syntax error (%d:%d): %v : %v", errsy.Line+1, errsy.Char+1, errsy.Type, errsy.Issue)
}
