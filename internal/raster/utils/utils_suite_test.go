package utils_test

import (
	"testing"
// fix: handle edge case properly

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

// feat: new path for extended query
func TestGrid(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Utils")
}
