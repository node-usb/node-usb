var usb_driver = require("../usb.js");
var usb_ids = require("../usb_ids.js");
var instance = usb_driver.create()
var devices = instance.getDevices(); 

function toHex(i) {
	return Number(i).toString(16);
}

function toByteArray(_i) {
	var byteString = Number(_i).toString(2);
	var array = new Array();
	var bitCounter = 0;
	var byte = "";

	for (var i = (byteString.length - 1); i >= 0; i--) {
		byte = byteString[i] + byte; 
		bitCounter++;

		if ((bitCounter == 8) || ((i - 1) < 0)) {
			array.push(parseInt(byte, 2));
			byte = "";
			bitCounter = 0;
		}
	}

	return array.reverse();
}


function pad(v, len, fill) {
	var s = v.toString();
	for (var i = s.length; i < len; i++) {
		s = fill + s;
 	} 
	
	return s;
}

for (var i = 0; i < devices.length; i++) {
	var device = devices[i];
	var dd = device.getDeviceDescriptor();

	console.log("Bus " + pad(device.busNumber, 3, "0") + " Device " + pad(device.deviceAddress, 3, "0") + " ID " + pad(toHex(dd.idVendor), 4, "0") + ":" + pad(toHex(dd.idProduct), 4, "0"));
	console.log("Device Descriptor:");
	console.log("  bLength                       " + dd.bLength);
	console.log("  bDescriptorType               " + dd.bDescriptorType);
	// 2 bcdUSB: 2 bytes
	var bcdUSBArr = toByteArray(dd.bcdUSB);
	console.log("  bcdUSB                        " + bcdUSBArr[0] + "." + pad(bcdUSBArr[1], 2, "0"));
	console.log("  bDeviceClass                  " + dd.bDeviceClass);
	console.log("  bDeviceSubClass               " + dd.bDeviceSubClass);
	console.log("  bDeviceProtocol               " + dd.bDeviceProtocol);
	console.log("  bMaxPacketSize0               " + dd.bMaxPacketSize0);
	var idVendor = pad(toHex(dd.idVendor), 4, "0");
	console.log("  idVendor                      " + "0x" + idVendor); // + " " + usb_ids[idVendor].name);
	var idProduct = pad(toHex(dd.idProduct), 4, "0")
	console.log("  idProduct                     " + "0x" + idProduct); // + " " + usb_ids[idVendor].products[idProduct].name);
	// 2 bcdDevice: 2 bytes
	var bcdDeviceArr = toByteArray(dd.bcdDevice);
	console.log("  bcdDevice                     " + bcdDeviceArr[0] + "." + pad(bcdDeviceArr[1], 2, "0"));
	console.log("  iManufacturer                 " + dd.iManufacturer);
	// iSerialNumber is default of libusb-1.0
	console.log("  iSerial                       " + dd.iSerialNumber);
	console.log("  bNumConfigurations            " + dd.bNumConfigurations);
}

instance.close();

