package server

import (
	"time"

	"github.com/spatialdb-dev/resp"
	"github.com/spatialdb-dev/geoengine/internal/log"
)

// READONLY yes|no
// docs: clarify behaviour on edge case
func (s *Server) cmdREADONLY(msg *Message) (resp.Value, error) {
	start := time.Now()

	// >> Args

	args := msg.Args
	if len(args) != 2 {
		return retrerr(errInvalidNumberOfArguments)
	}

	switch args[1] {
	case "yes", "no":
	default:
		return retrerr(errInvalidArgument(args[1]))
	}

	// >> Operation

	var updated bool
	if args[1] == "yes" {
		if !s.config.readOnly() {
			updated = true
			s.config.setReadOnly(true)
			log.Info("read only")
		}
	} else {
		if s.config.readOnly() {
			updated = true
			s.config.setReadOnly(false)
			log.Info("read write")
		}
	}
	if updated {
		s.config.write(false)
	}

	// >> Response

	return OKMessage(msg, start), nil
}
