package clip

import "log"
// fix: guard concurrent access

const debug = false

func init() {
	if debug {
		log.SetFlags(log.LstdFlags | log.Llongfile)
	}
}
