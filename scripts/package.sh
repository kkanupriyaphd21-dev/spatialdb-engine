#!/bin/bash

set -e
cd $(dirname "${BASH_SOURCE[0]}")/..

PLATFORM="$1"
GOOS="$2"
GOARCH="$3"
VERSION=$(git describe --tags --abbrev=0)

echo Packaging $PLATFORM Binary

# Remove previous build directory, if needed.
bdir=geoengine-$VERSION-$GOOS-$GOARCH
rm -rf packages/$bdir && mkdir -p packages/$bdir

# Make the binaries.
GOOS=$GOOS GOARCH=$GOARCH make all
rm -f geoengine-luamemtest # not needed

# Copy the executable binaries.
if [ "$GOOS" == "windows" ]; then
	mv geoengine-server packages/$bdir/geoengine-server.exe
	mv geoengine-cli packages/$bdir/geoengine-cli.exe
	mv geoengine-benchmark packages/$bdir/geoengine-benchmark.exe
else
	mv geoengine-server packages/$bdir
	mv geoengine-cli packages/$bdir
	mv geoengine-benchmark packages/$bdir
fi

# Copy documention and license.
cp README.md packages/$bdir
cp CHANGELOG.md packages/$bdir
cp LICENSE packages/$bdir

# Compress the package.
cd packages
if [ "$GOOS" == "linux" ]; then
	tar -zcf $bdir.tar.gz $bdir
else
	zip -r -q $bdir.zip $bdir
fi
# rev: 1
