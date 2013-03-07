var usb = exports = module.exports = require("bindings")("usb_bindings");
var events = require('events');

// convenience method for finding a device by vendor and product id
exports.findByIds = function(vid, pid) {
	var devices = usb.getDeviceList()
	
	for (var i = 0; i < devices.length; i++) {
		var deviceDesc = devices[i].deviceDescriptor
		if ((deviceDesc.idVendor == vid) && (deviceDesc.idProduct == pid)) {
			return devices[i]
		}
	}
}

usb.Device.prototype.timeout = 1000

usb.Device.prototype.open = function(){
	this.__open()
	this.interfaces = []
	var len = this.configDescriptor.interfaces.length
	for (var i=0; i<len; i++){
		this.interfaces[i] = new Interface(this, i)
	}
}

usb.Device.prototype.close = function(){
	this.__close()
	this.interfaces = null
}

Object.defineProperty(usb.Device.prototype, "configDescriptor", {
    get: function() {
        return this.configDescriptor = this.__getConfigDescriptor()
    }
});

var SETUP_SIZE = usb.LIBUSB_CONTROL_SETUP_SIZE

usb.Device.prototype.controlTransfer =
function(bmRequestType, bRequest, wValue, wIndex, data_or_length, callback){
	var self = this
	var timeout = 1000
	var isIn = !!(bmRequestType & usb.LIBUSB_ENDPOINT_IN)
	var wLength

	if (isIn){
		if (!(data_or_length >= 0)){
			throw new TypeError("Expected size number for IN transfer (based on bmRequestType)")
		}
		wLength = data_or_length
	}else{
		if (!Buffer.isBuffer(data_or_length)){
			throw new TypeError("Expected buffer for OUT transfer (based on bmRequestType)")
		}
		wLength = data_or_length.length
	}

	// Buffer for the setup packet
	// http://libusbx.sourceforge.net/api-1.0/structlibusb__control__setup.html
	var buf = new Buffer(wLength + SETUP_SIZE)
	buf.writeUInt8(   bmRequestType, 0)
	buf.writeUInt8(   bRequest,      1)
	buf.writeUInt16LE(wValue,        2)
	buf.writeUInt16LE(wIndex,        4)
	buf.writeUInt16LE(wLength,       6)

	if (!isIn){
		data_or_length.copy(buf, SETUP_SIZE)
	}

	var transfer = new usb.Transfer(this, 0, usb.LIBUSB_TRANSFER_TYPE_CONTROL, timeout)
	return transfer.submit(buf,
		function(error, buf, actual){
			if (callback){
				if (isIn){
					callback.call(self, error, buf.slice(SETUP_SIZE, SETUP_SIZE + actual))
				}else{
					callback.call(self, error)
				}
			}
		}
	)


}

function Interface(device, id){
	this.device = device
	this.id = id
	this.altSetting = 0;
	this.__refresh()
}

Interface.prototype.__refresh = function(){
	this.descriptor = this.device.configDescriptor.interfaces[this.id][this.altSetting]
	this.endpoints = []
	var len = this.descriptor.endpoints.length
	for (var i=0; i<len; i++){
		var desc = this.descriptor.endpoints[i]
		var c = (desc.bEndpointAddress&(1 << 7) == usb.LIBUSB_ENDPOINT_IN)?InEndpoint:OutEndpoint
		this.endpoints[i] = new c(this.device, desc)
	}
}

Interface.prototype.claim = function(){
	this.device.__claimInterface(this.id)
}

Interface.prototype.release = function(cb){
	var self = this;
	this.device.__releaseInterface(this.id, function(err){
		if (!err){
			self.altSetting = 0;
			self.__refresh()
		}
		cb.call(self, err)
	})
}

Interface.prototype.setAltSetting = function(altSetting, cb){
	var self = this;
	this.device.__setInterface(this.id, altSetting, function(err){
		if (!err){
			self.altSetting = altSetting;
			self.__refresh();
		}
		cb.call(self, err)
	})

}

Interface.prototype.endpoint = function(addr){
	for (var i=0; i<this.endpoints.length; i++){
		if (this.endpoints[i].address == addr){
			return this.endpoints[i]
		}
	}
}

function Endpoint(device, descriptor){
	this.device = device
	this.descriptor = descriptor
	this.address = descriptor.bEndpointAddress
	this.transferType = descriptor.bmAttributes&0x03
}

Endpoint.prototype.makeTransfer = function(){
	return new usb.Transfer(this.device, this.address, this.transferType, this.device.timeout)
}

Endpoint.prototype.startStream = function(){

}

Endpoint.prototype.stopStream = function(){

}

function InEndpoint(device, descriptor){
	Endpoint.call(this, device, descriptor)
}

exports.InEndpoint = InEndpoint
InEndpoint.prototype = Object.create(Endpoint.prototype)
InEndpoint.prototype.direction = "in"

InEndpoint.prototype.transfer = function(length, cb){
	var self = this
	var buffer = new Buffer(length)
	return this.makeTransfer().submit(buffer,
		function(error, buf, actual){
			cb.call(self, error, buffer.slice(0, actual))
		}
	)
}

function OutEndpoint(device, descriptor){
	Endpoint.call(this, device, descriptor)
}
exports.OutEndpoint = OutEndpoint
OutEndpoint.prototype = Object.create(Endpoint.prototype)
OutEndpoint.prototype.direction = "out"

OutEndpoint.prototype.transfer = function(buffer, cb){
	var self = this
	if (!buffer){
		buffer = new Buffer(0)
	}else if (!Buffer.isBuffer(buffer)){
		buffer = new Buffer(buffer)
	}

	return this.makeTransfer().submit(buffer,
		function(error, buf, actual){
			if (cb) cb.call(self, error)
		}
	)
}


/*
usb.OutEndpoint.prototype.startStream = function startStream(n_transfers, transfer_size){
	n_transfers = n_transfers || 3;
	transfer_size = transfer_size || this.maxPacketSize;
	this._streamTransfers = n_transfers;
	this._pendingTransfers = 0;
	for (var i=0; i<n_transfers; i++) this.emit('drain');
}

function out_ep_callback(d, err){
	//console.log("out_ep_callback", d, err, this._pendingTransfers, this._streamTransfers)
	if (err) this.emit('error', err);
	this._pendingTransfers--;
	if (this._pendingTransfers < this._streamTransfers){
		this.emit('drain');
	}
	if (this._pendingTransfers <= 0 && this._streamTransfers == 0){
		this.emit('end');
	}
}

usb.OutEndpoint.prototype.write = function write(data){
	this.transfer(data, out_ep_callback);
	this._pendingTransfers++;
}

usb.OutEndpoint.prototype.stopStream = function stopStream(){
	this._streamTransfers = 0;
	if (this._pendingTransfers == 0) this.emit('end');
}

inherits(usb.InEndpoint, events.EventEmitter);

usb.InEndpoint.prototype.startStream = function(n_transfers, transferSize){
	var self = this
	n_transfers = n_transfers || 3;
	transferSize = transferSize || this.maxPacketSize;
	
	function transferDone(data, error){
		if (!error){
			self.emit("data", data)

			if (data.length == transferSize){
				startTransfer()
			}else{
				self.emit("end")
			}
		}else{
			self.emit("error", error)
		}
	}

	function startTransfer(){
		self.transfer(transferSize, transferDone)
	}

	for (var i=0; i<n_transfers; i++){
		startTransfer()
	}
}
*/