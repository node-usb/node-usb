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
	
	assert.ok((device.busNumber > 0), "busNumber must be larger than 0");
	assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0");
	var id = device.busNumber + ":" + device.deviceAddress;
	assert.ok((device.getDeviceDescriptor() != undefined), "getDeviceDescriptor() must return an object");
	assert.ok((device.getConfigDescriptor() != undefined), "getConfigDescriptor() must return an object");
//	assert.equal(device.close(), true, "close() must be true because device is opened by prior 
}
assert.ok(instance.close());

console.log("Tests were successful :-)");

