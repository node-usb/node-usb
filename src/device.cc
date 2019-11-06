#include "node_usb.h"
#include <string.h>

#define STRUCT_TO_V8(TARGET, STR, NAME) \
	Nan::DefineOwnProperty(TARGET, V8STR(#NAME), Nan::New<Uint32>((uint32_t) (STR).NAME), CONST_PROP);

#define CHECK_OPEN() \
	if (!self->device_handle){THROW_ERROR("Device is not opened");}

#define MAX_PORTS 7

static Nan::Persistent<v8::FunctionTemplate> device_constructor;

Device::Device(libusb_device* d): device(d), device_handle(0) {
	libusb_ref_device(device);
	byPtr.insert(std::make_pair(d, this));
	DEBUG_LOG("Created device %p", this);
}

Device::~Device(){
	DEBUG_LOG("Freed device %p", this);
	byPtr.erase(device);
	libusb_close(device_handle);
	libusb_unref_device(device);
}

// Map pinning each libusb_device to a particular V8 instance
std::map<libusb_device*, Device*> Device::byPtr;

// Get a V8 instance for a libusb_device: either the existing one from the map,
// or create a new one and add it to the map.
Local<Object> Device::get(libusb_device* dev){
	auto it = byPtr.find(dev);
	if (it != byPtr.end()){
		return it->second->handle();
	} else {
		Local<FunctionTemplate> constructorHandle = Nan::New<v8::FunctionTemplate>(device_constructor);
		Local<Value> argv[1] = { EXTERNAL_NEW(new Device(dev)) };
		Local<Object> obj = Nan::NewInstance(Nan::GetFunction(constructorHandle).ToLocalChecked(), 1, argv).ToLocalChecked();
		return obj;
	}
}

static NAN_METHOD(deviceConstructor) {
	ENTER_CONSTRUCTOR_POINTER(Device, 1);

	Nan::DefineOwnProperty(info.This(), V8SYM("busNumber"),
		Nan::New<Uint32>((uint32_t) libusb_get_bus_number(self->device)), CONST_PROP);
	Nan::DefineOwnProperty(info.This(), V8SYM("deviceAddress"),
		Nan::New<Uint32>((uint32_t) libusb_get_device_address(self->device)), CONST_PROP);

	Local<Object> v8dd = Nan::New<Object>();
	Nan::DefineOwnProperty(info.This(), V8SYM("deviceDescriptor"), v8dd, CONST_PROP);

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
	if (ret > 0) {
		Local<Array> array = Nan::New<Array>(ret);
		for (int i = 0; i < ret; ++ i) {
			Nan::Set(array, i, Nan::New(port_numbers[i]));
		}
		Nan::DefineOwnProperty(info.This(), V8SYM("portNumbers"), array, CONST_PROP);
	}
	info.GetReturnValue().Set(info.This());
}

Local<Object> Device::cdesc2V8(libusb_config_descriptor * cdesc){
	Local<Object> v8cdesc = Nan::New<Object>();

	STRUCT_TO_V8(v8cdesc, *cdesc, bLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bDescriptorType)
	STRUCT_TO_V8(v8cdesc, *cdesc, wTotalLength)
	STRUCT_TO_V8(v8cdesc, *cdesc, bNumInterfaces)
	STRUCT_TO_V8(v8cdesc, *cdesc, bConfigurationValue)
	STRUCT_TO_V8(v8cdesc, *cdesc, iConfiguration)
	STRUCT_TO_V8(v8cdesc, *cdesc, bmAttributes)
	// Libusb 1.0 typo'd bMaxPower as MaxPower
	Nan::DefineOwnProperty(v8cdesc, V8STR("bMaxPower"), Nan::New<Uint32>((uint32_t) cdesc->MaxPower), CONST_PROP);

	Nan::DefineOwnProperty(v8cdesc, V8SYM("extra"),
		Nan::CopyBuffer((const char*) cdesc->extra, cdesc->extra_length).ToLocalChecked(),
		CONST_PROP);

	Local<Array> v8interfaces = Nan::New<Array>(cdesc->bNumInterfaces);
	Nan::DefineOwnProperty(v8cdesc, V8SYM("interfaces"), v8interfaces);

	for (int idxInterface = 0; idxInterface < cdesc->bNumInterfaces; idxInterface++) {
		int numAltSettings = cdesc->interface[idxInterface].num_altsetting;

		Local<Array> v8altsettings = Nan::New<Array>(numAltSettings);
		Nan::Set(v8interfaces, idxInterface, v8altsettings);

		for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
			const libusb_interface_descriptor& idesc =
				cdesc->interface[idxInterface].altsetting[idxAltSetting];

			Local<Object> v8idesc = Nan::New<Object>();
			Nan::Set(v8altsettings, idxAltSetting, v8idesc);

			STRUCT_TO_V8(v8idesc, idesc, bLength)
			STRUCT_TO_V8(v8idesc, idesc, bDescriptorType)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceNumber)
			STRUCT_TO_V8(v8idesc, idesc, bAlternateSetting)
			STRUCT_TO_V8(v8idesc, idesc, bNumEndpoints)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceClass)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceSubClass)
			STRUCT_TO_V8(v8idesc, idesc, bInterfaceProtocol)
			STRUCT_TO_V8(v8idesc, idesc, iInterface)

			Nan::DefineOwnProperty(v8idesc, V8SYM("extra"),
				Nan::CopyBuffer((const char*)idesc.extra, idesc.extra_length).ToLocalChecked(),
				CONST_PROP);

			Local<Array> v8endpoints = Nan::New<Array>(idesc.bNumEndpoints);
			Nan::DefineOwnProperty(v8idesc, V8SYM("endpoints"), v8endpoints, CONST_PROP);
			for (int idxEndpoint = 0; idxEndpoint < idesc.bNumEndpoints; idxEndpoint++){
				const libusb_endpoint_descriptor& edesc = idesc.endpoint[idxEndpoint];

				Local<Object> v8edesc = Nan::New<Object>();
				Nan::Set(v8endpoints, idxEndpoint, v8edesc);

				STRUCT_TO_V8(v8edesc, edesc, bLength)
				STRUCT_TO_V8(v8edesc, edesc, bDescriptorType)
				STRUCT_TO_V8(v8edesc, edesc, bEndpointAddress)
				STRUCT_TO_V8(v8edesc, edesc, bmAttributes)
				STRUCT_TO_V8(v8edesc, edesc, wMaxPacketSize)
				STRUCT_TO_V8(v8edesc, edesc, bInterval)
				STRUCT_TO_V8(v8edesc, edesc, bRefresh)
				STRUCT_TO_V8(v8edesc, edesc, bSynchAddress)

				Nan::DefineOwnProperty(v8edesc, V8SYM("extra"),
					Nan::CopyBuffer((const char*) edesc.extra, edesc.extra_length).ToLocalChecked(),
					CONST_PROP);
			}
		}
	}
	return v8cdesc;
}

NAN_METHOD(Device_GetConfigDescriptor) {
	ENTER_METHOD(Device, 0);
	libusb_config_descriptor* cdesc;
	CHECK_USB(libusb_get_active_config_descriptor(self->device, &cdesc));
	Local<Object> v8cdesc = Device::cdesc2V8(cdesc);
	libusb_free_config_descriptor(cdesc);
	info.GetReturnValue().Set(v8cdesc);
}

NAN_METHOD(Device_GetAllConfigDescriptors){
	ENTER_METHOD(Device, 0);
	libusb_config_descriptor * cdesc;
	struct libusb_device_descriptor dd;
	libusb_get_device_descriptor(self->device, &dd);
	Local<Array> v8cdescriptors = Nan::New<Array>(dd.bNumConfigurations);
	for(uint8_t i = 0; i < dd.bNumConfigurations; i++){
		libusb_get_config_descriptor(self->device, i, &cdesc);
		Nan::Set(v8cdescriptors, i, Device::cdesc2V8(cdesc));
		libusb_free_config_descriptor(cdesc);
	}
	info.GetReturnValue().Set(v8cdescriptors);
}

NAN_METHOD(Device_GetParent){
	ENTER_METHOD(Device, 0);
	libusb_device* dev = libusb_get_parent(self->device);
	if(dev)
		info.GetReturnValue().Set(Device::get(dev));
	else
		info.GetReturnValue().Set(Nan::Null());
}

NAN_METHOD(Device_Open) {
	ENTER_METHOD(Device, 0);
	if (!self->device_handle){
		CHECK_USB(libusb_open(self->device, &self->device_handle));
	}
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Device_Close) {
	ENTER_METHOD(Device, 0);
	if (self->canClose()){
		libusb_close(self->device_handle);
		self->device_handle = NULL;
	}else{
		THROW_ERROR("Can't close device with a pending request");
	}
	info.GetReturnValue().Set(Nan::Undefined());
}

struct Req{
	uv_work_t req;
	Device* device;
	Nan::Persistent<Function> callback;
	int errcode;

	void submit(Device* d, Local<Function> cb, uv_work_cb backend, uv_work_cb after){
		callback.Reset(cb);
		device = d;
		device->ref();
		req.data = this;
		uv_queue_work(uv_default_loop(), &req, backend, (uv_after_work_cb) after);
	}

	static void default_after(uv_work_t *req){
		Nan::HandleScope scope;
		auto baton = (Req*) req->data;

		auto device = baton->device->handle();
		baton->device->unref();

		if (!Nan::New(baton->callback).IsEmpty()) {
			Local<Value> error = Nan::Undefined();
			if (baton->errcode < 0){
				error = libusbException(baton->errcode);
			}
			Local<Value> argv[1] = {error};
			Nan::TryCatch try_catch;
			Nan::MakeCallback(device, Nan::New(baton->callback), 1, argv);
			if (try_catch.HasCaught()) {
				Nan::FatalException(try_catch);
			}
			baton->callback.Reset();
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
		info.GetReturnValue().Set(Nan::Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_Reset*) req->data;
		baton->errcode = libusb_reset_device(baton->device->device_handle);
	}
};

struct Device_Clear_Halt: Req{
	int endpoint;

        static NAN_METHOD(begin) {
		int endpoint;
		ENTER_METHOD(Device, 1);
		CHECK_OPEN();
		INT_ARG(endpoint, 0);
		CALLBACK_ARG(1);
		auto baton = new Device_Clear_Halt;
		baton->endpoint = endpoint;
                baton->submit(self, callback, &backend, &default_after);
                info.GetReturnValue().Set(Nan::Undefined());
        }

        static void backend(uv_work_t *req){
                auto baton = (Device_Clear_Halt*) req->data;
                baton->errcode = libusb_clear_halt(baton->device->device_handle, baton->endpoint);
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
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(AttachKernelDriver) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_attach_kernel_driver(self->device_handle, interface));
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Device_ClaimInterface) {
	ENTER_METHOD(Device, 1);
	CHECK_OPEN();
	int interface;
	INT_ARG(interface, 0);
	CHECK_USB(libusb_claim_interface(self->device_handle, interface));
	info.GetReturnValue().Set(Nan::Undefined());
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

		info.GetReturnValue().Set(Nan::Undefined());
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
		info.GetReturnValue().Set(Nan::Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_SetInterface*) req->data;
		baton->errcode = libusb_set_interface_alt_setting(
			baton->device->device_handle, baton->interface, baton->altsetting);
	}
};

struct Device_SetConfiguration: Req{
	int desired;

	static NAN_METHOD(begin){
		ENTER_METHOD(Device, 1);
		CHECK_OPEN();
		int desired;
		INT_ARG(desired, 0);
		CALLBACK_ARG(1);
		auto baton = new Device_SetConfiguration;
		baton->desired = desired;
		baton->submit(self, callback, &backend, &default_after);
		info.GetReturnValue().Set(Nan::Undefined());
	}

	static void backend(uv_work_t *req){
		auto baton = (Device_SetConfiguration*) req->data;
		baton->errcode = libusb_set_configuration(
			baton->device->device_handle, baton->desired);
	}
};

void Device::Init(Local<Object> target){
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(deviceConstructor);
	tpl->SetClassName(Nan::New("Device").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "__getParent", Device_GetParent);
	Nan::SetPrototypeMethod(tpl, "__getConfigDescriptor", Device_GetConfigDescriptor);
	Nan::SetPrototypeMethod(tpl, "__getAllConfigDescriptors", Device_GetAllConfigDescriptors);
	Nan::SetPrototypeMethod(tpl, "__open", Device_Open);
	Nan::SetPrototypeMethod(tpl, "__close", Device_Close);
	Nan::SetPrototypeMethod(tpl, "__clearHalt", Device_Clear_Halt::begin);
	Nan::SetPrototypeMethod(tpl, "reset", Device_Reset::begin);

	Nan::SetPrototypeMethod(tpl, "__claimInterface", Device_ClaimInterface);
	Nan::SetPrototypeMethod(tpl, "__releaseInterface", Device_ReleaseInterface::begin);
	Nan::SetPrototypeMethod(tpl, "__setInterface", Device_SetInterface::begin);
	Nan::SetPrototypeMethod(tpl, "__setConfiguration", Device_SetConfiguration::begin);

	Nan::SetPrototypeMethod(tpl, "__isKernelDriverActive", IsKernelDriverActive);
	Nan::SetPrototypeMethod(tpl, "__detachKernelDriver", DetachKernelDriver);
	Nan::SetPrototypeMethod(tpl, "__attachKernelDriver", AttachKernelDriver);

	device_constructor.Reset(tpl);
	Nan::Set(target, Nan::New("Device").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}
