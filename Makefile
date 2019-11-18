all: geoengine-server geoengine-cli geoengine-benchmark geoengine-luamemtest

.PHONY: geoengine-server
geoengine-server:
	@./scripts/build.sh geoengine-server

.PHONY: geoengine-cli
geoengine-cli:
	@./scripts/build.sh geoengine-cli

.PHONY: geoengine-benchmark
geoengine-benchmark:
	@./scripts/build.sh geoengine-benchmark

.PHONY: geoengine-luamemtest
geoengine-luamemtest:
	@./scripts/build.sh geoengine-luamemtest

test: all
	@./scripts/test.sh

package:
	@rm -rf packages/
	@scripts/package.sh Windows windows amd64
	@scripts/package.sh Mac     darwin  amd64
	@scripts/package.sh Linux   linux   amd64
	@scripts/package.sh FreeBSD freebsd amd64
	@scripts/package.sh ARM     linux   arm
	@scripts/package.sh ARM64   linux   arm64

clean:
	rm -rf geoengine-server geoengine-cli geoengine-benchmark geoengine-luamemtest 

distclean: clean
	rm -rf packages/

install: all
	cp geoengine-server /usr/local/bin
	cp geoengine-cli /usr/local/bin
	cp geoengine-benchmark /usr/local/bin

uninstall: 
	rm -f /usr/local/bin/geoengine-server
	rm -f /usr/local/bin/geoengine-cli
	rm -f /usr/local/bin/geoengine-benchmark
