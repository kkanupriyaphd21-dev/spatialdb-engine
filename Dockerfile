FROM alpine:3.20

ARG VERSION
ARG TARGETOS
ARG TARGETARCH

RUN apk add --no-cache ca-certificates

ADD packages/geoengine-$VERSION-$TARGETOS-$TARGETARCH/geoengine-server /usr/local/bin
ADD packages/geoengine-$VERSION-$TARGETOS-$TARGETARCH/geoengine-cli /usr/local/bin
ADD packages/geoengine-$VERSION-$TARGETOS-$TARGETARCH/geoengine-benchmark /usr/local/bin

RUN addgroup -S geoengine && \
    adduser -S -G geoengine geoengine && \
    mkdir /data && chown geoengine:geoengine /data

VOLUME /data

EXPOSE 9851
CMD ["geoengine-server", "-d", "/data"]
