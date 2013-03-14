#include "node_usb.h"

#define STRUCT_TO_V8(TARGET, STR, NAME) \
		TARGET->Set(V8STR(#NAME), Uint32::New((STR).NAME), CONST_PROP);

#define CHECK_OPEN() \
		if (!self->handle){THROW_ERROR("Device is not opened");}

Device::Device(libusb_device* d): device(d), handle(0) {
	libusb_ref_device(device);
	DEBUG_LOG("Created device %p", this);
}


Device::~Device(){
	DEBUG_LOG("Freed device %p", this);
	libusb_close(handle);
	libusb_unref_device(device);
}

// Map pinning each libusb_device to a particular V8 instance
std::map<libusb_device*, Persistent<Value> > Device::byPtr;

// Get a V8 instance for a libusb_device: either the existing one from the map, 
// or create a new one and add it to the map.
Handle<Value> Device::get(libusb_device* dev){
	auto it = byPtr.find(dev);
	if (it != byPtr.end()){
		return it->second;
	}else{
		auto v = Persistent<Value>::New(pDevice.create(new Device(dev)));
		v.MakeWeak(dev, weakCallback);
		byPtr.insert(std::make_pair(dev, v));
		return v;	
	}
}

// Callback to remove an instance from the cache map when V8 wants to GC it
void Device::weakCallback(Persistent<Value> object, void *parameter){
	byPtr.erase(static_cast<libusb_device*>(parameter));
	object.Dispose();
	object.Clear();
	DEBUG_LOG("Removed cached device %p", parameter);
}

static Handle<Value> deviceConstructor(const Arguments& args){
	ENTER_CONSTRUCTOR_POINTER(pDevice, 1);

	args.This()->Set(V8SYM("busNumber"),
		Uint32::New(libusb_get_bus_number(self->device)), CONST_PROP);
	args.This()->Set(V8SYM("deviceAddress"),
		Uint32::New(libusb_get_device_address(self->device)), CONST_PROP);

	Local<Object> v8dd = Object::New();
	args.This()->Set(V8SYM("deviceDescriptor"), v8dd, CONST_PROP);

	struct libusb_device_descriptor dd;
	CHECK_USB(libusb_get_device_descriptor(self->device, &dd));

	STRUCT_TO_V8(v8dd, dd, bLength)
	STRUCT_TO_V8(v8dd, dd, bDescriptorType)
	STRUCT_TO_V8(v8dd, dd, bcdUSB)
	STRUCT_TO_V8(v8dd, dd, bDeviceClass)
	STRUCT_TO_V8(v8dd, dd, bDeviceSubClass)
	STRUCT_TO_V8(v8dd, dd, bDeviceProtocol)
	STRUCT_TO_V8(v8dd, dd, bMaxPacketSize0)
	STRUCT_TO_V8(v8dd, dd, idVendor)
	STRUCT_TO_V8(v8dd, dd, idProduct)
	STRUCT_TO_V8(v8dd, dd, bcdDevice)
	STRUCT_TO_V8(v8dd, dd, iManufacturer)
	STRUCT_TO_V8(v8dd, dd, iProduct)
	STRUCT_TO_V8(v8dd, dd, iSerialNumber)
	STRUCT_TO_V8(v8dd, dd, bNumConfigurations)

	return scope.Close(args.This());
}

Handle<Value> Device_GetConfigDescriptor(const Arguments& args){
	ENTER_METHOD(pDevice, 0);

	libusb_config_descriptor* cdesc;
	CHECK_USB(libusb_get_active_config_descriptor(self->device, &cdesc));

	Local<Object> v8cdesc = Object::New();

	STRUCT_TO_V8(v8cdesc, *cdesc, bLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bDescriptorType)
	STRUCT_TO_V8(v8cdesc, *cdesc, wTotalLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bNumInterfaces)
	STRUCT_TO_V8(v8cdesc, *cdesc, bConfigurationValue)
	STRUCT_TO_V8(v8cdesc, *cdesc, iConfiguration)
	STRUCT_TO_V8(v8cdesc, *cdesc, bmAttributes)
	STRUCT_TO_V8(v8cdesc, *cdesc, MaxPower)
	//r->Set(V8STR("extra"), makeBuffer(cd.extra, cd.extra_length), CONST_PROP);

	Local<Array> v8interfaces = Array::New(cdesc->bNumInterfaces);
	v8cdesc->Set(V8SYM("interfaces"), v8interfaces);

	for (int idxInterface = 0; idxInterface < cdesc->bNumInterfaces; idxInterface++) {
		int numAltSettings = cdesc->interface[idxInterface].num_altsetting;
		
		Local<Array> v8altsettings = Array::New(numAltSettings);
		v8interfaces->Set(idxInterface, v8altsettings);

		for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
			const libusb_interface_descriptor& idesc =
				cdesc->interface[idxInterface].altsetting[idxAltSetting];
			
			Local<Object> v8idesc = Object::New();
			v8altsettings->Set(idxAltSetting, v8idesc);

			STRUCT_TO_V8(v8idesc, idesc, bLength)
			STRUCT_TO_V8(v8idesc, idesc, bDescriptorType)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceNumber)
			STRUCT_TO_V8(v8idesc, idesc, bAlternateSetting)
			STRUCT_TO_V8(v8idesc, idesc, bNumEndpoints)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceClass)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceSubClass)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceProtocol)
			STRUCT_TO_V8(v8idesc, idesc, iInterface)
			// TODO: extra data

			Local<Array> v8endpoints = Array::New(idesc.bNumEndpoints);
			v8idesc->Set(V8SYM("endpoints"), v8endpoints, CONST_PROP);
			for (int idxEndpoint = 0; idxEndpoint < idesc.bNumEndpoints; idxEndpoint++){
				const libusb_endpoint_descriptor& edesc = idesc.endpoint[idxEndpoint];

				Local<Object> v8edesc = Object::New();
				v8endpoints->Set(idxEndpoint, v8edesc);

				STRUCT_TO_V8(v8edesc, edesc, bLength)
				STRUCT_TO_V8(v8edesc, edesc, bDescriptorType)
				STRUCT_TO_V8(v8edesc, edesc, bEndpointAddress)
				STRUCT_TO_V8(v8edesc, edesc, bmAttributes)
				STRUCT_TO_V8(v8edesc, edesc, bInterval)
				STRUCT_TO_V8(v8edesc, edesc, bRefresh)
				STRUCT_TO_V8(v8edesc, edesc, bSynchAddress)
			}
		}
	}

	libusb_free_config_descriptor(cdesc);
	return scope.Close(v8cdesc);
}

Handle<Value> Device_Open(const Arguments& args){
	ENTER_METHOD(pDevice, 0);
	if (!self->handle){
		CHECK_USB(libusb_open(self->device, &self->handle));
	}
	return scope.Close(Undefined());
}

Handle<Value> Device_Close(const Arguments& args){
	ENTER_METHOD(pDevice, 0);
	if (self->canClose()){
		libusb_close(self->handle);
		self->handle = NULL;
	}else{
		THROW_ERROR("Can't close device with a pending request");
	}
	return scope.Close(Undefined());
}

struct Req{
	uv_work_t req;
	Device* device;
	Persistent<Function> callback;
	int errcode;

	void submit(Device* d, Handle<Function> cb, uv_work_cb backend, uv_work_cb after){
		callback = Persistent<Function>::New(cb);
		device = d;
		device->ref();
		req.data = this;
		uv_queue_work(uv_default_loop(), &req, backend, (uv_after_work_cb) after);
	}

	static void default_after(uv_work_t *req){
		HandleScope scope;
		auto baton = (Req*) req->data;

		auto device = Local<Object>::New(baton->device->handle_);
		baton->device->unref();

		if (!baton->callback.IsEmpty()) {
			Handle<Value> error = Undefined();
			if (baton->errcode < 0){
				error = libusbException(baton->errcode);
			}
			Handle<Value> argv[1] = {error};
			TryCatch try_catch;
			baton->callback->Call(device, 1, argv);
			if (try_catch.HasCaught()) {
				FatalException(try_catch);
			}
			baton->callback.Dispose();
		}
		delete baton;
	}
};

struct Device_Reset: Req{
	static Handle<Value> begin(const Arguments& args){
		ENTER_METHOD(pDevice, 0);
		CHECK_OPEN();
		CALLBACK_ARG(0);
		auto baton = new Device_Reset;
		baton->submit(self, callback, &backend, &default_after);
		return scope.Close(Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_Reset*) req->data;
		baton->errcode = libusb_reset_device(baton->device->handle);
	}
};

Handle<Value> IsKernelDriverActive(const Arguments& args) {
	ENTER_METHOD(pDevice, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	int r = libusb_kernel_driver_active(self->handle, interface);
	CHECK_USB(r);
	return scope.Close(Boolean::New(r));
}	
	
Handle<Value> DetachKernelDriver(const Arguments& args) {
	ENTER_METHOD(pDevice, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_detach_kernel_driver(self->handle, interface));
	return Undefined();
}

Handle<Value> AttachKernelDriver(const Arguments& args) {
	ENTER_METHOD(pDevice, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_attach_kernel_driver(self->handle, interface));
	return Undefined();
}

Handle<Value> Device_ClaimInterface(const Arguments& args) {
	ENTER_METHOD(pDevice, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_claim_interface(self->handle, interface));
	return Undefined();
}

struct Device_ReleaseInterface: Req{
	int interface;

	static Handle<Value> begin(const Arguments& args){
		ENTER_METHOD(pDevice, 1);
		CHECK_OPEN();
		int interface;
		INT_ARG(interface, 0);
		CALLBACK_ARG(1);
		auto baton = new Device_ReleaseInterface;
		baton->interface = interface;
		baton->submit(self, callback, &backend, &default_after);

		return scope.Close(Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_ReleaseInterface*) req->data;
		baton->errcode = libusb_release_interface(baton->device->handle, baton->interface);
	}
};

struct Device_SetInterface: Req{
	int interface;
	int altsetting;

	static Handle<Value> begin(const Arguments& args){
		ENTER_METHOD(pDevice, 2);
		CHECK_OPEN();
		int interface, altsetting;
		INT_ARG(interface, 0);
		INT_ARG(altsetting, 1);
		CALLBACK_ARG(2);
		auto baton = new Device_SetInterface;
		baton->interface = interface;
		baton->altsetting = altsetting;
		baton->submit(self, callback, &backend, &default_after);
		return scope.Close(Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_SetInterface*) req->data;
		baton->errcode = libusb_set_interface_alt_setting(
			baton->device->handle, baton->interface, baton->altsetting);
	}
};

static void init(Handle<Object> target){
	pDevice.init(&deviceConstructor);
	pDevice.addMethod("__getConfigDescriptor", Device_GetConfigDescriptor);
	pDevice.addMethod("__open", Device_Open);
	pDevice.addMethod("__close", Device_Close);
	pDevice.addMethod("reset", Device_Reset::begin);
	
	pDevice.addMethod("__claimInterface", Device_ClaimInterface);
	pDevice.addMethod("__releaseInterface", Device_ReleaseInterface::begin);
	pDevice.addMethod("__setInterface", Device_SetInterface::begin);
	
	pDevice.addMethod("__isKernelDriverActive", IsKernelDriverActive);
	pDevice.addMethod("__detachKernelDriver", DetachKernelDriver);
	pDevice.addMethod("__attachKernelDriver", AttachKernelDriver);
}

Proto<Device> pDevice("Device", &init);
