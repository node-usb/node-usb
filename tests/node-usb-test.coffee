{test, wait, next} = require('./test-helper.coffee')
assert = require('assert')
usb = require("../usb.js")

test "Basic constants must exist", -> 
	assert.notEqual(usb, undefined, "usb must be undefined")
	assert.ok((usb.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be defined")
	assert.ok((usb.LIBUSB_ENDPOINT_IN == 128))
	assert.notEqual(usb.revision, "unknown", "Revision should not unknown")

test "setDebugLevel must error with invalid args", ->
	assert.throws((-> usb.setDebugLevel()), TypeError)
	assert.throws((-> usb.setDebugLevel(-1)), TypeError)
	assert.throws((-> usb.setDebugLevel(4)), TypeError)

test "setDebugLevel must succeed with good args", ->
	assert.doesNotThrow(-> usb.setDebugLevel(0))

devices = null

test "getDevices works", ->
	devices = usb.getDevices()
	assert.notEqual(devices, undefined, "getDevices() must not be undefined")
	assert.ok((devices.length > 0), "getDevices() must be larger than 0 (assume that at least one host controller is available)")

assert_extra_length = (obj) ->
	r = obj.getExtraData()
	assert.ok((r.length == obj.extra_length), "getExtraLength() (length is: " + r.length + ") + must be equal to .extra_length (is: " + obj.extra_length + ")")


arr = null
device = null

test "Finding demo device", ->
	arr = usb.find_by_vid_and_pid(0x59e3, 0x0a23)
	assert.ok((arr != undefined), "usb.find_by_vid_and_pid() must return array")
	assert.ok((arr.length > 0), "usb.find_by_vid_and_pid() must return array with length > 0")
	device = arr[0]
	
test "Device properties are sane" , -> 
	assert.ok((device.busNumber > 0), "deviceAddress must be larger than 0")
	assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0")
	
test "deviceDescriptor must return an object", ->
	assert.ok(((deviceDesc = device.deviceDescriptor) != undefined))
	
test "getConfigDescriptor() must return an object", ->
	assert.ok(((deviceConfigDesc = device.getConfigDescriptor()) != undefined))

test "Invalid timeout is an error", ->
	assert.throws(-> device.timeout = 'foo')
	
test "Default timeout is 1000", ->
	assert.equal(device.timeout, 1000);
	
test "Changing timeout", ->
	device.timeout = 100;
	assert.equal(device.timeout, 100);

b = new Buffer([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])

test "Control transfer out", ->
	device.controlTransfer 0x40, 0x81, 0, 0, b, (d, e) ->
		console.log("ControlTransferOut", d, e)
		assert.ok(e == undefined, e)
		next()
	wait()
	
test "Control transfer fails when bmRequestType doesn't match buffer / length", ->
	assert.throws(-> device.controlTransfer(0x40, 0x81, 0, 0, 64))
	
test "Control transfer in", ->
	device.controlTransfer 0xc0, 0x81, 0, 0, 64, (d, e) ->
		console.log("ControlTransferIn", d, e)
		assert.ok(e == undefined, e)
		assert.equal(d.toString(), b.toString())
		next()
	wait()

interface = null
test "Get interface", ->
	interfaces = device.getInterfaces()
	interface = interfaces[0]
	assert.notEqual(interface, undefined, "Interface must be defined")

test "Claim interface", ->
	interface.claim()

endpoints = null
inEndpoint = null
outEndpoint = null

test "Get in endpoint", ->
	endpoints = interface.getEndpoints()
	inEndpoint = endpoints[0]
	assert.notEqual(inEndpoint, undefined, "Endpoint must be defined")
	assert.equal(inEndpoint.__endpointType, usb.LIBUSB_ENDPOINT_IN)

test "Attempt to write to IN endpoint", ->
	assert.throws -> inEndpoint.transfer(b)
	
test "Read from IN endpoint", ->
	inEndpoint.transfer 64, (d, e) ->
		console.log("BulkTransferIn", d, e)
		assert.ok(e == undefined, e)
		next()
	wait()
	
test "Get out endpoint", -> 
	outEndpoint = endpoints[1]
	assert.notEqual(outEndpoint, undefined, "Endpoint must be defined")
	assert.equal(outEndpoint.__endpointType, usb.LIBUSB_ENDPOINT_OUT)
	
test "Attempt to read from OUT endpoint", ->
	assert.throws -> outEndpoint.transfer(64)
	
test "Write to OUT endpoint", ->
	outEndpoint.transfer b, (d, e) ->
		console.log("BulkTransferIn", d, e)
		next()
	wait()
	
test "Stream from IN endpoint", ->
	pkts = 0
	
	inEndpoint.__stream_data_cb = (d, e) ->
		console.log("Stream callback", d, e)
		pkts++
		
		if pkts == 10
			inEndpoint.stopStream()
			console.log("Stopping stream")
			
	inEndpoint.__stream_stop_cb = () ->
		console.log("Stream stopped")
		next()
	
	inEndpoint.startStream 64, 30

	wait()
			

test "Complete!", ->

