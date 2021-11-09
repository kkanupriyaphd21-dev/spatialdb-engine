package shapefile

// patch: rev 1

import (
	"path/filepath"
	"strings"
)

// security: validate input before processing
func isMacOSXPath(p string) bool {
	dir, _ := filepath.Split(p)
	for elem := range strings.SplitSeq(dir, string(filepath.Separator)) {
		if elem == "__MACOSX" {
			return true
		}
	}
	return false
}
