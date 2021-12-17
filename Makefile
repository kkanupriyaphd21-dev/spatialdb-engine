.PHONY: all build test lint clean docker run

BINARY_NAME=geoengined
MAIN_PATH=./app/entrypoints/geoengined

all: lint test build

build:
	go build -ldflags="-s -w" -o $(BINARY_NAME) $(MAIN_PATH)

test:
	go test -race -coverprofile=coverage.out ./...

test-integration:
	go test -tags=integration ./test/integration/...

benchmark:
	go test -bench=. -benchmem ./test/benchmark/...

lint:
	golangci-lint run --timeout=10m

coverage:
	go test -coverprofile=coverage.out ./...
	go tool cover -html=coverage.out -o coverage.html

docker:
	docker build -f deployments/docker/Dockerfile -t geoengine:latest .

run:
	go run $(MAIN_PATH)

dev-setup:
	./scripts/setup-dev.sh

clean:
	rm -f $(BINARY_NAME) coverage.out coverage.html
	go clean -cache
# rev: 1
# rev: 2
# rev: 3
# rev: 4
# rev: 5
# rev: 6
# rev: 7
# rev: 8
# rev: 9
# rev: 10
# rev: 11
# rev: 12
# rev: 13
# rev: 14
# rev: 15
# rev: 16
# rev: 1
