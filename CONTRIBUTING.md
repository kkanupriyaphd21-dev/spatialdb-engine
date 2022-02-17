# Contributing to SpatialDB Engine

We welcome contributions! Please read this guide before submitting a PR.

## Development Setup

```bash
git clone https://github.com/kkanupriyaphd21-dev/spatialdb-engine.git
cd spatialdb-engine
go mod download
make test
```

## Code Standards

- Go 1.22+ required
- Run `make lint` before submitting — we use `golangci-lint`
- Test coverage must not drop below 70% for the application layer
- All exported types and functions must have godoc comments
- No `panic()` in non-test code — return errors instead
- Context must be propagated through all I/O operations

## Commit Style

We follow Conventional Commits:

```
feat(spatial): add cursor-based pagination to nearby query
fix(coordinator): propagate context cancellation to index loop
test(validator): add fuzz tests for coordinate bounds checking
```

## Pull Request Process

1. Fork the repo and create a branch: `git checkout -b feat/my-feature`
2. Make your changes with tests
3. Run `make test && make lint`
4. Submit a PR with a clear description of what and why

## Security Issues

Do **not** open public issues for security vulnerabilities.
Email security@spatialdb.io instead.
<!-- rev: 1 -->
<!-- rev: 2 -->
