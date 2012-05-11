make:
	node-waf -v configure clean build

debug:
	node-waf configure --debug=true clean build
	coffee tests/node-usb-test.coffee

create-usb-ids:
	rm usb_ids.js;	node util/create_usb_ids.js >> usb_ids.js
