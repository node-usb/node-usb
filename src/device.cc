#include "node_usb.h"
#include <string.h>

#define STRUCT_TO_V8(TARGET, STR, NAME) \
		TARGET->ForceSet(V8STR(#NAME), Nan::New<Uint32>((uint32_t) (STR).NAME).ToLocalChecked(), CONST_PROP);

#define CHECK_OPEN() \
		if (!self->device_handle){THROW_ERROR("Device is not opened");}

#define MAX_PORTS 7

static v8::Persistent<v8::FunctionTemplate> device_constructor;

Handle<Object> makeBuffer(const unsigned char* ptr, unsigned length) {
	return Nan::NewBuffer((char*) ptr, (uint32_t) length).ToLocalChecked();
}

Device::Device(libusb_device* d): device(d), device_handle(0) {
	libusb_ref_device(device);
	DEBUG_LOG("Created device %p", this);
}


Device::~Device(){
	DEBUG_LOG("Freed device %p", this);
	libusb_close(device_handle);
	libusb_unref_device(device);
}

// Map pinning each libusb_device to a particular V8 instance
std::map<libusb_device*, Nan::WeakCallbackInfo<std::pair<v8::Value, libusb_device*>>*> Device::byPtr;

void DeviceWeakCallback (const Nan::WeakCallbackInfo<std::pair<v8::Value, libusb_device*>> &data) {
	Device::unpin(data.GetParameter()->second);
}

// Get a V8 instance for a libusb_device: either the existing one from the map,
// or create a new one and add it to the map.
Local<Value> Device::get(libusb_device* dev){
	auto it = byPtr.find(dev);
	if (it != byPtr.end()){
		return Nan::New<v8::Value>(it->second->GetParameter()->first);
	}else{
		Local<FunctionTemplate> constructorHandle = Nan::New<v8::FunctionTemplate>(device_constructor).ToLocalChecked();
		v8::Local<v8::Value> argv[1] = { EXTERNAL_NEW(new Device(dev)) };
		v8::Local<v8::Value> v = constructorHandle->GetFunction()->NewInstance(1, argv);
		auto p = Nan::MakeWeakPersistent(v, dev, DeviceWeakCallback);
		byPtr.insert(std::make_pair(dev, std::make_pair(p, dev)));
		return v;
	}
}

void Device::unpin(libusb_device* device) {
	byPtr.erase(device);
	DEBUG_LOG("Removed cached device %p", device);
}

static NAN_METHOD(deviceConstructor) {
	ENTER_CONSTRUCTOR_POINTER(Device, 1);

	info.This()->ForceSet(V8SYM("busNumber"),
		Nan::New<Uint32>((uint32_t) libusb_get_bus_number(self->device)).ToLocalChecked(), CONST_PROP);
	info.This()->ForceSet(V8SYM("deviceAddress"),
		Nan::New<Uint32>((uint32_t) libusb_get_device_address(self->device)).ToLocalChecked(), CONST_PROP);

	Local<Object> v8dd = Nan::New<Object>().ToLocalChecked();
	info.This()->ForceSet(V8SYM("deviceDescriptor"), v8dd, CONST_PROP);

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

	uint8_t port_numbers[MAX_PORTS];
	int ret = libusb_get_port_numbers(self->device, &port_numbers[0], MAX_PORTS);
	CHECK_USB(ret);
	Local<Array> array = Nan::New<Array>(ret);
	for (int i = 0; i < ret; ++ i) {
		array->Set(i, Nan::New(port_numbers[i]).ToLocalChecked());
	}

	info.This()->ForceSet(V8SYM("portNumbers"), array, CONST_PROP);

	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Device_GetConfigDescriptor) {
	ENTER_METHOD(Device, 0);

	libusb_config_descriptor* cdesc;
	CHECK_USB(libusb_get_active_config_descriptor(self->device, &cdesc));

	Local<Object> v8cdesc = Nan::New<Object>().ToLocalChecked();

	STRUCT_TO_V8(v8cdesc, *cdesc, bLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bDescriptorType)
	STRUCT_TO_V8(v8cdesc, *cdesc, wTotalLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bNumInterfaces)
	STRUCT_TO_V8(v8cdesc, *cdesc, bConfigurationValue)
	STRUCT_TO_V8(v8cdesc, *cdesc, iConfiguration)
	STRUCT_TO_V8(v8cdesc, *cdesc, bmAttributes)
	// Libusb 1.0 typo'd bMaxPower as MaxPower
	v8cdesc->ForceSet(V8STR("bMaxPower"), Nan::New<Uint32>((uint32_t) cdesc->MaxPower).ToLocalChecked(), CONST_PROP);

	v8cdesc->ForceSet(V8SYM("extra"), makeBuffer(cdesc->extra, cdesc->extra_length), CONST_PROP);

	Local<Array> v8interfaces = Nan::New<Array>(cdesc->bNumInterfaces);
	v8cdesc->ForceSet(V8SYM("interfaces"), v8interfaces);

	for (int idxInterface = 0; idxInterface < cdesc->bNumInterfaces; idxInterface++) {
		int numAltSettings = cdesc->interface[idxInterface].num_altsetting;

		Local<Array> v8altsettings = Nan::New<Array>(numAltSettings);
		v8interfaces->Set(idxInterface, v8altsettings);

		for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
			const libusb_interface_descriptor& idesc =
				cdesc->interface[idxInterface].altsetting[idxAltSetting];

			Local<Object> v8idesc = Nan::New<Object>().ToLocalChecked();
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

			v8idesc->ForceSet(V8SYM("extra"), makeBuffer(idesc.extra, idesc.extra_length), CONST_PROP);

			Local<Array> v8endpoints = Nan::New<Array>(idesc.bNumEndpoints);
			v8idesc->ForceSet(V8SYM("endpoints"), v8endpoints, CONST_PROP);
			for (int idxEndpoint = 0; idxEndpoint < idesc.bNumEndpoints; idxEndpoint++){
				const libusb_endpoint_descriptor& edesc = idesc.endpoint[idxEndpoint];

				Local<Object> v8edesc = Nan::New<Object>().ToLocalChecked();
				v8endpoints->Set(idxEndpoint, v8edesc);

				STRUCT_TO_V8(v8edesc, edesc, bLength)
				STRUCT_TO_V8(v8edesc, edesc, bDescriptorType)
				STRUCT_TO_V8(v8edesc, edesc, bEndpointAddress)
				STRUCT_TO_V8(v8edesc, edesc, bmAttributes)
				STRUCT_TO_V8(v8edesc, edesc, wMaxPacketSize)
				STRUCT_TO_V8(v8edesc, edesc, bInterval)
				STRUCT_TO_V8(v8edesc, edesc, bRefresh)
				STRUCT_TO_V8(v8edesc, edesc, bSynchAddress)

				v8edesc->ForceSet(V8SYM("extra"), makeBuffer(edesc.extra, edesc.extra_length), CONST_PROP);
			}
		}
	}

	libusb_free_config_descriptor(cdesc);
	info.GetReturnValue().Set(v8cdesc);
}

NAN_METHOD(Device_Open) {
	ENTER_METHOD(Device, 0);
	if (!self->device_handle){
		CHECK_USB(libusb_open(self->device, &self->device_handle));
	}
}

NAN_METHOD(Device_Close) {
	ENTER_METHOD(Device, 0);
	if (self->canClose()){
		libusb_close(self->device_handle);
		self->device_handle = NULL;
	}else{
		THROW_ERROR("Can't close device with a pending request");
	}
}

struct Req{
	uv_work_t req;
	Device* device;
	Persistent<Function> callback;
	int errcode;

	void submit(Device* d, Handle<Function> cb, uv_work_cb backend, uv_work_cb after){
		NanAssignPersistent(callback, cb);
		device = d;
		device->ref();
		req.data = this;
		uv_queue_work(uv_default_loop(), &req, backend, (uv_after_work_cb) after);
	}

	static void default_after(uv_work_t *req){
		Nan::HandleScope scope;
		auto baton = (Req*) req->data;

		auto device = NanObjectWrapHandle(baton->device);
		baton->device->unref();

		if (!Nan::New(baton->callback).ToLocalChecked().IsEmpty()) {
			Handle<Value> error = NanUndefined();
			if (baton->errcode < 0){
				error = libusbException(baton->errcode);
			}
			Handle<Value> argv[1] = {error};
			TryCatch try_catch;
			Nan::MakeCallback(device, Nan::New(baton->callback).ToLocalChecked(), 1, argv);
			if (try_catch.HasCaught()) {
				FatalException(try_catch);
			}
			NanDisposePersistent(baton->callback);
		}
		delete baton;
	}
};

struct Device_Reset: Req{
	static NAN_METHOD(begin) {
		ENTER_METHOD(Device, 0);
		CHECK_OPEN();
		CALLBACK_ARG(0);
		auto baton = new Device_Reset;
		baton->submit(self, callback, &backend, &default_after);
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_Reset*) req->data;
		baton->errcode = libusb_reset_device(baton->device->device_handle);
	}
};

NAN_METHOD(IsKernelDriverActive) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	int r = libusb_kernel_driver_active(self->device_handle, interface);
	CHECK_USB(r);
	info.GetReturnValue().Set(Nan::New<Boolean>(r));
}

NAN_METHOD(DetachKernelDriver) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_detach_kernel_driver(self->device_handle, interface));
}

NAN_METHOD(AttachKernelDriver) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_attach_kernel_driver(self->device_handle, interface));
}

NAN_METHOD(Device_ClaimInterface) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_claim_interface(self->device_handle, interface));
}

struct Device_ReleaseInterface: Req{
	int interface;

	static NAN_METHOD(begin){
		ENTER_METHOD(Device, 1);
		CHECK_OPEN();
		int interface;
		INT_ARG(interface, 0);
		CALLBACK_ARG(1);
		auto baton = new Device_ReleaseInterface;
		baton->interface = interface;
		baton->submit(self, callback, &backend, &default_after);

	}

	static void backend(uv_work_t *req){
		auto baton = (Device_ReleaseInterface*) req->data;
		baton->errcode = libusb_release_interface(baton->device->device_handle, baton->interface);
	}
};

struct Device_SetInterface: Req{
	int interface;
	int altsetting;

	static NAN_METHOD(begin){
		ENTER_METHOD(Device, 2);
		CHECK_OPEN();
		int interface, altsetting;
		INT_ARG(interface, 0);
		INT_ARG(altsetting, 1);
		CALLBACK_ARG(2);
		auto baton = new Device_SetInterface;
		baton->interface = interface;
		baton->altsetting = altsetting;
		baton->submit(self, callback, &backend, &default_after);
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_SetInterface*) req->data;
		baton->errcode = libusb_set_interface_alt_setting(
			baton->device->device_handle, baton->interface, baton->altsetting);
	}
};

void Device::Init(Handle<Object> target){
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(deviceConstructor).ToLocalChecked();
	tpl->SetClassName(Nan::New("Device").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "__getConfigDescriptor", Device_GetConfigDescriptor);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__open", Device_Open);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__close", Device_Close);
	NODE_SET_PROTOTYPE_METHOD(tpl, "reset", Device_Reset::begin);

	NODE_SET_PROTOTYPE_METHOD(tpl, "__claimInterface", Device_ClaimInterface);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__releaseInterface", Device_ReleaseInterface::begin);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__setInterface", Device_SetInterface::begin);

	NODE_SET_PROTOTYPE_METHOD(tpl, "__isKernelDriverActive", IsKernelDriverActive);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__detachKernelDriver", DetachKernelDriver);
	NODE_SET_PROTOTYPE_METHOD(tpl, "__attachKernelDriver", AttachKernelDriver);

	NanAssignPersistent(device_constructor, tpl);
	target->Set(Nan::New("Device").ToLocalChecked(), tpl->GetFunction());
}
