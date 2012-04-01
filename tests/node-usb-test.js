var assert = require('assert')
var usb_driver = require("../usb.js")

var instance = usb_driver.create()

console.log("---- Failed tests does NOT mean, node-usb is working. It depends on your USB infrastructure that all tests pass. Try the examples/kinect/kinect.js for proper working")
assert.notEqual(instance, undefined, "instance must be undefined")
assert.ok((instance.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be defined")
assert.ok((instance.LIBUSB_ENDPOINT_IN == 128))
assert.notEqual(instance.revision, "unknown", "Revision should not unknown")
assert.equal(instance.isLibusbInitalized, false, "isLibusbInitalized must be false")
assert.equal(instance.close(), false, "close() must be false because driver is not opened")

assert.ok(instance.refresh(), "refresh() must be true")
assert.ok(instance.isLibusbInitalized, "isLibusbInitalized must be true after refresh()")

assert.throws(function() { instance.setDebugLevel(); }, TypeError)
assert.throws(function() { instance.setDebugLevel(-1); }, TypeError)
assert.throws(function() { instance.setDebugLevel(4); }, TypeError)
assert.doesNotThrow(function() { instance.setDebugLevel(0)})

var devices = instance.get_devices()
assert.notEqual(devices, undefined, "getDevices() must not be undefined")
assert.ok((devices.length > 0), "getDevices() must be larger than 0 (assume that at least one host controller is available)")

function assert_extra_length(obj) {
	var r = obj.getExtraData()
	assert.ok((r.length == obj.extra_length), "getExtraLength() (length is: " + r.length + ") + must be equal to .extra_length (is: " + obj.extra_length + ")")
}

for (var i = 0; i < devices.length; i++) {
	var device = devices[i]
	var deviceDesc = undefined
	var deviceConfigDesc = undefined
	assert.ok((device.busNumber > 0), "busNumber must be larger than 0")
	assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0")
	var id = device.busNumber + ":" + device.deviceAddress
	assert.ok(((deviceDesc = device.getDeviceDescriptor()) != undefined), "getDeviceDescriptor() must return an object")
	assert.ok(((deviceConfigDesc = device.getConfigDescriptor()) != undefined), "getConfigDescriptor() must return an object")

	var arr = instance.find_by_vid_and_pid(deviceDesc.idVendor, deviceDesc.idProduct)
	assert.ok((arr != undefined), "usb.find_by_vid_and_pid() must return array")
	assert.ok((arr.length > 0), "usb.find_by_vid_and_pid() must return array with length > 0")
	var found = false

	for (var j = 0; j < arr.length; j++) {
		if ((arr[j].deviceAddress == device.deviceAddress) && (arr[j].busNumber == device.busNumber)) {
			found = true
			break
		}
	}

	assert.ok(found, "could not find USB interface with find_by_vid_and_pid with equal busNumber and deviceAddress")
	assert_extra_length(device)

	var interfaces = device.getInterfaces()

	assert.ok((interfaces != undefined), "Device.getInterfaces() must return an array")
	assert.ok((interfaces.length >= 0), "Device.getInterfaces() must return an array with length >= 0")

	for (var j = 0; j < interfaces.length; j++) {
		var interface = interfaces[j]
		assert_extra_length(interface)

		var endpoints = interface.getEndpoints()
		assert.ok((endpoints != undefined), "Device.getEndpoints() must return an array")
		assert.ok((endpoints.length >= 0), "Device.getEndpoints() must return an array with length >= 0")
		
		// if we do not claim the interface, we'll get some pthrad_mutex_lock errors from libusb/glibc
		var r = interface.claim()

		if (r != undefined) {
			console.log("Failed to claim endpoint; error: " + r.errno)
			continue
		}

		for (k = 0; k < endpoints.length; k++) {
			var endpoint = endpoints[k]
			assert_extra_length(endpoint)
		
			assert.throws(function() { endpoint.submitNative()})
			assert.throws(function() { endpoint.submitNative(1,2)})

			if (endpoint.__endpointType == instance.LIBUSB_ENDPOINT_OUT) {
				// wrong usage of endpoint. Endpoint is OUT, usage is IN
				assert.throws(function() { endpoint.submitNative(100, function(_stat) {}, 0, 0)})
				var param = new Buffer(new Array(0x01))
				assert.ok(endpoint.submitNative(param, function (_stat, _data) {}, 10, 10))

			}
			else {
				// endoint is IN, usage is OUT
				var param = new Buffer(new Array(0x01));
				assert.throws(function() { endpoint.submitNative(param, function (_stat, _data) {}, 0, 0)})
				assert.ok(endpoint.submitNative(100, function (stat, _data) {}, 10, 10))
			}
		}
	}
//	assert.equal(device.close(), true, "close() must be true because device is opened by prior 
}

assert.ok(instance.close())

/*
endpoint.write(new Buffer(0x01, 0x01), function(status) {
});

endpoint.read(100./^bytes$/, function(status, bytes) {
});
*/

console.log("Tests were successful :-)")

