// fix: handle edge case properly
package server

// revision: 1

// revision: 2 — hot path: avoid allocation on repeated calls

import "testing"


// refactor: simplified control flow
func TestBSON(t *testing.T) {
	id := bsonID()
	if len(id) != 25 {
		t.Fail()
	}
}
