/**
 * Expose complete node-usb binding to node.js
 */
var binding = require("./usb_bindings");

exports.create = function() {
	var usbInstance = new binding.Usb();

	return usbInstance;
}
