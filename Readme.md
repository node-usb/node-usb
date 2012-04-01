libusb-1.0 bindings for Node.js
===============================
node-usb binds libusb-1.0 to Node.js 

Tested with Node version 0.6.6/Linux.
Brew/OSX port of libusb-1.0 has still an issue which results in a segfault. Try using the latest development branch from http://git.libusb.org

Installation
============
Make sure you have installed libusb-1.0-0-dev (Ubuntu: sudo apt-get install libusb-1.0-0-dev).

Just run

	make

in current directory and wait. "Unit tests" are automatically executed by default target.
Failed unit tests do *not* mean that node-usb is not working. It depends on your USB environment that all tests pass.
If they fail, just ignore them.

You can although execute the tests by typing:

	node tests/node-usb-test.js

If you want to use the USB vendor ids, execute

	make create-usb-ids

For creating debug output you have to compile node-usb with

	make debug

or

	node-waf clean configure --debug=true build

API
===
returned values can always be an error object, so make sure that no error occurs!
The device/interface/endpoint descriptor values are always injected into the proper object. I do not list them.

Usb
---
	.LIBUSB_* : const
		constant properties from libusb
	.isLibusbInitalized : boolean
		property for getting libusb initalization status
	.refresh() : undefined
		refreshes the node-usb environment
	.setDebugLevel(int _level) : undefined
		wrapper for libusb_set_debug; _level has to be between 0 and 4
	.getDevices() : Device[]
		returns the devices discovered by libusb
	.close() : undefined
		closes node-usb

	Device
	.deviceAddress : int
		property for getting device address
	.busNumber : int
		property for getting bus number
	.reset(function _afterReset) : undefined
		[async] resets device, expects callback function after reset
	.getConfigDescriptor() : Object
		returns configuration descriptor of device
	.getExtraData() : Array
		returns byte array with extra data from config descriptor
	.getInterfaces() : Interface[]
		returns interfaces
	.getDeviceDescriptor() : Object
		returns devices descriptor
	.controlTransfer(mixed read|write, int _bmRequestType, int _bRequest, int _wValue, int _wIndex, function afterTransfer(data) [, int _timeout])
		[async] delegates to libusb_control_transfer. First parameter can be either
		  * int, then controlTransfer works in read mode (read data FROM USB device)
		  * Buffer object, then controlTransfer works in write mode (write data TO USB device)
		_timeout parameter is optional.
		afterTransfer contains a Buffer object with the data read from device.

Interface
---------

	.__idxInterface : int
		property for internal interface index
	.__idxAltSetting : int
		property for internal alternate interface setting
	.getEndpoints() : Endpoint[]
		returns an array of Endpoint objects
	.detachKernelDriver() : undefined
		detaches the kernel driver from interface
	.attachKernelDriver() : undefined
		re-attaches the kernel driver to interface
	.claim() : undefined
		claims the interface. this method must be __always__ called if you want to transfer data via the endpoint. There is no hig level API check for this
	.release(function afterClaim(data)) : undefined
		[async] releases prior claimed interface; function wil be called after releasing
	.setAlternateSetting(int _alternateSettingIdx, function afterAlternateSettingChosen(data))
		[async] sets alternate setting of interface; first parameter is the alternate setting index (there is no check for a valid index!), seconds is the callback function which is called after alternate setting has been chosen	
	.isKernelDriverActive() : int
		returns 0  if not active; 1 if active
	.getExtraData() : Array
		returns byte array with extra data from interface descriptor

Endpoint
--------

	.__endpointType : int
		property for getting the endpoint type (IN/OUT), check with Usb.LIBUSB_ENDPOINT_TYPE constants
	.__transferType : int
		property for getting the transfer type (bulk, interrupt, isochronous); check with Usb.LIBUSB_TRANSFER_* constants
	.__maxIsoPacketSize : int
		property for getting the max isochronous packet size if transfer type is isochronous
	.__maxPacketSize : int
		property for getting the max packet size
	.getExtraData() : Array
		byte array with extra data from endpoint descriptor
	.bulkTransfer(mixed read|write, function afterTransfer(data) [, int _timeout])
	.interruptTransfer(mixed read|write, function afterTransfer(data) [, int _timeout]) : undefined
		[async] bulkTransfer and interruptTransfer are more or less equal. If endpoint works in bulk mode, you use bulkTransfer(), if endpoint work in interrupt mode, you use interruptTransfer(). If you use the wrong method, the function call will fail.
		First parameter can be either
		* int, then the function will work in read mode (read _int_ bytes FROM USB device)
		* Buffer object, the function will work in write mode (write buffer TO USB device)
		The _timeout parameter is optional; if not used, an unlimited timeout is used.
		Both functions are working in asynchronous mode, the second parameter is the callback handler after the function has been executed / data arrived.
		afterTransfer contains a Buffer object as first parameter
	.submitNative(mixed read|write, function after(data) [, int _timeout, int _iso_packets, int _flags]) : undefined
		[async] bulkTransfer, controlTransfer and interruptTransfer are using the EV library for threading. submitNative() uses the libusb asynchronous interface. This is not really tested now.
		submitNative() will be the only function to handle isochronous transfer mode and detects the endpoint type automatically.
		First parameter can be either (you know it already):
		* int, the function will be work in read mode (read _int_ bytes FROM USB device)
		* Buffer object, the function will work in write mode (write byte array TO USB device)
		The parameter _iso_packets and _flags can be used, if the endpoint is working in isochronous mode otherwise they are useless. 

Examples
========
A simple port of lsusb can be executed by typing

	node examples/lsusb/lsusb.js

If you own a Microsoft Kinect, you can manipulate the LED, set the angle of the device and read the axis of the accelerometer

	node examples/node-usb-kinect.js

Browse to localhost:8080 and see the magic.


If you have permission issues, you have to add the proper rights to the USB device or execute the examples as root (NOT RECOMMENDED!)

More information
=================
 * Christopher Klein <schakkonator[at]googlemail[dot]com>
 * http://twitter.com/schakko
 * http://wap.ecw.de
