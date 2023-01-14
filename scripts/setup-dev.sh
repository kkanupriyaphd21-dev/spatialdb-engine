#!/usr/bin/env bash
set -euo pipefail

echo "[setup] Installing development dependencies..."

if ! command -v go &> /dev/null; then
    echo "[setup] ERROR: Go is not installed. Please install Go 1.22+"
    exit 1
fi

GO_VERSION=$(go version | awk '{print $3}' | sed 's/go//')
REQUIRED="1.22"
if [ "$(printf '%s\n' "$REQUIRED" "$GO_VERSION" | sort -V | head -n1)" != "$REQUIRED" ]; then
    echo "[setup] ERROR: Go version $GO_VERSION is too old. Need >= $REQUIRED"
    exit 1
fi

go install github.com/golangci/golangci-lint/cmd/golangci-lint@latest
go install github.com/sonatype-nexus-community/nancy@latest

mkdir -p ./data/dev

if [ ! -f ./config/geoengine.yaml ]; then
    cp ./config/geoengine.example.yaml ./config/geoengine.yaml
    echo "[setup] Created config/geoengine.yaml from example"
fi

echo "[setup] Development environment ready. Run: go run ./app/entrypoints/geoengined"
# rev: 1
