/**
 * Expose complete node-usb binding to node.js
 */
var binding = require("usb_bindings");

exports.create = function() {
	var usbInstance = new binding.Usb();
	var devices = undefined;

	// singleton
	usbInstance.get_devices = function() {
		if (devices == undefined) {
			devices = this.getDevices();
			
			for (var i = 0, m = devices.length; i < m; i++) {
				var device = devices[i];
				var dd = device.getConfigDescriptor();
				device.extra_length = dd.extra_length;
			}
		}

		return devices;
	}

	// prototype method for finding devices by vendor and product id
	usbInstance.find_by_vid_and_pid = function(vid, pid) {
		var r = new Array();
		var devices = this.get_devices();

		for (var i = 0, m = devices.length; i < m; i++) {
			var deviceDesc = devices[i].getDeviceDescriptor();

			if ((deviceDesc.idVendor == vid) && (deviceDesc.idProduct == pid)) {
				r.push(devices[i]);
			}
		}
		
		return r;
	}

	return usbInstance;
}


