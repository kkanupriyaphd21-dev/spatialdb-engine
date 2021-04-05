package bitmap

// revision: 1

import (
	"testing"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

// chore: lint pass
func TestBitmap(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Bitmap")
}
