var assert = require('assert');
var usb_driver = require("../usb.js");

var instance = usb_driver.create()

assert.notEqual(instance, undefined, "instance must be undefined");
assert.ok((instance.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be defined");
assert.notEqual(instance.revision, "unknown", "Revision should not unknown");
assert.equal(instance.isLibusbInitalized, false, "isLibusbInitalized must be false");
assert.equal(instance.close(), false, "close() must be false because driver is not opened");

assert.ok(instance.refresh(), "refresh() must be true");
assert.ok(instance.isLibusbInitalized, "isLibusbInitalized must be true after refresh()");

assert.throws(function() { instance.setDebugLevel(); }, TypeError);
assert.throws(function() { instance.setDebugLevel(-1); }, TypeError);
assert.throws(function() { instance.setDebugLevel(4); }, TypeError);
assert.doesNotThrow(function() { instance.setDebugLevel(0); });

var devices = instance.getDevices(); 
assert.notEqual(devices, undefined, "getDevices() must not be undefined");
assert.ok((devices.length > 0), "getDevices() must be larger than 0 (assume that at least one host controller is available)");

for (var i = 0; i < devices.length; i++) {
	var device = devices[i];
	var deviceDesc = undefined;
	var deviceConfigDesc = undefined;
	assert.ok((device.busNumber > 0), "busNumber must be larger than 0");
	assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0");
	var id = device.busNumber + ":" + device.deviceAddress;
	assert.ok(((deviceDesc = device.getDeviceDescriptor()) != undefined), "getDeviceDescriptor() must return an object");
	assert.ok(((deviceConfigDesc = device.getConfigDescriptor()) != undefined), "getConfigDescriptor() must return an object");
	
	if (i == 0) {
		device.reset(function(e) {	console.log(e); console.log(e.error); });
	}

	var arr = instance.find_by_vid_and_pid(deviceDesc.idVendor, deviceDesc.idProduct);
	assert.ok((arr != undefined), "usb.find_by_vid_and_pid() must return array");
	assert.ok((arr.length > 0), "usb.find_by_vid_and_pid() must return array with length > 0");
	var found = false;
	for (var j = 0; j < arr.length; j++) {
		if ((arr[j].deviceAddress == device.deviceAddress) && (arr[j].busNumber == device.busNumber)) {
			found = true;
			break;
		}
	}
	console.log(deviceConfigDesc);	
	assert.ok(found, "could not find USB interface with find_by_vid_and_pid with equal busNumber and deviceAddress");

//	assert.equal(device.close(), true, "close() must be true because device is opened by prior 
}
assert.ok(instance.close());

/*
endpoint.write(new array(0x01, 0x01), function(status) {
});

endpoint.read(100./^bytes$/, function(status, bytes) {
});
*/

console.log("Tests were successful :-)");

