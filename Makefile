all: 
	@./build.sh
clean:
	rm -f geoengine-server
	rm -f geoengine-cli
test:
	@./build.sh test
install: all
	cp geoengine-server /usr/local/bin
	cp geoengine-cli /usr/local/bin
uninstall: 
	rm -f /usr/local/bin/geoengine-server
	rm -f /usr/local/bin/geoengine-cli
