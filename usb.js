/**
 * Expose complete node-usb binding to node.js
 */
var exports = module.exports = require("./usb_bindings");

var devices = undefined;

// singleton
exports.getDevices = function() {
	if (devices == undefined) {
		devices = this._getDevices();
		
		for (var i = 0, m = devices.length; i < m; i++) {
			var device = devices[i];
		}
	}

	return devices;
}

//  method for finding devices by vendor and product id
exports.find_by_vid_and_pid = function(vid, pid) {
	var r = new Array();

	for (var i = 0, m = devices.length; i < m; i++) {
		var deviceDesc = devices[i].deviceDescriptor;

		if ((deviceDesc.idVendor == vid) && (deviceDesc.idProduct == pid)) {
			r.push(devices[i]);
		}
	}
	
	return r;
}
