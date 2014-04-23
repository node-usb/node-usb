USB Library for Node.JS
===============================

**POSIX:** [![Build Status](https://travis-ci.org/nonolith/node-usb.svg?branch=tcr-usb)](https://travis-ci.org/nonolith/node-usb) &nbsp;&nbsp;&nbsp; **Windows:** [![Build status](https://ci.appveyor.com/api/projects/status/1q64jnnfseidkjwl)](https://ci.appveyor.com/project/tcr/node-usb)

Node.JS library for communicating with USB devices in JavaScript / CoffeeScript.

This is a refactoring / rewrite of Christopher Klein's [node-usb](https://github.com/schakko/node-usb). The API is not compatible (hopefully you find it an improvement).

It's based entirely on libusb's asynchronous API for better efficiency, and provides a stream API for continuously streaming data or events.

Tested with Node 0.10/Fedora and Node 0.10/WinXP.

Installation
============

Libusb is required. Older versions of libusb segfault when using bulk or interrupt endpoints.
Use [libusb](http://libusb.org) or [libusbx](http://libusbx.org) 1.0.9 or greater.

**Ubuntu/Debian:** `sudo apt-get install build-essential pkg-config libusb-1.0-0-dev`  
**Fedora:** `sudo yum install libusbx-devel`

**OSX:** `brew install libusb pkg-config`

**Windows:** Download a Windows Binary package from http://libusbx.org/ and extract it at `C:\Program Files\libusb`. Use [Zadig](http://sourceforge.net/projects/libwdi/files/zadig/) to install the WinUSB driver for your USB device. Otherwise you will get a `LIBUSB_ERROR_NOT_SUPPORTED` when attempting to open devices.

Then, just run

	npm install usb

to install from npm. See the bottom of this page for instructions for building from a git checkout.

API
===

  var usb = require('usb')

usb
---

Top-level object.

### usb.getDeviceList()
Return a list of `Device` objects for the USB devices attached to the system.

### usb.findByIds(vid, pid)
Convenience method to get the first device with the specified VID and PID, or `undefined` if no such device is present.

### usb.LIBUSB_*
Constant properties from libusb

### usb.setDebugLevel(level : int)
Set the libusb debug level (between 0 and 4)

Device
------

Represents a USB device.

### .busNumber
Integer USB device number

### .deviceAddress
Integer USB device address

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
Object with properties for the fields of the configuration descriptor:

  - bLength
  - bDescriptorType
  - wTotalLength
  - bNumInterfaces
  - bConfigurationValue
  - iConfiguration
  - bmAttributes
  - bMaxPower
  - extra (Buffer containing any extra data or additional descriptors)

### .open()

Open the device. All methods below require the device to be open before use.

### .close()

Close the device.

### .controlTransfer(bmRequestType, bRequest, wValue, wIndex, data_or_length, callback(error, data))

Perform a control transfer with `libusb_control_transfer`.

Parameter `data_or_length` can be a integer length for an IN transfer, or a Buffer for an out transfer. The type must match the direction specified in the MSB of bmRequestType.

The `data` parameter of the callback is always undefined for OUT transfers, or will be passed a Buffer for IN transfers.

### .getStringDescriptor(index, callback(error, data))
Perform a control transfer to retrieve a string descriptor

### .interface(interface)
Return the interface with the specified interface number.

### .interfaces
List of Interface objects for the interfaces of the default configuration of the device.

### .timeout
Timeout in milliseconds to use for controlTransfer and endpoint transfers.

### .reset(callback(error))
Performs a reset of the device. Callback is called when complete.


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

### .setAltSetting(altSetting, callback(error))
Sets the alternate setting. It updates the `interface.endpoints` array to reflect the endpoints found in the alternate setting.

### .claim()
Claims the interface. This method must be called before using any endpoints of this interface.

### .release([closeEndpoints], callback(error))
Releases the interface and resets the alternate setting. Calls callback when complete.

It is an error to release an interface with pending transfers. If the optional closeEndpoints parameter is true, any active endpoint streams are stopped (see `Endpoint.stopStream`), and the interface is released after the stream transfers are cancelled. Transfers submitted individually with `Endpoint.transfer` are not affected by this parameter.

### .isKernelDriverActive()
Returns `false` if a kernel driver is not active; `true` if active.

### .detachKernelDriver()
Detaches the kernel driver from the interface.

### .attachKernelDriver()
Re-attaches the kernel driver for the interface.

### .descriptor
Object with fields from the interface descriptor -- see libusb documentation or USB spec.

  - bLength
  - bDescriptorType
  - bInterfaceNumber
  - bAlternateSetting
  - bNumEndpoints
  - bInterfaceClass
  - bInterfaceSubClass
  - bInterfaceProtocol
  - iInterface
  - extra (Buffer containing any extra data or additional descriptors)

Endpoint
--------

Common base for InEndpoint and OutEndpoint, see below.

### .direction
Endpoint direction: `usb.LIBUSB_ENDPOINT_IN` or `usb.LIBUSB_ENDPOINT_OUT`.

### .transferType
Endpoint type: `usb.LIBUSB_TRANSFER_TYPE_BULK`, `usb.LIBUSB_TRANSFER_TYPE_INTERRUPT`, or `usb.LIBUSB_TRANSFER_TYPE_ISOCHRONOUS`.

###  .descriptor
Object with fields from the endpoint descriptor -- see libusb documentation or USB spec.

  - bLength
  - bDescriptorType
  - bEndpointAddress
  - bmAttributes
  - wMaxPacketSize
  - bInterval
  - bRefresh
  - bSynchAddress
  - extra (Buffer containing any extra data or additional descriptors)

InEndpoint
----------

Endpoints in the IN direction (device->PC) have this type.

### .transfer(length, callback(error, data))
Perform a transfer to read data from the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback with all data once all packets are complete.

`this` in the callback is the InEndpoint object.

### .startStream(nTransfers=3, transferSize=maxPacketSize)
Start a streaming transfer from the endpoint.

The library will keep `nTransfers`
transfers of size `transferSize` pending in the kernel at all times to ensure
continuous data flow. This is handled by the libusb event thread, so it continues even
if the Node v8 thread is busy. The `data` and `error` events are emitted as transfers complete.

### .stopStream()
Stop the streaming transfer.

Further data may still be received. The `end` event is emitted once all transfers have completed or canceled.

### Event: data(data : Buffer)
Emitted with data received by the stream

### Event: error(error)
Emitted when the stream encounters an error.

### Event: end
Emitted when the stream has been canceled

OutEndpoint
-----------

Endpoints in the OUT direction (PC->device) have this type.

### .transfer(data, callback(error))
Perform a transfer to write `data` to the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback once all packets are complete.

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

No further `drain` events will be emitted. When all transfers have been completed, the OutEndpoint emits the `end` event.

### Event: drain
Emitted when the stream requests more data. Use the .write() method to start another transfer.

### Event: error(error)
Emitted when the stream encounters an error.

### Event: end
Emitted when the stream has been stopped and all pending requests have been completed.


Development and testing
=======================

To build from git:

	git clone https://github.com/nonolith/node-usb.git
	cd node-usb
	npm install

To execute the unit tests, [CoffeeScript](http://coffeescript.org) is required. Run

	npm test

Some tests require an attached USB device -- firmware to be released soon.

Limitations
===========

Does not support:

  - Configurations other than the default one
  - Isochronous transfers
