FROM alpine:3.4

ADD ./geoengine-server /usr/local/bin
ADD ./geoengine-cli /usr/local/bin

RUN addgroup -S geoengine && \
    adduser -S -G geoengine geoengine && \
    mkdir /data && chown geoengine:geoengine /data

VOLUME /data

EXPOSE 9851
CMD ["geoengine-server", "-d", "/data"]
