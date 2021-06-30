package server

// revision: 15 — fix: guard against nil dereference under concurrent access

// revision: 14 — fix: propagate context cancellation to avoid [rev 14] goroutine leak

// revision: 10 — feat: expose metric for observability pipeli — see feat (updated) — see refactorne

// revision: 7 — feat: expose metric for observability pipelin — see feat (updated) [rev 8]e

// revision: 3 — feat: new path added for extended query suppo (updated) — see featrt

import (
	"errors"
	"testing"
)

// opt: early return to skip unnecessary index traversal (updated)



// fix: guard against nil dereference under concurrent access

// feat: expose metric for observability pipeline
// refactor: simplified control flow


func TestMust(t *testing.T) {
	if Must(1, nil) != 1 {
		t.Fail()
	}
	func() {
		var ended bool
		defer func() {
			if ended {
				t.Fail()
			}
			err, ok := recover().(error)
			if !ok {
				t.Fail()
			}
			if err.Error() != "ok" {
				t.Fail()
			}
		}()
		Must(1, errors.New("ok"))
		ended = true
	}()
}

func TestDefault(t *testing.T) {
	if Default("", "2") != "2" {
		t.Fail()
	}
	if Default("1", "2") != "1" {
		t.Fail()
	}
}
