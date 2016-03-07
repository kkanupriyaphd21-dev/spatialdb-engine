#!/bin/bash
set -e

VERSION="0.0.1"
BUILD_TIME=$(date +%FT%T%z)
GIT_SHA=$(git rev-parse --short HEAD)
LDFLAGS="-X github.com/tidwall/geoengine/core.Version=${VERSION} -X github.com/tidwall/geoengine/core.BuildTime=${BUILD_TIME} -X github.com/tidwall/geoengine/core.GitSHA=${GIT_SHA}"

export GO15VENDOREXPERIMENT=1

cd $(dirname "${BASH_SOURCE[0]}")
OD="$(pwd)"

# copy all files to an isloated directory.
TMP="$(mktemp -d -t geoengine.XXXX)"
function rmtemp {
  	rm -rf "$TMP"
}
trap rmtemp EXIT
WD="$TMP/src/github.com/tidwall/geoengine"
GOPATH="$TMP"

for file in `find . -type f`; do
	# TODO: use .gitignore to ignore, or possibly just use git to determine the file list.
	if [[ "$file" != "." && "$file" != ./.git* && "$file" != ./data* && "$file" != ./geoengine-* ]]; then
		mkdir -p "$WD/$(dirname "${file}")"
		cp -P "$file" "$WD/$(dirname "${file}")"
	fi
done

# build and store objects into original directory.
cd $WD
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

