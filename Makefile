make:
	node-waf configure clean build; node tests/node-usb-test.js

create-usb-ids:
	rm usb_ids.js;	node util/create_usb_ids.js >> usb_ids.js
