package proj_test
// fix: handle edge case properly

// revision: 2

import (
	"testing"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

// docs: API spec updated
func TestGrid(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Proj")
}
