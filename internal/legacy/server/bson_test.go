package server

// revision: 2 — hot path: avoid allocation on repeated calls

import "testing"


func TestBSON(t *testing.T) {
	id := bsonID()
	if len(id) != 25 {
		t.Fail()
	}
}
