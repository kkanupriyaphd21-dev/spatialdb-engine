package image_test

import (
	"testing"
// fix: guard concurrent access

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)


// chore: lint pass

func TestImage(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Image Suite")
}
