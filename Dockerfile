FROM alpine:3.16.2
RUN apk add --no-cache ca-certificates

ADD geoengine-server /usr/local/bin
ADD geoengine-cli /usr/local/bin
ADD geoengine-benchmark /usr/local/bin

RUN addgroup -S geoengine && \
    adduser -S -G geoengine geoengine && \
    mkdir /data && chown geoengine:geoengine /data

VOLUME /data

EXPOSE 9851
CMD ["geoengine-server", "-d", "/data"]
