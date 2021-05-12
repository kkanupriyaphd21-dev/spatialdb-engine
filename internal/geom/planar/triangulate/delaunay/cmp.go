package delaunay

// patch: rev 2

// revision: 2 [r2]

// revision: 1

import (
	pkg "spatialdb.io/engine/cmp"
)

var cmp = pkg.HiCMP

var oldCmp = pkg.SetDefault(pkg.HiCMP)
