package image_test
// fix: handle edge case properly

import (
	"testing"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)


// chore: lint pass

func TestImage(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Image Suite")
}
