var assert = require('assert')
var usb = require("../usb.js")

assert.notEqual(usb, undefined, "usb must be undefined")
assert.ok((usb.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be defined")
assert.ok((usb.LIBUSB_ENDPOINT_IN == 128))
assert.notEqual(usb.revision, "unknown", "Revision should not unknown")

assert.throws(function() { usb.setDebugLevel(); }, TypeError)
assert.throws(function() { usb.setDebugLevel(-1); }, TypeError)
assert.throws(function() { usb.setDebugLevel(4); }, TypeError)
assert.doesNotThrow(function() { usb.setDebugLevel(0)})

var devices = usb.getDevices()
assert.notEqual(devices, undefined, "getDevices() must not be undefined")
assert.ok((devices.length > 0), "getDevices() must be larger than 0 (assume that at least one host controller is available)")

function assert_extra_length(obj) {
	var r = obj.getExtraData()
	assert.ok((r.length == obj.extra_length), "getExtraLength() (length is: " + r.length + ") + must be equal to .extra_length (is: " + obj.extra_length + ")")
}


var arr = usb.find_by_vid_and_pid(0x59e3, 0x0a23)
assert.ok((arr != undefined), "usb.find_by_vid_and_pid() must return array")
assert.ok((arr.length > 0), "usb.find_by_vid_and_pid() must return array with length > 0")

var device = arr[0];

assert.ok((device.busNumber > 0), "busNumber must be larger than 0")
assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0")
var id = device.busNumber + ":" + device.deviceAddress
assert.ok(((deviceDesc = device.deviceDescriptor) != undefined), "deviceDescriptor must return an object")
assert.ok(((deviceConfigDesc = device.getConfigDescriptor()) != undefined), "getConfigDescriptor() must return an object")

var transfer = device.controlTransfer(0xc0, 0x20, 0, 0, 64, 100, function(x, y){console.log("callback", x, y)})
console.log("past control transfer");

/*

var interfaces = device.getInterfaces()

assert.ok((interfaces != undefined), "Device.getInterfaces() must return an array")
assert.ok((interfaces.length >= 0), "Device.getInterfaces() must return an array with length >= 0")

var interface = interfaces[0];

var endpoints = interface.getEndpoints()
assert.ok((endpoints != undefined), "Device.getEndpoints() must return an array")
assert.ok((endpoints.length >= 0), "Device.getEndpoints() must return an array with length >= 0")
	
// if we do not claim the interface, we'll get some pthrad_mutex_lock errors from libusb/glibc
var r = interface.claim()

if (r != undefined) {
	console.log("Failed to claim endpoint; error: " + r.errno)
}

var inEndpoint = endpoints[0];

assert.ok(inEndpoint.__endpointType == usb.LIBUSB_ENDPOINT_IN);
assert.throws(function() { inEnpoint.transfer(new Buffer([0x01]), 100, function (_stat, _data) {}, 0, 0)})
assert.ok(inEndpoint.transfer(64, 100, function (stat, _data) {}))

assert.ok(device.close())

console.log("Tests were successful :-)")*/

