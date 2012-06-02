/**
 * Expose complete node-usb binding to node.js
 */
var usb = exports = module.exports = require("./usb_bindings");
var events = require('events');


//  method for finding devices by vendor and product id
exports.find_by_vid_and_pid = function(vid, pid) {
	var r = new Array();
	var len = usb.devices.length;
	
	for (var i = 0, m = len; i < m; i++) {
		var dev = usb.devices[i]
		var deviceDesc = dev.deviceDescriptor;

		if ((deviceDesc.idVendor == vid) && (deviceDesc.idProduct == pid)) {
			r.push(dev);
		}
	}
	
	return r;
}

usb.Device.prototype.interface = function interface(interface, altSetting){
	altSetting = altSetting || 0;
	var interfaces = this.interfaces;
	for (var i=0; i<interfaces.length; i++){
		if (interfaces[i].interface == interface && interfaces[i].altSetting == altSetting)
			return interfaces[i];
	}
}

usb.Interface.prototype.endpoint = function endpoint(addr){
	var endpoints = this.endpoints;
	for (var i=0; i<endpoints.length; i++){
		if (endpoints[i].bEndpointAddress == addr) return endpoints[i];
	}
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
