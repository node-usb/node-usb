assert = require('assert')
usb = require("../usb.js")

describe 'Module', ->
	it 'should describe basic constants', ->
		assert.notEqual(usb, undefined, "usb must be undefined")
		assert.ok((usb.LIBUSB_CLASS_PER_INTERFACE != undefined), "Constants must be described")
		assert.ok((usb.LIBUSB_ENDPOINT_IN == 128))

	it 'should have a revision number', ->
		assert.notEqual(usb.revision, "unknown", "Revision should not unknown")

	it 'should handle abuse without crashing', ->
		assert.throws -> new usb.Device()
		assert.throws -> usb.Device()
		assert.throws -> new usb.Endpoint(device, 100, 100, 100)
		assert.throws -> new usb.Interface(device, 100, 100, 100)
		assert.throws -> new usb.Endpoint(inEndpoint, 0, 0, 0)

describe 'setDebugLevel', ->
	it 'should throw when passed invalid args', ->
		assert.throws((-> usb.setDebugLevel()), TypeError)
		assert.throws((-> usb.setDebugLevel(-1)), TypeError)
		assert.throws((-> usb.setDebugLevel(4)), TypeError)

	it 'should succees with good args', ->
		assert.doesNotThrow(-> usb.setDebugLevel(0))

describe 'Device list', ->
	devices = null

	it 'should exist', ->
		assert.notEqual(usb.devices, undefined)

	it 'should have at least one device', ->
		#assume that at least one host controller is available
		assert.ok((usb.devices.length > 0))

	it 'should handle invalid indexes', ->
		assert.equal(usb.devices[1000000], undefined)

arr = null
device = null
b = new Buffer([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])
iface = null
inEndpoint = null
outEndpoint = null

describe 'find_by_vid_and_pid', ->
	it 'should return an array with length > 0', ->
		# Assume demo device is attached
		arr = usb.find_by_vid_and_pid(0x59e3, 0x0a23)
		assert.ok((arr != undefined), "usb.find_by_vid_and_pid() must return array")
		assert.ok((arr.length > 0), "usb.find_by_vid_and_pid() must return array with length > 0")
		device = arr[0]

describe 'Device', ->
	it 'should have sane properties', ->	
		assert.ok((device.busNumber > 0), "deviceAddress must be larger than 0")
		assert.ok((device.deviceAddress > 0), "deviceAddress must be larger than 0")

	it 'should have a deviceDescriptor property', ->
		assert.ok(((deviceDesc = device.deviceDescriptor) != undefined))

	it 'should have a configDescriptor property', ->
		assert.ok(((deviceConfigDesc = device.configDescriptor) != undefined))
		assert.ok(device.configDescriptor.extra.length == 0)

	describe 'timeout', ->
		it 'should fail with an invalid timeout', ->
			assert.throws(-> device.timeout = 'foo')

		it 'should default to 1000', ->
			assert.equal(device.timeout, 1000);

		it 'should be able to be changed', ->
			device.timeout = 100;
			assert.equal(device.timeout, 100);

	describe 'control transfer', ->
		it 'should OUT transfer when the IN bit is not set', (done) ->
			device.controlTransfer 0x40, 0x81, 0, 0, b, (d, e) ->
				#console.log("ControlTransferOut", d, e)
				assert.ok(e == undefined, e)
				done()

		it "should fail when bmRequestType doesn't match buffer / length", ->
			assert.throws(-> device.controlTransfer(0x40, 0x81, 0, 0, 64))

		it 'should IN transfer when the IN bit is set', (done) ->
			device.controlTransfer 0xc0, 0x81, 0, 0, 64, (d, e) ->
				#console.log("ControlTransferIn", d, e)
				assert.ok(e == undefined, e)
				assert.equal(d.toString(), b.toString())
				done()

	describe 'interface', ->
		it 'should have a list of interfaces', ->
			iface = device.interfaces[0]
			assert.notEqual(iface, undefined, "Interface must be described")
			assert.equal(iface, device.interface(0))

		it 'should be able to claim an interface', ->
			iface.claim()

	describe 'IN endpoint', ->
		it 'should be able to get the endpoint', ->
			inEndpoint = iface.endpoints[0]
			assert.ok inEndpoint?

		it 'should be able to get the endpoint by address', ->
			assert.equal(inEndpoint, iface.endpoint(0x81))

		it 'should have the IN direction flag', ->
			assert.equal(inEndpoint.direction, usb.LIBUSB_ENDPOINT_IN)

		it 'should fail to write', ->
			assert.throws -> inEndpoint.transfer(b)

		it 'should support read', (done) ->
			inEndpoint.transfer 64, (d, e) ->
				#console.log("BulkTransferIn", d, e)
				assert.ok(e == undefined, e)
				done()

		it 'should signal errors', (done) ->
			inEndpoint.transfer 1, (d, e) ->
				assert.equal e, usb.LIBUSB_TRANSFER_OVERFLOW
				assert.equal d, undefined
				done()

		it 'should be a readableStream', (done) ->
			pkts = 0
	
			inEndpoint.on 'data', (d) ->
				#console.log("Stream callback", d)
				pkts++
				
				if pkts == 10
					inEndpoint.stopStream()
					#console.log("Stopping stream")
					
			inEndpoint.on 'error', (e) ->
				#console.log("Stream error", e)
				assert.equal(e, 3)
					
			inEndpoint.on 'end', ->
				#console.log("Stream stopped")
				done()
			
			inEndpoint.startStream 4, 64

	describe 'OUT endpoint', ->
		it 'should be able to get the endpoint', ->
			outEndpoint = iface.endpoints[1]
			assert.ok outEndpoint?

		it 'should be able to get the endpoint by address', ->
			assert.equal(outEndpoint, iface.endpoint(0x02))

		it 'should have the OUT direction flag', ->
			assert.equal(outEndpoint.direction, usb.LIBUSB_ENDPOINT_OUT)

		it 'should fail to read', ->
			assert.throws -> outEndpoint.transfer(64)

		it 'should support write', (done) ->
			outEndpoint.transfer b, (d, e) ->
				#console.log("BulkTransferOut", d, e)
				done()

		it 'should be a writableStream', (done)->
			pkts = 0

			outEndpoint.on 'drain', ->
				#console.log("Stream callback")
				pkts++

				outEndpoint.write(new Buffer(64));

				if pkts == 10
					outEndpoint.stopStream()
					#console.log("Stopping stream")

			outEndpoint.on 'error', (e) ->
				#console.log("Stream error", e)

			outEndpoint.on 'end', ->
				#console.log("Stream stopped")
				done()

			outEndpoint.startStream 4, 64