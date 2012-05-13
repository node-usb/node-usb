/**
 * Expose complete node-usb binding to node.js
 */
var usb = exports = module.exports = require("./usb_bindings");
var events = require('events');

var devices = undefined;

// singleton
exports.getDevices = function() {
	if (devices == undefined) {
		devices = usb._getDevices();
		
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

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

inherits(usb.Endpoint, events.EventEmitter);

usb.Endpoint.prototype.__stream_data_cb = function stream_data_cb(data, error){
	if (!error){
		this.emit("data", data)
	}else{
		this.emit("error", error)
	}
}

usb.Endpoint.prototype.__stream_stop_cb = function stream_stop_cb(data, error){
	this.emit("end")
}
