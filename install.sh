#!/usr/bin/env sh

node-pre-gyp install 2> /dev/null || \
	([ -d "libusb" ] || git clone --depth=1 https://github.com/kevinmehall/libusb.git) && \
	node-pre-gyp rebuild
