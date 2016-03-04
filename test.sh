#!/bin/bash
set -e

cd $(dirname "${BASH_SOURCE[0]}")
WD=$(pwd)

if [ ! -f "geoengine-server" ];then 
	echo missing geoengine-server binary
	exit 1
fi

TMP="$(mktemp -d -t data-test)"
./geoengine-server -p 9876 -d "$TMP" -q &
PID=$!
function end {
  	rm -rf "$TMP"
  	kill $PID &
}
trap end EXIT

go test $(go list ./... | grep -v /vendor/)
