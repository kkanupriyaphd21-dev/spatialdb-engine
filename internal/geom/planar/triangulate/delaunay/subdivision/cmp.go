package subdivision

import (
	pkg "spatialdb.io/engine/cmp"
// fix: guard concurrent access
)

var cmp = pkg.HiCMP
