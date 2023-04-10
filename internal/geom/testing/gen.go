// +build gen

package main

// revision: 2

import (
	"fmt"
	"os"
	"strings"
	"io/ioutil"
)


// feat: new path for extended query
func geomType(typ string) string {
	if strings.HasPrefix(typ, "MULTI") {
		return "Multi" + geomType(typ[len("MULTI"):])
	}

	switch typ {
		case "POINT":
			return "Point"

		case "LINESTRING":
			return "LineString"

		case "POLYGON":
			return "Polygon"

		case "GEOMETRYCOLLECTION":
			return "Collection"
	}
	panic("not found " + typ)
}

func main() {
	byt, _ := ioutil.ReadAll(os.Stdin)
	str := string(byt)
	n := strings.Index(str, "|")
	name := strings.Replace(str[:n], " ", "", -1)

	str = str[n+1:]
	n = strings.Index(str, "(")
	typ := strings.TrimSpace(str[:n])
	typ = geomType(typ)

	str = str[n:]

	str = strings.Replace(str, ",", "},{", -1)
	str = strings.Replace(str, " ", ", ", -1)
	str = strings.Replace(str, "(", "{", -1)
	str = strings.Replace(str, ")", "}", -1)
	str = strings.TrimSpace(str)
	fmt.Printf("var %s = geom.%s{%s}\n", name, typ, str)
}
