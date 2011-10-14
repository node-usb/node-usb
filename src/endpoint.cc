#include "bindings.h"
#include "endpoint.h"
#include "device.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Endpoint::constructor_template;

	Endpoint::Endpoint(Handle<Object> _device, nodeusb_device_container* _device_container, const libusb_endpoint_descriptor* _endpoint_descriptor, uint32_t _idx_endpoint) {
		device = Persistent<Object>::New(_device);
		device_container = _device_container;
		descriptor = _endpoint_descriptor;
		// if bit[7] of endpoint address is set => ENDPOINT_IN (device to host), else: ENDPOINT_OUT (host to device)
		endpoint_type = (descriptor->bEndpointAddress & (1 << 7)) ? (LIBUSB_ENDPOINT_IN) : (LIBUSB_ENDPOINT_OUT);
		// bit[0] and bit[1] of bmAttributes masks transfer_type; 3 = 0000 0011
		transfer_type = (3 & descriptor->bmAttributes);
		idx_endpoint = _idx_endpoint;
	}

	Endpoint::~Endpoint() {
		// TODO Close
		device.Dispose();
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
		NODE_SET_PROTOTYPE_METHOD(t, "getExtraData", Endpoint::GetExtraData);
		NODE_SET_PROTOTYPE_METHOD(t, "submitNative", Endpoint::Submit);
		NODE_SET_PROTOTYPE_METHOD(t, "bulkTransfer", Endpoint::BulkTransfer);
		NODE_SET_PROTOTYPE_METHOD(t, "interruptTransfer", Endpoint::InterruptTransfer);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Endpoint"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Endpoint::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Endpoint object created")

		if (args.Length() != 5 || !args[0]->IsObject() || !args[1]->IsExternal() || !args[2]->IsUint32() || !args[3]->IsUint32()|| !args[4]->IsUint32()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:device, object:external:libusb_device, uint32_t:idx_interface, uint32_t:idx_alt_setting, uint32_t:idx_endpoint]!")
		}

		// make local value reference to first parameter
		Local<Object> device = Local<Object>::Cast(args[0]);
		Local<External> refDeviceContainer = Local<External>::Cast(args[1]);
		uint32_t idxInterface  = args[2]->Uint32Value();
		uint32_t idxAltSetting = args[3]->Uint32Value();
		uint32_t idxEndpoint   = args[4]->Uint32Value();

		nodeusb_device_container *deviceContainer = static_cast<nodeusb_device_container*>(refDeviceContainer->Value());
		const libusb_endpoint_descriptor *libusbEndpointDescriptor = &(((*deviceContainer->config_descriptor).interface[idxInterface]).altsetting[idxAltSetting]).endpoint[idxEndpoint];

		// create new Endpoint object
		Endpoint *endpoint = new Endpoint(device, deviceContainer, libusbEndpointDescriptor, idxEndpoint);
		// initalize handle

#define LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Uint32::New(endpoint->descriptor->name));
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

	Handle<Value> Endpoint::GetExtraData(const Arguments& args) {
		LOCAL(Endpoint, self, args.This())
		 
		int m = (*self->descriptor).extra_length;
		
		Local<Array> r = Array::New(m);
		
		for (int i = 0; i < m; i++) {
		  uint32_t c = (*self->descriptor).extra[i];
		  
		  r->Set(i, Uint32::New(c));
		}
		
		return scope.Close(r);
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
#define TRANSFER_ARGUMENTS_MIDDLE _buffer, _buflen
#define TRANSFER_ARGUMENTS_RIGHT Callback::DispatchAsynchronousUsbTransfer, &(_callback), _timeout
#define TRANSFER_ARGUMENTS_DEFAULT TRANSFER_ARGUMENTS_LEFT, descriptor->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, TRANSFER_ARGUMENTS_RIGHT
	int Endpoint::FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, int32_t _buflen, Persistent<Function> _callback, uint32_t _timeout, unsigned int num_iso_packets) {
		int err = 0;

		switch (transfer_type) {
			case LIBUSB_TRANSFER_TYPE_BULK:
				libusb_fill_bulk_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_INTERRUPT:
				libusb_fill_interrupt_transfer(TRANSFER_ARGUMENTS_DEFAULT);
				break;
			case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
				libusb_fill_iso_transfer(TRANSFER_ARGUMENTS_LEFT, descriptor->bEndpointAddress, TRANSFER_ARGUMENTS_MIDDLE, num_iso_packets, TRANSFER_ARGUMENTS_RIGHT);
				break;
			default:
				err = -1;
		}

		return err;
	}

#define CHECK_MODUS_EQUALS_ENDPOINT_TYPE \
		if (modus != self->endpoint_type) {\
			THROW_BAD_ARGS("Endpoint::Transfer is used as a wrong endpoint type. Change your parameters") \
		}\

	/**
	 * @param function js-callback[status]
	 * @param array byte-array
	 * @param int (optional) timeout timeout in milliseconds
	 */
	Handle<Value> Endpoint::Submit(const Arguments& args) {
		LOCAL(Endpoint, self, args.This())
		uint32_t iso_packets = 0;
		uint8_t flags = 0;
		
		INIT_TRANSFER_CALL(2, 1, 2)
		CHECK_MODUS_EQUALS_ENDPOINT_TYPE

		if (args.Length() >= 4) {
			if (!args[3]->IsUint32()) {
				THROW_BAD_ARGS("Endpoint::Submit expects unsigned char as flags parameter")
			} else {
				flags = (uint8_t)args[3]->Uint32Value();
			}
		}

		Local<Function> callback = Local<Function>::Cast(args[1]);

		// TODO Isochronous transfer mode 
		if (self->transfer_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
		}
		
		libusb_transfer* transfer = libusb_alloc_transfer(iso_packets);
		

		if (self->FillTransferStructure(transfer, buf, buflen, Persistent<Function>::New(callback), timeout, iso_packets) < 0) {
			THROW_BAD_ARGS("Could not fill USB packet structure on device!")
		}

		transfer->flags = flags;
		libusb_submit_transfer(transfer);
		
		return scope.Close(True());
	}

#define BULK_INTERRUPT_EIO(EIO_TO_EXECUTE, EIO_AFTER)\
		LOCAL(Endpoint, self, args.This())\
		INIT_TRANSFER_CALL(2, 1, 2)\
		CHECK_MODUS_EQUALS_ENDPOINT_TYPE\
		OPEN_DEVICE_HANDLE_NEEDED(scope)\
		EIO_NEW(bulk_interrupt_transfer_request, bulk_interrupt_transfer_req)\
		EIO_DELEGATION(bulk_interrupt_transfer_req, 1)\
		bulk_interrupt_transfer_req->endpoint = self;\
		bulk_interrupt_transfer_req->endpoint->Ref();\
		bulk_interrupt_transfer_req->timeout = timeout;\
		bulk_interrupt_transfer_req->length = buflen;\
		bulk_interrupt_transfer_req->data = buf;\
		eio_custom(EIO_TO_EXECUTE, EIO_PRI_DEFAULT, EIO_AFTER, bulk_interrupt_transfer_req);\
		ev_ref(EV_DEFAULT_UC);\
		return Undefined();	

#define BULK_INTERRUPT_FREE TRANSFER_REQUEST_FREE(bulk_interrupt_transfer_request, endpoint)


#define BULK_INTERRUPT_EXECUTE(METHOD, SOURCE)\
		EIO_CAST(bulk_interrupt_transfer_request, bit_req)\
		Endpoint * self = bit_req->endpoint;\
		bit_req->errcode = libusb_bulk_transfer(self->device_container->handle, self->descriptor->bEndpointAddress, bit_req->data, bit_req->length, &(bit_req->transferred), bit_req->timeout);\
		if (bit_req->errcode < LIBUSB_SUCCESS) {\
			bit_req->errsource = SOURCE;\
		}\
		req->result = 0;\
		return 0;


	Handle<Value> Endpoint::BulkTransfer(const Arguments& args) {
		BULK_INTERRUPT_EIO(EIO_BulkTransfer, EIO_After_BulkTransfer)
	}

	int Endpoint::EIO_BulkTransfer(eio_req *req) {
		BULK_INTERRUPT_EXECUTE(libusb_bulk_transfer, "bulkTransfer")
	}

	int Endpoint::EIO_After_BulkTransfer(eio_req *req) {
		BULK_INTERRUPT_FREE
	}

	Handle<Value> Endpoint::InterruptTransfer(const Arguments& args) {
		BULK_INTERRUPT_EIO(EIO_InterruptTransfer, EIO_After_InterruptTransfer)
	}

	int Endpoint::EIO_InterruptTransfer(eio_req *req) {
		BULK_INTERRUPT_EXECUTE(libusb_interrupt_transfer, "interruptTransfer")
	}

	int Endpoint::EIO_After_InterruptTransfer(eio_req *req) {
		BULK_INTERRUPT_FREE
	}
}
