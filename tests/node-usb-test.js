var assert = require('assert');
var usb_driver = require("../usb.js");

var instance = usb_driver.create()

assert.notEqual(instance, undefined, "instance must be undefined");
assert.ok((instance.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be defined");
assert.equal(instance.isLibusbInitalized, false, "isLibusbInitalized must be false");
assert.equal(instance.close(), false, "close() must be false because driver is not opened");

assert.ok(instance.refresh(), "refresh() must be true");
assert.ok(instance.isLibusbInitalized, "isLibusbInitalized must be true after refresh()");

var devices = instance.getDevices(); 
assert.notEqual(devices, undefined, "getDevices() must not be undefined");
assert.ok((devices.length > 0), "getDevices() must be larger than 0 (assume that at least one host controller is available)");

for (var i = 0; i < devices.length; i++) {
	var device = devices[i];
	console.log(device);
	assert.ok((device.busNumber > 0), "busNumber must be larger than 0");
	assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0");
	var id = device.busNumber + ":" + device.deviceAddress;
//	console.log("working on " + id);
	assert.equal(device.close(), false, "close() must be false because device is not opened");

	try {
		device.open();
		assert.ok(device.close());
	}
	catch (e) {
		console.log("failed to open device [" + id + "]: " + e.message);
	}

//	console.log(device.getDeviceDescriptor());
//	console.log(device.getConfigDescriptor());
}

assert.ok(instance.close());

console.log("Tests were successful :-)");

