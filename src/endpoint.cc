#include "./bindings.h"
#include "endpoint.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Endpoint::constructor_template;

	Endpoint::Endpoint(nodeusb_device_container* _device_container, libusb_endpoint_descriptor* _endpoint_descriptor) {
		device_container = _device_container;
		descriptor = _endpoint_descriptor;
		// if bit[7] of endpoint address is set => ENDPOINT_IN (device to host), else: ENDPOINT_OUT (host to device)
		endpoint_type = (descriptor->bEndpointAddress & (1 << 7)) ? (LIBUSB_ENDPOINT_IN) : (LIBUSB_ENDPOINT_OUT);
		// bit[0] and bit[1] of bmAttributes masks transfer_type; 3 = 0000 0011
		transfer_type = (3 & descriptor->bmAttributes);
	}

	Endpoint::~Endpoint() {
		// TODO Close
		DEBUG("Endpoint object destroyed")
	}


	void Endpoint::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Endpoint::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Endpoint"));
		Endpoint::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("__endpointType"), Endpoint::EndpointTypeGetter);
		instance_template->SetAccessor(V8STR("__transferType"), Endpoint::TransferTypeGetter);
		instance_template->SetAccessor(V8STR("__maxIsoPacketSize"), Endpoint::MaxIsoPacketSizeGetter);
		instance_template->SetAccessor(V8STR("__maxPacketSize"), Endpoint::MaxPacketSizeGetter);

		// methods exposed to node.js

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Endpoint"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Endpoint::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Endpoint object created")

		// need libusb_device structure as first argument
		if (args.Length() != 2 || !args[0]->IsExternal() || !args[1]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:external:libusb_device, object:external:libusb_external_descriptor]!") 
		}

		// make local value reference to first parameter
		Local<External> refDeviceContainer = Local<External>::Cast(args[0]);
		Local<External> refEndpointDescriptor = Local<External>::Cast(args[1]);

		// cast local reference to local 
		nodeusb_device_container *deviceContainer = static_cast<nodeusb_device_container*>(refDeviceContainer->Value());
		libusb_endpoint_descriptor *libusbEndpointDescriptor = static_cast<libusb_endpoint_descriptor*>(refEndpointDescriptor->Value());

		// create new Endpoint object
		Endpoint *endpoint = new Endpoint(deviceContainer, libusbEndpointDescriptor);
		// initalize handle

#define LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Integer::New(endpoint->descriptor->name));
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
		
		CHECK_USB((r = libusb_get_max_packet_size(self->device_container->device, self->descriptor->bEndpointAddress)), scope)
		
		return scope.Close(Integer::New(r));
	}

	Handle<Value> Endpoint::MaxIsoPacketSizeGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Endpoint, self, info.Holder())
		int r = 0;
		
		CHECK_USB((r = libusb_get_max_iso_packet_size(self->device_container->device, self->descriptor->bEndpointAddress)), scope)
		
		return scope.Close(Integer::New(r));
	}

	void Callback::DispatchAsynchronousUsbTransfer(libusb_transfer *_transfer) {
		const int num_args = 2;
		Local<Value> argv[num_args];

		// first argument is byte buffer
		Local<Array> bytes = Array::New();
		for (int i = 0; i < _transfer->actual_length; i++) {
			bytes->Set(i, Uint32::New(_transfer->buffer[i]));
		}
		argv[0] = bytes;

		// second argument of callback is transfer status
		argv[1] = Integer::New(_transfer->status);

		// user_data is our callback handler and has to be casted
		Persistent<Function>* callback = ((Persistent<Function>*)(_transfer->user_data));
		// call callback
		(*callback)->Call(Context::GetCurrent()->Global(), num_args, argv);
		// free callback
		(*callback).Dispose();
	}

#define TRANSFER_ARGUMENTS_LEFT _transfer, device_container->handle 
#define TRANSFER_ARGUMENTS_MIDDLE _buffer, buflen
#define TRANSFER_ARGUMENTS_RIGHT Callback::DispatchAsynchronousUsbTransfer, &(_callback), _timeout
#define TRANSFER_ARGUMENTS_DEFAULT TRANSFER_ARGUMENTS_LEFT, descriptor->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, TRANSFER_ARGUMENTS_RIGHT
	int Endpoint::FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, Persistent<Function> _callback, uint32_t _timeout, unsigned int num_iso_packets) {
		unsigned int buflen = -1; // TODO
		int err = 0;

		switch (transfer_type) {
			case LIBUSB_TRANSFER_TYPE_BULK:
				libusb_fill_bulk_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_INTERRUPT:
				libusb_fill_interrupt_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_CONTROL:
				libusb_fill_control_transfer(TRANSFER_ARGUMENTS_LEFT, _buffer, TRANSFER_ARGUMENTS_RIGHT);
				break;
			case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
				libusb_fill_iso_transfer(TRANSFER_ARGUMENTS_LEFT, descriptor->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, num_iso_packets, TRANSFER_ARGUMENTS_RIGHT);
				break;
			default:
				err = -1;
		}

		return err;
	}

	/**
	 * @param array byte-array
	 * @param function js-callback[status]
	 * @param int (optional) timeout timeout in milliseconds
	 */
	Handle<Value> Endpoint::Write(const Arguments& args) {
		LOCAL(Endpoint, self, args.This())

		uint32_t timeout = 0;

		// need libusb_device structure as first argument
		if (args.Length() < 2 || !args[0]->IsArray() || !args[1]->IsFunction()) {
			THROW_BAD_ARGS("Endpoint::Write expects at least 2 arguments [array[] data, function:callback[data, status] [, uint:timeout_in_ms, uint:transfer_flags]]!") 
		}

		if (args.Length() >= 3) {
			if (!args[2]->IsUint32()) {			
				THROW_BAD_ARGS("Endpoint::Write expects unsigned int as timeout parameter")
			} else {
				timeout = args[2]->Uint32Value();
			}
		}
		Local<Function> callback = Local<Function>::Cast(args[1]);

		// TODO Isochronous transfer mode 
		libusb_transfer* transfer = libusb_alloc_transfer(0);
		

		if (self->FillTransferStructure(transfer, NULL, Persistent<Function>::New(callback), timeout, 0) < 0) {
			THROW_BAD_ARGS("Could not fill USB packet structure on device!")
		}

		return scope.Close(True());
	}
}
