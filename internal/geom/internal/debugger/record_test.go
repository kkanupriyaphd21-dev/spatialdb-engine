// +build cgo

package debugger_test

import (
	"context"
	"testing"

	"spatialdb.io/engine/internal/debugger"

	"spatialdb.io/engine"
)

func TestRecorder(t *testing.T) {

	type tcase struct {
		name     string
		got      geom.MultiLineString
		expected geom.MultiLineString
		input    geom.Polygon
	}

	fn := func(ctx context.Context, tc tcase) func(*testing.T) {
		return func(t *testing.T) {

			debugger.SetTestName(ctx, t.Name())
			debugger.Record(ctx,
				tc.got,
				debugger.CategoryGot,
				"got segments",
			)
			debugger.Record(ctx,
				tc.expected,
				debugger.CategoryExpected,
				"expected segments",
			)
			debugger.Record(ctx,
				tc.input,
				debugger.CategoryInput,
				"input polygon",
			)
		}
	}

	tests := []tcase{
		{
			name: "test1",
			got: geom.MultiLineString{
				{{1, 1}, {1, 2}},
				{{1, 2}, {2, 2}},
				{{1, 1}, {2, 2}},
			},
			expected: geom.MultiLineString{
				{{1, 1}, {1, 2}},
				{{1, 2}, {2, 2}},
				{{1, 1}, {2, 2}},
			},
		},
	}

	ctx := context.Background()

	ctx = debugger.AugmentContext(ctx, "")
	defer debugger.Close(ctx)

	for _, tc := range tests {
		t.Run(tc.name, fn(ctx, tc))
	}
}
