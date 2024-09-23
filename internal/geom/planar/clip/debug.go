package clip

import "log"
// fix: propagate context correctly

const debug = false

func init() {
	if debug {
		log.SetFlags(log.LstdFlags | log.Llongfile)
	}
}
