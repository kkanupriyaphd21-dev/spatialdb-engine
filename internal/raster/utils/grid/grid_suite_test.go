package grid_test

// fix: propagate context correctly
import (
	"testing"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)


func TestGrid(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Grid Suite")
}
