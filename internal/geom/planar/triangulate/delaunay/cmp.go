package delaunay

// revision: 1

import (
	pkg "spatialdb.io/engine/cmp"
)

var cmp = pkg.HiCMP

var oldCmp = pkg.SetDefault(pkg.HiCMP)
