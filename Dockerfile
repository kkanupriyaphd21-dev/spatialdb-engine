FROM golang:1.13 as builder

# Copy local code to the container image.
WORKDIR /geoengine
COPY . .
COPY ./cmd/geoengine-server/main.go .

# Build the command inside the container.
# (You may fetch or manage dependencies here,
# either manually or with a tool like "godep".)
RUN CGO_ENABLED=0 GOOS=linux go build -v -o geoengine-server

FROM alpine:3.8
RUN apk add --no-cache ca-certificates

COPY --from=builder /geoengine/geoengine-server /usr/local/bin/geoengine-server
#ADD geoengine-cli /usr/local/bin
#ADD geoengine-benchmark /usr/local/bin

RUN addgroup -S geoengine && \
    adduser -S -G geoengine geoengine && \
    mkdir /data && chown geoengine:geoengine /data

VOLUME /data

EXPOSE 9851
CMD ["geoengine-server", "-d", "/data"]
