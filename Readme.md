USB Library for Node.JS
===============================

**POSIX:** [![Build Status](https://travis-ci.org/nonolith/node-usb.svg?branch=tcr-usb)](https://travis-ci.org/nonolith/node-usb) &nbsp;&nbsp;&nbsp; **Windows:** [![Build status](https://ci.appveyor.com/api/projects/status/b23kn1pi386nguya/branch/master)](https://ci.appveyor.com/project/kevinmehall/node-usb/branch/master)

Node.JS library for communicating with USB devices in JavaScript / CoffeeScript.

This is a refactoring / rewrite of Christopher Klein's [node-usb](https://github.com/schakko/node-usb). The API is not compatible (hopefully you find it an improvement).

It's based entirely on libusb's asynchronous API for better efficiency, and provides a stream API for continuously streaming data or events.

Installation
============

Libusb is included as a submodule. On Linux, you'll need libudev to build libusb. On Ubuntu/Debian: `sudo apt-get install build-essential libudev-dev`

Then, just run

	npm install usb

to install from npm. See the bottom of this page for instructions for building from a git checkout.

### Windows
Use [Zadig](http://sourceforge.net/projects/libwdi/files/zadig/) to install the WinUSB driver for your USB device. Otherwise you will get `LIBUSB_ERROR_NOT_SUPPORTED` when attempting to open devices.


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

### .portNumbers
Array containing the USB device port numbers

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

### .setConfiguration(id, callback(error))
Set the device configuration to something other than the default (0). To use this, first call `.open(false)` (which tells it not to auto configure), then before claiming an interface, call this method.

### .getStringDescriptor(index, callback(error, data))
Perform a control transfer to retrieve a string descriptor

### .interface(interface)
Return the interface with the specified interface number.

### .interfaces
List of Interface objects for the interfaces of the default configuration of the device.

### .timeout
Timeout in milliseconds to use for control transfers.

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
Endpoint direction: `"in"` or `"out"`.

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

### .timeout
Sets the timeout in milliseconds for transfers on this endpoint. The default, `0`, is infinite timeout.

InEndpoint
----------

Endpoints in the IN direction (device->PC) have this type.

### .transfer(length, callback(error, data))
Perform a transfer to read data from the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback with all data once all packets are complete.

`this` in the callback is the InEndpoint object.

### .startPoll(nTransfers=3, transferSize=maxPacketSize)
Start polling the endpoint.

The library will keep `nTransfers` transfers of size `transferSize` pending in
the kernel at all times to ensure continuous data flow. This is handled by the
libusb event thread, so it continues even if the Node v8 thread is busy. The
`data` and `error` events are emitted as transfers complete.

### .stopPoll(cb)
Stop polling.

Further data may still be received. The `end` event is emitted and the callback
is called once all transfers have completed or canceled.

### Event: data(data : Buffer)
Emitted with data received by the polling transfers

### Event: error(error)
Emitted when polling encounters an error.

### Event: end
Emitted when polling has been canceled

OutEndpoint
-----------

Endpoints in the OUT direction (PC->device) have this type.

### .transfer(data, callback(error))
Perform a transfer to write `data` to the endpoint.

If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback once all packets are complete.

`this` in the callback is the OutEndpoint object.

### Event: error(error)
Emitted when the stream encounters an error.

### Event: end
Emitted when the stream has been stopped and all pending requests have been completed.


UsbDetection
------------

### usb.on('attach', function(device) { ... });
Attaches a callback to plugging in a `device`.

### usb.on('detach', function(device) { ... });
Attaches a callback to unplugging a `device`.


Development and testing
=======================

To build from git:

	git clone --recursive https://github.com/nonolith/node-usb.git
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

License
=======

MIT

Note that the compiled Node extension includes Libusb, and is thus subject to the LGPL.
