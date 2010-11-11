make:
	node-waf configure clean build; node tests/node-usb-test.js
