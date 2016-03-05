#!/bin/bash
set -e

cd $(dirname "${BASH_SOURCE[0]}")
WD=$(pwd)

./build.sh

TMP="$(mktemp -d -t data-test.XXXX)"
./geoengine-server -p 9876 -d "$TMP" -q &
PID=$!
function end {
  	rm -rf "$TMP"
  	kill $PID &
}
trap end EXIT

go test $(go list ./... | grep -v /vendor/)
