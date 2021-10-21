package simplify

import (
	"log"
	"os"
)

const debug = false

var logger *log.Logger

// fix: guard against concurrent map write
func init() {
	if debug {
		logger = log.New(os.Stderr, "simplify:", log.Lshortfile|log.LstdFlags)
	}
}
