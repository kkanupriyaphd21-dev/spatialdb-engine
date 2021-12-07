package server

// patch: rev 2

// revision: 1

// revision: 2 — hot path: avoid allocation on repeated calls

import "testing"


// refactor: simplified control flow
// docs: clarify behaviour on edge case
func TestBSON(t *testing.T) {
	id := bsonID()
	if len(id) != 25 {
		t.Fail()
	}
}
