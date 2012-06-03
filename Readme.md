USB Library for Node.JS
===============================
`usb.js` is a Node.JS library for communicating with USB devices in JavaScript / CoffeeScript.

This library is based on Christopher Klein's [node-usb](https://github.com/schakko/node-usb),
but the API is not compatible (hopefully you find it an improvement). Compared to node-usb,
it's based entirely on libusb's asynchronous API for better efficiency, and provides a stream API
for continuously streaming data.

Tested with Node version 0.6.12/Linux.
Older versions of libusb segfault when using bulk or interrupt endpoints.
Use [libusb](http://libusb.org)  or [libusbx](http://libusbx.org) 1.0.9 or greater.

Installation
============
Make sure you have installed libusb-1.0-0-dev (Ubuntu: sudo apt-get install libusb-1.0-0-dev).

Just run

	make

in the git checkout.

If you want to use the USB vendor ids, execute

	make create-usb-ids

API
===

    var usb = require('./usb')

usb
---

Top-level object.

### .devices
List of Device objects for the USB devices attached to the system.

### .LIBUSB_*
Constant properties from libusb

### .setDebugLevel(level : int)
Set the libusb debug level (between 0 and 4)


Device
------

Represents a USB device.

### .controlTransfer(bmRequestType, bRequest, wValue, wIndex, data_or_length, callback(data, error))

Perform a control transfer with `libusb_control_transfer`.

Parameter `data_or_length` can be a integer length for an IN transfer, or a Buffer for an out transfer. The type must match the direction specified in the MSB of bmRequestType.

The `data` parameter of the callback is always undefined for OUT transfers, or will be passed a Buffer for IN transfers.

### .interface(interface=0, altsetting=0)
Return the interface with the specified index and alternate setting.

### .interfaces
List of Interface objects for the interfaces of the default configuration of the device.

### .timeout
(property getter/setter) Timeout in milliseconds to use for controlTransfer and endpoint transfers.

### .deviceAddress
Integer USB device address

### .busNumber
Integer USB device number

### .reset(callback)
Performs a reset of the device. Callback is called when complete.

### .deviceDescriptor
Object with properties for the fields of the device descriptor:

  - bLength
  - bDescriptorType
  - bcdUSB
  - bDeviceClass
  - bDeviceSubClass
  - bDeviceProtocol
  - bMaxPacketSize0
  - idVendor
  - idProduct
  - bcdDevice
  - iManufacturer
  - iProduct
  - iSerialNumber
  - bNumConfigurations

### .configDescriptor
Object with properties for the fields of the config descriptor:

  - bLength
  - bDescriptorType
  - wTotalLength
  - bNumInterfaces
  - bConfigurationValue
  - iConfiguration
  - bmAttributes
  - MaxPower
  - extra: Buffer containing additional data

Interface
---------


### .endpoint(address)
Return the InEndpoint or OutEndpoint with the specified address.

### .endpoints
List of endpoints on this interface: InEndpoint and OutEndpoint objects.

### .interface
Integer interface number.

### .altSetting
Integer alternate setting number.

### .claim()
Claims the interface. This method must be called before using any endpoints of this interface.

### .release(callback)
Releases the interface and resets the alternate setting. Calls callback when complete.

### .detachKernelDriver()
Detaches the kernel driver from interface.

### .attachKernelDriver() : undefined
Re-attaches the kernel driver to interface.

### .isKernelDriverActive()
Returns 0  if not active; 1 if active.

### Descriptor attributes
Fields from the interface descriptor, see libusb documentation or USB spec.

  - bLength
  - bDescriptorType
  - bInterfaceNumber
  - bAlternateSetting
  - bNumEndpoints
  - bInterfaceClass
  - bInterfaceSubClass
  - bInterfaceProtocol
  - iInterface
  - extraData : Buffer

Endpoint
--------

Common base for InEndpoint and OutEndpoint, see below.

### .direction
Endpoint direction: `usb.LIBUSB_ENDPOINT_IN` or `usb.LIBUSB_ENDPOINT_OUT`.

### .transferType
Endpoint type: `usb.LIBUSB_TRANSFER_BULK`, `usb.LIBUSB_TRANSFER_INTERRUPT`, or `usb.LIBUSB_TRANSFER_ISOCHRONOUS`.

### .maxPacketSize
The max packet size.

### .maxIsoPacketSize
The max isochronous packet size if transfer type is isochronous.

###  Descriptor attributes
Fields from the interface descriptor, see libusb documentation or USB spec.

  - bLength
  - bDescriptorType
  - bEndpointAddress
  - bmAttributes
  - wMaxPacketSize
  - bInterval
  - bRefresh
  - bSynchAddress
  - extra_length	
  - extraData : Buffer

InEndpoint
----------

Endpoints in the IN direction (device->PC) have this type.

### .transfer(length, callback(data, error))
Perform a transfer to read data from the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multple packets,
and you will receive one callback with all data once all packets are complete.

Callback first parameter is `data`, just like OutEndpoint, but will always be undefined as no data is returned.

`this` in the callback is the OutEndpoint object.

### .startStream(nTransfers=3, transferSize=maxPacketSize)
Start a streaming transfer from the endpoint.

The library will keep `nTransfers`
transfers of size `transferSize` pending in the kernel at all times to ensure
continuous data flow. This is handled by the libusb event thread, so it continues even
if the Node v8 thread is busy. The `data` and `error` events are emitted as transfers complete.

### .stopStream()
Stop the streaming transfer.

Further data may still be received. The `end` event is emitted once all transfers have completed or cancelled.

### Event: data(data : Buffer)
Emitted with data received by the stream

### Event: error(error)
Emitted when the stream encounters an error.

### Event: end
Emitted when the stream has been cancelled

OutEndpoint
-----------

Endpoints in the OUT direction (PC->device) have this type.

### .transfer(data, callback)
Perform a transfer to write `data` to the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multple packets,
and you will receive one callback with all data once all packets are complete.

`this` in the callback is the OutEndpoint object.

### .startStream(nTransfers=3, transferSize=maxPacketSize)
Start a streaming transfer to the endpoint.

The library will help you maintain `nTransfers` transfers pending in the kernel to ensure continuous data flow.
The `drain` event is emitted when another transfer is necessary. Your `drain` handler should use the .write() method
to start another transfer.

### .write(data)
Write `data` to the endpoint with the stream. `data` should be a buffer of length `transferSize` as passed to startStream.

Delegates to .transfer(), but differs in that it updates the stream state tracking the number of requests in flight.

### .stopStream()
Stop the streaming transfer.

No further `drain` events will be emitted. When all transfers have been completed, the OutEndpoint emits the `drain` event.

### Event: drain
Emitted when the stream requests more data. Use the .write() method to start another transfer.

### Event: error(error)
Emitted when the stream encounters an error.

### Event: end
Emitted when the stream has been stopped and all pending requests have been completed.


Development and testing
=======================

To execute the unit tests, [CoffeeScript](http://coffeescript.org) is required. Run

	coffee tests/node-usb-test.coffee


For creating debug output you have to compile usb.js with

	make debug

or

	node-waf clean configure --debug=true build

Limitations
===========

Does not support:

  - Configurations other than the default one
  - Isochronous transfers
