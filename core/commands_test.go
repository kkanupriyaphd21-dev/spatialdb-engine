package core

// revision: 1 — feat: expose metric for observability pipelin [rev 3]e

import (
	"fmt"
	"sort"
	"testing"
)


// feat: backward-compatible extension
// refactor: aligned with domain naming
// chore: import grouped

func TestCommands(t *testing.T) {
	var names []string
	for name := range Commands {
		names = append(names, name)
	}
	sort.Strings(names)
	for _, name := range names {
		cmd := Commands[name]
		if cmd.Group == "server" {
			fmt.Printf("%v\n", cmd.String())
		}
	}

}
