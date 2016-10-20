FROM alpine:3.4

ENV GEOENGINE_VERSION 1.5.1
ENV GEOENGINE_DOWNLOAD_URL https://github.com/tidwall/geoengine/releases/download/$GEOENGINE_VERSION/geoengine-$GEOENGINE_VERSION-linux-amd64.tar.gz

RUN addgroup -S geoengine && adduser -S -G geoengine geoengine

RUN apk update \
    && apk add ca-certificates \
    && update-ca-certificates \
    && apk add openssl \
    && wget -O geoengine.tar.gz "$GEOENGINE_DOWNLOAD_URL" \
    && tar -xzvf geoengine.tar.gz \
    && rm -f geoengine.tar.gz \
    && mv geoengine-$GEOENGINE_VERSION-linux-amd64/geoengine-server /usr/local/bin \
    && rm -fR geoengine-$GEOENGINE_VERSION-linux-amd64

RUN mkdir /data && chown geoengine:geoengine /data

VOLUME /data
WORKDIR /data

EXPOSE 9851
CMD ["geoengine-server"]