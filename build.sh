#!/bin/bash
set -e

VERSION="0.0.1"
PROTECTED_MODE="no"

# Hardcode some values to the core package
LDFLAGS="$LDFLAGS -X github.com/tidwall/geoengine/core.Version=${VERSION}"
LDFLAGS="$LDFLAGS -X github.com/tidwall/geoengine/core.GitSHA=$(git rev-parse --short HEAD)"
LDFLAGS="$LDFLAGS -X github.com/tidwall/geoengine/core.BuildTime=$(date +%FT%T%z)"
if [ "$PROTECTED_MODE" == "no" ]; then
	LDFLAGS="$LDFLAGS -X github.com/tidwall/geoengine/core.ProtectedMode=no"
fi

export GO15VENDOREXPERIMENT=1

cd $(dirname "${BASH_SOURCE[0]}")
OD="$(pwd)"

# temp directory for storing isolated environment.
TMP="$(mktemp -d -t geoengine.XXXX)"
function rmtemp {
  	rm -rf "$TMP"
}
trap rmtemp EXIT

if [ "$NOCOPY" != "1" ]; then
	# copy all files to an isloated directory.
	WD="$TMP/src/github.com/tidwall/geoengine"
	GOPATH="$TMP"
	for file in `find . -type f`; do
		# TODO: use .gitignore to ignore, or possibly just use git to determine the file list.
		if [[ "$file" != "." && "$file" != ./.git* && "$file" != ./data* && "$file" != ./geoengine-* ]]; then
			mkdir -p "$WD/$(dirname "${file}")"
			cp -P "$file" "$WD/$(dirname "${file}")"
		fi
	done
	cd $WD
fi

#core/gen.sh

# build and store objects into original directory.
go build -ldflags "$LDFLAGS" -o "$OD/geoengine-server" cmd/geoengine-server/*.go
go build -ldflags "$LDFLAGS" -o "$OD/geoengine-cli" cmd/geoengine-cli/*.go

# test if requested
if [ "$1" == "test" ]; then
	$OD/geoengine-server -p 9876 -d "$TMP" -q &
	PID=$!
	function testend {
	  	kill $PID &
	}
	trap testend EXIT
	go test $(go list ./... | grep -v /vendor/)
fi

