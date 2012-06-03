#include "bindings.h"
#include "endpoint.h"
#include "device.h"
#include "transfer.h"
#include "stream.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Endpoint::constructor_template_in;
	Persistent<FunctionTemplate> Endpoint::constructor_template_out;

	Endpoint::Endpoint(Handle<Object> _v8device, Device* _device, const libusb_endpoint_descriptor* _endpoint_descriptor, uint32_t _idx_endpoint) : ObjectWrap() {
		v8device = Persistent<Object>::New(_v8device);
		device = _device;
		descriptor = _endpoint_descriptor;
		// if bit[7] of endpoint address is set => ENDPOINT_IN (device to host), else: ENDPOINT_OUT (host to device)
		endpoint_type = (descriptor->bEndpointAddress & (1 << 7)) ? (LIBUSB_ENDPOINT_IN) : (LIBUSB_ENDPOINT_OUT);
		// bit[0] and bit[1] of bmAttributes masks transfer_type; 3 = 0000 0011
		transfer_type = (libusb_transfer_type)(3 & descriptor->bmAttributes);
		idx_endpoint = _idx_endpoint;
		stream = NULL;
	}

	Endpoint::~Endpoint() {
		delete stream;
		// TODO Close
		v8device.Dispose();
		DEBUG("Endpoint object destroyed")
	}

	void Endpoint::initTemplate(Local<FunctionTemplate> t){
		Local<ObjectTemplate> instance_template = t->InstanceTemplate();
		instance_template->SetInternalFieldCount(1);

		// Getters
		instance_template->SetAccessor(V8STR("direction"), Endpoint::EndpointTypeGetter);
		instance_template->SetAccessor(V8STR("transferType"), Endpoint::TransferTypeGetter);
		instance_template->SetAccessor(V8STR("maxIsoPacketSize"), Endpoint::MaxIsoPacketSizeGetter);
		instance_template->SetAccessor(V8STR("maxPacketSize"), Endpoint::MaxPacketSizeGetter);
		instance_template->SetAccessor(V8STR("extraData"), Endpoint::ExtraDataGetter);
	}
		
	void Endpoint::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t_in = FunctionTemplate::New(Endpoint::New);
		Local<FunctionTemplate> t_out = FunctionTemplate::New(Endpoint::New);

		initTemplate(t_in);
		initTemplate(t_out);
		
		// Constructor	
		t_in->SetClassName(String::NewSymbol("InEndpoint"));
		t_out->SetClassName(String::NewSymbol("OutEndpoint"));
		
		Endpoint::constructor_template_in = Persistent<FunctionTemplate>::New(t_in);
		Endpoint::constructor_template_out = Persistent<FunctionTemplate>::New(t_out);

		// methods exposed to node.js
		NODE_SET_PROTOTYPE_METHOD(t_out, "transfer", Endpoint::StartTransfer);
		NODE_SET_PROTOTYPE_METHOD(t_in, "transfer", Endpoint::StartTransfer);
		NODE_SET_PROTOTYPE_METHOD(t_in, "startStream", Endpoint::StartStream);
		NODE_SET_PROTOTYPE_METHOD(t_in, "stopStream", Endpoint::StopStream);
		
		// Make it visible in JavaScript
		target->Set(String::NewSymbol("InEndpoint"), t_in->GetFunction());
		target->Set(String::NewSymbol("OutEndpoint"), t_out->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Endpoint::New(const Arguments& args) {
		HandleScope scope;

		if (!AllowConstructor::Check()) THROW_ERROR("Illegal constructor");
		
		DEBUG("New Endpoint object created")

		if (!args.IsConstructCall()
		   || args.Length() != 4
		   || !args[0]->IsObject()
		   || !args[1]->IsUint32()
		   || !args[2]->IsUint32()
		   || !args[3]->IsUint32()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:device, uint32_t:idx_interface, uint32_t:idx_alt_setting, uint32_t:idx_endpoint]!")
		}

		// make local value reference to first parameter
		Local<Object> device = Local<Object>::Cast(args[0]);
		Device *dev = ObjectWrap::Unwrap<Device>(device);
		uint32_t idxInterface  = args[1]->Uint32Value();
		uint32_t idxAltSetting = args[2]->Uint32Value();
		uint32_t idxEndpoint   = args[3]->Uint32Value();

		const libusb_interface* interface;
		if (idxInterface < dev->config_descriptor->bNumInterfaces){
			interface = &dev->config_descriptor->interface[idxInterface];
		}else THROW_BAD_ARGS("Invalid interface");

		const libusb_interface_descriptor* interface_alt;
		if ((int) idxAltSetting < interface->num_altsetting){
			interface_alt = &interface->altsetting[idxAltSetting];
		}else THROW_BAD_ARGS("Invalid altsetting");
		
		const libusb_endpoint_descriptor *libusbEndpointDescriptor;
		if (idxEndpoint < interface_alt->bNumEndpoints){
			libusbEndpointDescriptor = &interface_alt->endpoint[idxEndpoint];
		}else THROW_BAD_ARGS("Invalid endpoint");
		
		// create new Endpoint object
		Endpoint *endpoint = new Endpoint(device, dev, libusbEndpointDescriptor, idxEndpoint);
		// initalize handle

#define LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Uint32::New(endpoint->descriptor->name), CONST_PROP);
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bEndpointAddress)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(wMaxPacketSize)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bInterval)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bRefresh)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bSynchAddress)
		LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		// wrap created Endpoint object to v8
		endpoint->Wrap(args.This());

		return args.This();
	}

	Handle<Value> Endpoint::EndpointTypeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		
		return scope.Close(Integer::New(self->endpoint_type));
	}

	Handle<Value> Endpoint::TransferTypeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		
		return scope.Close(Integer::New(self->transfer_type));
	}

	Handle<Value> Endpoint::MaxPacketSizeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		int r = 0;
		
		CHECK_USB((r = libusb_get_max_packet_size(self->device->device, self->descriptor->bEndpointAddress)), scope)
		
		return scope.Close(Integer::New(r));
	}

	Handle<Value> Endpoint::MaxIsoPacketSizeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		int r = 0;
		
		CHECK_USB((r = libusb_get_max_iso_packet_size(self->device->device, self->descriptor->bEndpointAddress)), scope)
		
		return scope.Close(Integer::New(r));
	}

	Handle<Value> Endpoint::ExtraDataGetter(Local<String> property, const AccessorInfo &info){
		LOCAL(Endpoint, self, info.Holder())
		 
		return scope.Close(makeBuffer(self->descriptor->extra, self->descriptor->extra_length));
	}

	Handle<Value> Endpoint::StartTransfer(const Arguments& args){
		LOCAL(Endpoint, self, args.This())
		
		CHECK_USB(self->device->openHandle(), scope);

		unsigned length;
		unsigned char *buf;
		
		//args: buffer/size, callback
		
		if (args.Length() < 2 || !args[1]->IsFunction()) {
			THROW_BAD_ARGS("Transfer missing arguments!")
		}
		
		BUF_LEN_ARG(args[0]);
		
		if (modus != self->endpoint_type) {
			THROW_BAD_ARGS("Transfer is used in the wrong direction (IN/OUT) for this endpoint");
		}
		
		DEBUG_OPT("Submitting transfer %x (%i: %p)", modus, length, buf);
		
		NodeUsb::Transfer* t = Transfer::newTransfer(
			self->transfer_type,
			self->v8device,
			args.This(),
			self->descriptor->bEndpointAddress,
			buf,
			length,
			self->device->timeout,
			Handle<Function>::Cast(args[1])
		);
		
		t->submit();

		return Undefined();
	}
	
	
	Handle<Value> Endpoint::StartStream(const Arguments& args){
		LOCAL(Endpoint, self, args.This())
		
		if (self->endpoint_type != LIBUSB_ENDPOINT_IN){
			THROW_BAD_ARGS("This implementation is only for IN endpoints.");
		}
		
		CHECK_USB(self->device->openHandle(), scope);
		
		unsigned transfer_size = libusb_get_max_packet_size(self->device->device, self->descriptor->bEndpointAddress);
		unsigned n_transfers = 3;
		
		//args: n_transfers, transfer_size
		if (args.Length() >= 1) {
			INT_ARG(n_transfers, args[0]);
		}

		if (args.Length() >= 2) { 
			INT_ARG(transfer_size, args[1]);
		}
		
		if (!self->stream){
			Handle<Value> streamCb = args.This()->Get(V8SYM("__stream_data_cb"));
			Handle<Value> streamStopCb = args.This()->Get(V8SYM("__stream_stop_cb"));
			
			if (!streamCb->IsFunction() || !streamStopCb->IsFunction()){
				THROW_BAD_ARGS("Stream prototype functions not defined");
			}
		
			self->stream = new Stream(args.This(),
			                          Handle<Function>::Cast(streamCb),
			                          Handle<Function>::Cast(streamStopCb));
		}
		
		if (self->stream->state != Stream::STREAM_IDLE){
			THROW_BAD_ARGS("Stream cannot be started");
		}
				
		self->stream->start(n_transfers, transfer_size);
		
		return Undefined();
	}
	
	Handle<Value> Endpoint::StopStream(const Arguments& args){
		LOCAL(Endpoint, self, args.This())
		
		if (self->stream && self->stream->state == Stream::STREAM_ACTIVE){
			self->stream->stop();
		}
		
		return Undefined();
	}

}
