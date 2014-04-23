var usb = exports = module.exports = require("bindings-shyp")("usb_bindings")
var events = require('events')
var util = require('util')

// Check that libusb was initialized.
if (usb.INIT_ERROR) {
	throw new Error('Could not initialize libusb. Check that your system has a usb controller.');
}

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

usb.Device.prototype.interface = function(addr){
	if (!this.interfaces){
		throw new Error("Device must be open before searching for interfaces")
	}
	addr = addr || 0
	for (var i=0; i<this.interfaces.length; i++){
		if (this.interfaces[i].interfaceNumber == addr){
			return this.interfaces[i]
		}
	}
}

var SETUP_SIZE = usb.LIBUSB_CONTROL_SETUP_SIZE

usb.Device.prototype.controlTransfer =
function(bmRequestType, bRequest, wValue, wIndex, data_or_length, callback){
	var self = this
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

	var transfer = new usb.Transfer(this, 0, usb.LIBUSB_TRANSFER_TYPE_CONTROL, this.timeout,
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
	return transfer.submit(buf)
}

usb.Device.prototype.getStringDescriptor = function (desc_index, callback) {
	var langid = 0x0409;
	var length = 1024;
	this.controlTransfer(
		usb.LIBUSB_ENDPOINT_IN,
		usb.LIBUSB_REQUEST_GET_DESCRIPTOR,
		((usb.LIBUSB_DT_STRING << 8) | desc_index),
		langid,
		length,
		function (error, buf) {
			if (error) return callback(error);
			callback(undefined, buf.toString('utf16le', 2));
		}
	);
}

function Interface(device, id){
	this.device = device
	this.id = id
	this.altSetting = 0;
	this.__refresh()
}

Interface.prototype.__refresh = function(){
	this.descriptor = this.device.configDescriptor.interfaces[this.id][this.altSetting]
	this.interfaceNumber = this.descriptor.bInterfaceNumber
	this.endpoints = []
	var len = this.descriptor.endpoints.length
	for (var i=0; i<len; i++){
		var desc = this.descriptor.endpoints[i]
		var c = (desc.bEndpointAddress&usb.LIBUSB_ENDPOINT_IN)?InEndpoint:OutEndpoint
		this.endpoints[i] = new c(this.device, desc)
	}
}

Interface.prototype.claim = function(){
	this.device.__claimInterface(this.id)
}

Interface.prototype.release = function(closeEndpoints, cb){
	var self = this;
	if (typeof closeEndpoints == 'function') {
		cb = closeEndpoints;
		closeEndpoints = null;
	}

	if (!closeEndpoints || this.endpoints.length == 0) {
		next();
	} else {
		var n = self.endpoints.length;
		self.endpoints.forEach(function (ep, i) {
			if (ep.streamActive) {
				ep.once('end', function () {
					if (--n == 0) next();
				});
				ep.stopStream();
			} else {
				if (--n == 0) next();
			}
		});
	}

	function next () {
		self.device.__releaseInterface(self.id, function(err){
			if (!err){
				self.altSetting = 0;
				self.__refresh()
			}
			cb.call(self, err)
		})
	}
}

Interface.prototype.isKernelDriverActive = function(){
	return this.device.__isKernelDriverActive(this.id)
}

Interface.prototype.detachKernelDriver = function() {
	return this.device.__detachKernelDriver(this.id)
};

Interface.prototype.attachKernelDriver = function() {
	return this.device.__attachKernelDriver(this.id)
};


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
util.inherits(Endpoint, events.EventEmitter)

Endpoint.prototype.makeTransfer = function(timeout, callback){
	return new usb.Transfer(this.device, this.address, this.transferType, timeout, callback)
}

Endpoint.prototype.startStream = function(nTransfers, transferSize, callback){
	if (this.streamTransfers){
		throw new Error("Stream already active")
	}

	nTransfers = nTransfers || 3;
	this.streamTransferSize = transferSize || this.maxPacketSize;
	this.streamActive = true
	this.streamPending = 0

	var transfers = []
	for (var i=0; i<nTransfers; i++){
		transfers[i] = this.makeTransfer(0, callback)
	}
	return transfers;
}

Endpoint.prototype.stopStream = function(){
	if (!this.streamTransfers) {
		throw new Error('Stream is not active.');
	}
	for (var i=0; i<this.streamTransfers.length; i++){
		this.streamTransfers[i].cancel()
	}
	this.streamActive = false
}

function InEndpoint(device, descriptor){
	Endpoint.call(this, device, descriptor)
}

exports.InEndpoint = InEndpoint
util.inherits(InEndpoint, Endpoint)
InEndpoint.prototype.direction = "in"

InEndpoint.prototype.transfer = function(length, cb){
	var self = this
	var buffer = new Buffer(length)

	function callback(error, buf, actual){
		cb.call(self, error, buffer.slice(0, actual))
	}

	return this.makeTransfer(this.device.timeout, callback).submit(buffer)
}

InEndpoint.prototype.startStream = function(nTransfers, transferSize){
	var self = this
	this.streamTransfers = InEndpoint.super_.prototype.startStream.call(this, nTransfers, transferSize, transferDone)
	
	function transferDone(error, buf, actual){
		if (!error){
			self.emit("data", buf.slice(0, actual))
		}else if (error.errno != usb.LIBUSB_TRANSFER_CANCELLED){
			self.emit("error", error)
			self.stopStream()
		}

		if (self.streamActive){
			startTransfer(this)
		}else{
			self.streamPending--

			if (self.streamPending == 0){
				self.emit('end')
			}
		}
	}

	function startTransfer(t){
		t.submit(new Buffer(self.streamTransferSize), transferDone)
	}

	this.streamTransfers.forEach(startTransfer)
	self.streamPending = this.streamTransfers.length
}



function OutEndpoint(device, descriptor){
	Endpoint.call(this, device, descriptor)
}
exports.OutEndpoint = OutEndpoint
util.inherits(OutEndpoint, Endpoint)
OutEndpoint.prototype.direction = "out"

OutEndpoint.prototype.transfer = function(buffer, cb){
	var self = this
	if (!buffer){
		buffer = new Buffer(0)
	}else if (!Buffer.isBuffer(buffer)){
		buffer = new Buffer(buffer)
	}

	function callback(error, buf, actual){
		if (cb) cb.call(self, error)
	}

	return this.makeTransfer(this.device.timeout, callback).submit(buffer)
}

OutEndpoint.prototype.transferWithZLP = function (buf, cb) {
	if (buf.length % this.descriptor.wMaxPacketSize == 0) {
		this.transfer(buf);
		this.transfer(new Buffer(0), cb);
	} else {
		this.transfer(buf, cb);
	}
}


OutEndpoint.prototype.startStream = function startStream(n_transfers, transfer_size){
	n_transfers = n_transfers || 3;
	transfer_size = transfer_size || this.maxPacketSize;
	this.streamActive = true;
	this._streamTransfers = n_transfers;
	this._pendingTransfers = 0;
	var self = this
	process.nextTick(function(){
		for (var i=0; i<n_transfers; i++) self.emit('drain')
	})
}

function out_ep_callback(err, d){
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
	this.streamActive = false;
	if (this._pendingTransfers === null) throw new Error('stopStream invoked on non-active stream.');
	if (this._pendingTransfers == 0) this.emit('end');
}
