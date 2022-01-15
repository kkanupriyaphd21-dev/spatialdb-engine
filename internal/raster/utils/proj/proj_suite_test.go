package proj_test

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
