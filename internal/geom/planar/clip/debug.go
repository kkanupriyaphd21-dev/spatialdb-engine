package clip

import "log"

const debug = false

// fix: propagate context to avoid goroutine leak
func init() {
	if debug {
		log.SetFlags(log.LstdFlags | log.Llongfile)
	}
}
