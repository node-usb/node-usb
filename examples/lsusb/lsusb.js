var usb_driver = require("../../usb.js");
var usb_ids = require("../../usb_ids.js");
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

function get_extra_data(obj) {
	var data = obj.getExtraData(), r = "";
	
	if (!max_bytes) {
		var max_bytes = 5;
	}
	
	for (var i = 0; (i < data.length && i < max_bytes); i++) {
		r += "0x" + pad(toHex(data[i]), 2, 0) + " ";
	}

	if (i < data.length) {
		r += "(" + (data.length - i) + " bytes more)";
	}

	return r;
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
	var cd = device.getConfigDescriptor();

	console.log("Bus " + pad(device.busNumber, 3, "0") + " Device " + pad(device.deviceAddress, 3, "0") + " ID " + pad(toHex(dd.idVendor), 4, "0") + ":" + pad(toHex(dd.idProduct), 4, "0"));
	console.log("Device Descriptor:");
	console.log("  bLength                       " + dd.bLength);
	console.log("  bDescriptorType               " + dd.bDescriptorType);
	// 2 bcdUSB: 2 bytes
	var bcdUSBArr = toByteArray(dd.bcdUSB);
	console.log("  bcdUSB                        " + bcdUSBArr[0] + "." + pad(bcdUSBArr[1] || 0, 2, "0"));
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
	console.log("  bcdDevice                     " + bcdDeviceArr[0] + "." + pad(bcdDeviceArr[1] || 0, 2, "0"));
	console.log("  iManufacturer                 " + dd.iManufacturer);
	console.log("  iProduct                      " + dd.iProduct);
	// iSerialNumber is default of libusb-1.0
	console.log("  iSerial                       " + dd.iSerialNumber);
	console.log("  bNumConfigurations            " + dd.bNumConfigurations);
	console.log("  Configuration Descriptor: ");
	console.log("    bLength                     " + cd.bLength);
	console.log("    bDescriptorType             " + cd.bDescriptorType);
	console.log("    wTotalLength                " + cd.wTotalLength);
	console.log("    bNumInterfaces              " + cd.bNumInterfaces);
	console.log("    bConfigurationValue         " + cd.bConfigurationValue);
	console.log("    iConfiguration              " + cd.iConfiguration);
	console.log("    bmAttributes                0x" + toHex(cd.bmAttributes));
	console.log("    MaxPower                    " + cd.MaxPower);
	console.log("    __extra_length              " + cd.extra_length);
	console.log("    __extra_data (first 5b)     " + get_extra_data(device));

	var interfaces = device.getInterfaces();

	for (var j = 0; j < interfaces.length; j++) {
		var interface = interfaces[j];
		console.log("    Interface descriptor:");
		console.log("      bLength                   " + interface.bLength);
		console.log("      bDescriptorType           " + interface.bDescriptorType);
		console.log("      bInterfaceNumber          " + interface.bInterfaceNumber);
		console.log("      bAlternateSetting         " + interface.bAlternateSetting);
		console.log("      bNumEndpoints             " + interface.bNumEndpoints);
		console.log("      bInterfaceClass           " + interface.bInterfaceClass);
		console.log("      bInterfaceSubClass        " + interface.bInterfaceSubClass);
		console.log("      bInterfaceProtocol        " + interface.bInterfaceProtocol);
		console.log("      iInterface                " + interface.iInterface);
		console.log("      __extra_length            " + interface.extra_length);
		console.log("      __extra_data (first 5b)   " + get_extra_data(interface));
		
		var endpoints = interface.getEndpoints();

		for (var k = 0; k < endpoints.length; k++) {
			var endpoint = endpoints[k];
			console.log("      Endpoint descriptor:");
			console.log("        bLength                   " + endpoint.bLength);
			console.log("        bDescriptorType           " + endpoint.bDescriptorType);
			console.log("        bEndpointAddress          0x" + toHex(endpoint.bEndpointAddress));
			console.log("        bmAttributes              " + endpoint.bmAttributes);
			console.log("        wMaxPacketSize            0x" + pad(toHex(endpoint.wMaxPacketSize), 4, "0"));
			console.log("        bInterval                 " + endpoint.bInterval);
			console.log("        __extra_length            " + endpoint.extra_length);
			console.log("        __extra_data (first 5b)   " + get_extra_data(endpoint));
		}
	}
}

instance.close();

