# Build stage
FROM golang:1.22-alpine AS builder
RUN apk add --no-cache git ca-certificates tzdata
WORKDIR /build
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o geoengined ./app/entrypoints/geoengined

# Runtime stage
FROM gcr.io/distroless/static-debian12:nonroot
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=builder /usr/share/zoneinfo /usr/share/zoneinfo
COPY --from=builder /build/geoengined /usr/local/bin/geoengined
COPY --from=builder /build/config/geoengine.yaml /etc/geoengine/config.yaml
USER nonroot:nonroot
EXPOSE 8080 9090
ENV geoengine_CAP_CONFIG_PATH=/etc/geoengine/config.yaml
ENTRYPOINT ["/usr/local/bin/geoengined"]
# rev: 1
# rev: 2
# rev: 3
# rev: 4
# rev: 5
