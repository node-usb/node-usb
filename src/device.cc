#include "bindings.h"
#include "device.h"
#include "interface.h"
#include "transfer.h"
#include <assert.h>

namespace NodeUsb {
	/** constructor template is needed for creating new Device objects from outside */
	Persistent<FunctionTemplate> Device::constructor_template;

	/**
	 * @param device.busNumber integer
	 * @param device.deviceAddress integer
	 */
	void Device::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Device::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Device"));
		Device::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("deviceAddress"), Device::DeviceAddressGetter);
		instance_template->SetAccessor(V8STR("busNumber"), Device::BusNumberGetter);
		instance_template->SetAccessor(V8SYM("timeout"), Device::TimeoutGetter, Device::TimeoutSetter);
		instance_template->SetAccessor(V8STR("configDescriptor"), Device::ConfigDescriptorGetter);
		instance_template->SetAccessor(V8STR("interfaces"), Device::InterfacesGetter);

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "reset", Device::Reset);
		NODE_SET_PROTOTYPE_METHOD(t, "controlTransfer", Device::ControlTransfer);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Device"), t->GetFunction());	
		
		Usb::LoadDevices();
		
		DEBUG("Leave")
	}

	Device::Device(libusb_device* _device) : 
		ObjectWrap(), device(_device), handle(NULL), config_descriptor(NULL), timeout(1000) {
		DEBUG_OPT("Device object %p created", device)
	}

	Device::~Device() {
		DEBUG_OPT("Device object %p destroyed", device)
		// free configuration descriptor
		close();
		v8ConfigDescriptor.Dispose();
		v8Interfaces.Dispose();
		libusb_free_config_descriptor(config_descriptor);
		libusb_unref_device(device);
	}
	
	int Device::openHandle() {
		if (handle) return LIBUSB_SUCCESS;
		return libusb_open(device, &handle);
	}
	
	void Device::close(){
		libusb_close(handle);
		handle = NULL;
	}
	
	Handle<Value> Device::New(const Arguments& args) {
		HandleScope scope;

		if (!AllowConstructor::Check()) THROW_ERROR("Illegal constructor")

		// need libusb_device structure as first argument
		if (args.Length() < 1 || !args[0]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be external!")
		}

		Local<External> refDevice = Local<External>::Cast(args[0]);

		// cast local reference to local libusb_device structure
		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());

		// create new Device object
		Device *device = new Device(libusbDevice);

		// wrap created Device object to v8
		device->Wrap(args.This());

		#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(NAME) \
			dd->Set(V8SYM(#NAME), Uint32::New(device->device_descriptor.NAME), CONST_PROP);

		assert(device->device != NULL);
		CHECK_USB(libusb_get_device_descriptor(device->device, &device->device_descriptor), scope);

		Local<Object> dd = Object::New();

		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bcdUSB)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceClass)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceSubClass)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bDeviceProtocol)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bMaxPacketSize0)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(idVendor)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(idProduct)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bcdDevice)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iManufacturer)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iProduct)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(iSerialNumber)
		LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(bNumConfigurations)
		
		args.This()->Set(V8SYM("deviceDescriptor"), dd, CONST_PROP);
		
		return args.This();
	}
	
	/**
	 * @return integer
	 */
	Handle<Value> Device::BusNumberGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t bus_number = libusb_get_bus_number(self->device);

		return scope.Close(Integer::New(bus_number));
	}

	/**
	 * @return integer
	 */
	Handle<Value> Device::DeviceAddressGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t address = libusb_get_device_address(self->device);

		return scope.Close(Integer::New(address));
	}
	
	Handle<Value> Device::TimeoutGetter(Local<String> property, const AccessorInfo &info){
		LOCAL(Device, self, info.Holder())
		return scope.Close(Integer::New(self->timeout));
	}
	
	void Device::TimeoutSetter(Local<String> property, Local<Value> value, const AccessorInfo &info){
		LOCAL(Device, self, info.Holder())
		
		if (value->IsNumber()){
			self->timeout = value->Int32Value();
		}else{
			ThrowException(Exception::TypeError(V8STR("Timeout must be number")));
		}
	}

	/**
	 * libusb_reset_device incurs a noticeable delay, so this is asynchronous
	 */
	Handle<Value> Device::Reset(const Arguments& args) {
		LOCAL(Device, self, args.This())
		
		// allocation of intermediate EIO structure
		EIO_NEW(device_request, reset_req)

		// create default delegation
		EIO_DELEGATION(reset_req, 0)
		
		reset_req->device = self;
		reset_req->device->Ref();

		EIO_CUSTOM(EIO_Reset, reset_req, EIO_After_Reset);

		return Undefined();
	}

	/**
	 * Contains the blocking libusb_reset_device function
	 */
	void Device::EIO_Reset(uv_work_t *req) {
		// Inside EIO Threadpool, so don't touch V8.
		// Be careful!
		EIO_CAST(device_request, reset_req)
		
		libusb_device *device = reset_req->device->device;
		libusb_device_handle *handle;

		int errcode = 0;
		
		if ((errcode = libusb_open(device, &handle)) >= LIBUSB_SUCCESS) {
			if ((errcode = libusb_reset_device(handle)) < LIBUSB_SUCCESS) {
				libusb_close(handle);
				reset_req->errsource = "reset";
			}
		} else {
			reset_req->errsource = "open";
		}

		EIO_AFTER(reset_req, device)
	}
	
	void Device::EIO_After_Reset(uv_work_t *req) {
		TRANSFER_REQUEST_FREE(device_request, device)
	}
	

// TODO: Read-Only
#define LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(NAME) \
		r->Set(V8STR(#NAME), Uint32::New(self->config_descriptor->NAME), CONST_PROP);

#define LIBUSB_GET_CONFIG_DESCRIPTOR(scope) \
		DEBUG("Get active config descriptor"); \
		if (!self->config_descriptor) { \
			CHECK_USB(libusb_get_active_config_descriptor(self->device, &(self->config_descriptor)), scope) \
		}

	/**
	 * Returns configuration descriptor structure
	 */
	Handle<Value> Device::ConfigDescriptorGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())

		LIBUSB_GET_CONFIG_DESCRIPTOR(scope);
		
		if (self->v8ConfigDescriptor.IsEmpty()){
			Local<Object> r = Object::New();

			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bLength)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(wTotalLength)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bNumInterfaces)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bConfigurationValue)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(iConfiguration)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
			LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(MaxPower)
			r->Set(V8STR("extra"), makeBuffer(self->config_descriptor->extra, self->config_descriptor->extra_length), CONST_PROP);
			
			self->v8ConfigDescriptor = Persistent<Object>::New(r);
		}

		return scope.Close(self->v8ConfigDescriptor);
	}
	
	Handle<Value> Device::InterfacesGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())

		if (self->v8Interfaces.IsEmpty()){
			LIBUSB_GET_CONFIG_DESCRIPTOR(scope);
			AllowConstructor allow;

			self->v8Interfaces = Persistent<Array>::New(Array::New());
			int idx = 0;

			// iterate interfaces
			int numInterfaces = self->config_descriptor->bNumInterfaces;

			for (int idxInterface = 0; idxInterface < numInterfaces; idxInterface++) {
				int numAltSettings = ((*self->config_descriptor).interface[idxInterface]).num_altsetting;

				for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
					Local<Value> args_new_interface[3] = {
						info.Holder(),
						Uint32::New(idxInterface),
						Uint32::New(idxAltSetting),
					};

					self->v8Interfaces->Set(idx++,
						Interface::constructor_template->GetFunction()->NewInstance(3, args_new_interface)
					);
				}
			}
		}
		
		return scope.Close(self->v8Interfaces);
	}
	
	/**
	 * Sends control transfer commands to device
	 * @param Array|int data to send OR bytes to retrieve
	 * @param uint8_t bmRequestType
	 * @param uint8_t bRequest
	 * @param uint16_t wValue
	 * @param uint16_t wIndex
	 * @param function callback handler (data, status)
	 * @param int timeout (optional)
	 */
	Handle<Value> Device::ControlTransfer(const Arguments& args) {
		LOCAL(Device, self, args.This())
		
		CHECK_USB(self->openHandle(), scope);
		
		uint8_t bmRequestType, bRequest;
		uint16_t wValue, wIndex;
		unsigned length;
		unsigned char *buf;
		
		//args: bmRequestType, bRequest, wValue, wIndex, array/size, callback
		
		if (args.Length() < 6 || !args[5]->IsFunction()) {
			THROW_BAD_ARGS("Transfer missing arguments!")
		}
		
		INT_ARG(bmRequestType, args[0]);
		INT_ARG(bRequest, args[1]);
		INT_ARG(wValue, args[2]);
		INT_ARG(wIndex, args[3]);
		BUF_LEN_ARG(args[4]);
		
		if ((bmRequestType & 0x80) != modus){
			if ((bmRequestType & 0x80) == LIBUSB_ENDPOINT_IN){
				THROW_BAD_ARGS("Expected size number for IN transfer (based on bmRequestType)");
			}else{
				THROW_BAD_ARGS("Expected buffer for OUT transfer (based on bmRequestType)");			
			}	
		}
		
		DEBUG_OPT("Submitting control transfer %x %x %x %x %x (%i: %p)", modus, bmRequestType, bRequest, wValue, wIndex, length, buf);
		
		Transfer* transfer = Transfer::newControlTransfer(
			args.This(),
			bmRequestType,
		    bRequest,
			wValue,
			wIndex,
			buf,
			length,
			self->timeout,
			Handle<Function>::Cast(args[5]));
		
		transfer->submit();

		return Undefined();
	}

}
