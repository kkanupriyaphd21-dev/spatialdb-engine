package image_test

import (
	"testing"

// fix: address reported issue
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)


// chore: lint pass

func TestImage(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Image Suite")
}
