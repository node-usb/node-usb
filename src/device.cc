#include "bindings.h"
#include "device.h"
#include "interface.h"
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

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "reset", Device::Reset); 
		NODE_SET_PROTOTYPE_METHOD(t, "ref", Device::AddReference); 
		NODE_SET_PROTOTYPE_METHOD(t, "unref", Device::RemoveReference); 
		NODE_SET_PROTOTYPE_METHOD(t, "getDeviceDescriptor", Device::GetDeviceDescriptor);
		NODE_SET_PROTOTYPE_METHOD(t, "getConfigDescriptor", Device::GetConfigDescriptor);
		NODE_SET_PROTOTYPE_METHOD(t, "getInterfaces", Device::GetInterfaces);
		NODE_SET_PROTOTYPE_METHOD(t, "getExtraData", Device::GetExtraData);
		NODE_SET_PROTOTYPE_METHOD(t, "controlTransfer", Device::ControlTransfer);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Device"), t->GetFunction());	
		DEBUG("Leave")
	}

	Device::Device(Handle<Object> _usb, libusb_device* _device) {
		usb = Persistent<Object>::New(_usb); // Ensure USB is around for the duration of this Device object

		device_container = (nodeusb_device_container*)malloc(sizeof(nodeusb_device_container));
		device_container->device = _device;
		device_container->handle = NULL;
		device_container->config_descriptor = NULL;
		device_container->handle_status = UNINITIALIZED;
		device_container->last_error = 0;

		DEBUG_OPT("Device object %p created", device_container->device)
	}

	Device::~Device() {
		DEBUG_OPT("Device object %p destroyed", device_container->device)
		// free configuration descriptor
		if (device_container) {
			libusb_free_config_descriptor(device_container->config_descriptor);
			free(device_container);
		}
		usb.Dispose();
	}
	
	Handle<Value> Device::New(const Arguments& args) {
		HandleScope scope;

		// need libusb_device structure as first argument
		if (args.Length() < 2) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be external!")
		}

		if (!args[0]->IsObject()) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be Object!")
		}

		if (!args[1]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be external!")
		}

		Local<Object> usb = Local<Object>::Cast(args[0]);
		Local<External> refDevice = Local<External>::Cast(args[1]);

		// cast local reference to local libusb_device structure
		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());

		// create new Device object
		Device *device = new Device(usb, libusbDevice);

		// wrap created Device object to v8
		device->Wrap(args.This());

		return args.This();
	}
	
	/**
	 * @return integer
	 */
	Handle<Value> Device::BusNumberGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t bus_number = libusb_get_bus_number(self->device_container->device);

		return scope.Close(Integer::New(bus_number));
	}

	/**
	 * @return integer
	 */
	Handle<Value> Device::DeviceAddressGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Device, self, info.Holder())
		uint8_t address = libusb_get_device_address(self->device_container->device);

		return scope.Close(Integer::New(address));
	}

	Handle<Value> Device::AddReference(const Arguments& args) {
		LOCAL(Device, self, args.This())
		libusb_ref_device(self->device_container->device);
		
		return Undefined();
	}

	Handle<Value> Device::RemoveReference(const Arguments& args) {
		LOCAL(Device, self, args.This())
		libusb_unref_device(self->device_container->device);
		
		return Undefined();
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

		// Make asynchronous call
		eio_custom(EIO_Reset, EIO_PRI_DEFAULT, EIO_After_Reset, reset_req);

		// add reference
		ev_ref(EV_DEFAULT_UC);
		
		return Undefined();
	}

	/**
	 * Contains the blocking libusb_reset_device function
	 */
	int Device::EIO_Reset(eio_req *req) {
		// Inside EIO Threadpool, so don't touch V8.
		// Be careful!

		EIO_CAST(device_request, reset_req)
		
		libusb_device *device = reset_req->device->device_container->device;
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

		reset_req->errcode = errcode;
		
		// needed for EIO so that the EIO_After_Reset method will be called
		req->result = 0;
		
		return 0;
	}

	int Device::EIO_After_Reset(eio_req *req) {
		TRANSFER_REQUEST_FREE(device_request, device)
	}
	

// TODO: Read-Only
#define LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Uint32::New((*container->config_descriptor).name));

#define LIBUSB_GET_CONFIG_DESCRIPTOR(scope) \
		DEBUG("Get active config descriptor"); \
		struct nodeusb_device_container *container = self->device_container; \
		if (!container->config_descriptor) { \
			CHECK_USB(libusb_get_active_config_descriptor(container->device, &(container->config_descriptor)), scope) \
		}

	/**
	 * Returns configuration descriptor structure
	 */
	Handle<Value> Device::GetConfigDescriptor(const Arguments& args) {
		// make local value reference to first parameter
		Local<External> refDevice = Local<External>::Cast(args[0]);

		LOCAL(Device, self, args.This())

#if defined(__APPLE__) && defined(__MACH__)
		DEBUG("Open device handle for getConfigDescriptor (Darwin fix)")
		OPEN_DEVICE_HANDLE_NEEDED(scope)
#endif

		LIBUSB_GET_CONFIG_DESCRIPTOR(scope);

		Local<Object> r = Object::New();
		DEBUG("Converting structure");

		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(wTotalLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bNumInterfaces)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bConfigurationValue)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(iConfiguration)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(MaxPower)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		return scope.Close(r);
	}
	
	Handle<Value> Device::GetExtraData(const Arguments& args) {
		LOCAL(Device, self, args.This())

		LIBUSB_GET_CONFIG_DESCRIPTOR(scope);

		int m = (*self->device_container->config_descriptor).extra_length;
		
		Local<Array> r = Array::New(m);
		
		for (int i = 0; i < m; i++) {
		  uint32_t c = (*self->device_container->config_descriptor).extra[i];
		  
		  r->Set(i, Uint32::New(c));
		}
		
		return scope.Close(r);
	}
	
	Handle<Value> Device::GetInterfaces(const Arguments& args) {
		LOCAL(Device, self, args.This())
#if defined(__APPLE__) && defined(__MACH__)
		DEBUG("Open device handle for GetInterfacse (Darwin fix)")
		OPEN_DEVICE_HANDLE_NEEDED(scope)
#endif

		LIBUSB_GET_CONFIG_DESCRIPTOR(scope);

		Local<Array> r = Array::New();
		int idx = 0;

		// iterate interfaces
		int numInterfaces = (*self->device_container->config_descriptor).bNumInterfaces;

		for (int idxInterface = 0; idxInterface < numInterfaces; idxInterface++) {
			int numAltSettings = ((*self->device_container->config_descriptor).interface[idxInterface]).num_altsetting;

			for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
				// passing a pointer of libusb_interface_descriptor does not work. struct is lost by V8
				// idx of interface and alt_setting is passed so that Interface class can extract the given interface
				Local<Value> args_new_interface[4] = {
					args.This(),
					External::New(self->device_container),
					Uint32::New(idxInterface),
					Uint32::New(idxAltSetting),
				};

				// create new object instance of class NodeUsb::Interface  
				r->Set(idx++, Interface::constructor_template->GetFunction()->NewInstance(4, args_new_interface));
			}
		}
		
		return scope.Close(r);
	}

// TODO: Read-Only
#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Uint32::New(self->device_descriptor.name));

#define LIBUSB_GET_DEVICE_DESCRIPTOR(scope) \
		DEBUG("Get device descriptor"); \
		assert(self->device_container->device != NULL); \
		CHECK_USB(libusb_get_device_descriptor(self->device_container->device, &(self->device_descriptor)), scope) \

	/**
	 * Returns the device descriptor of current device
	 * @return object
	 */
	Handle<Value> Device::GetDeviceDescriptor(const Arguments& args) {
		DEBUG("Entering")
		LOCAL(Device, self, args.This())

#if defined(__APPLE__) && defined(__MACH__)
		DEBUG("Open device handle for getDeviceDescriptor (Darwin fix)")
		OPEN_DEVICE_HANDLE_NEEDED(scope)
#endif

		LIBUSB_GET_DEVICE_DESCRIPTOR(scope);

		Local<Object> r = Object::New();

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

		return scope.Close(r);
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
		INIT_TRANSFER_CALL(6, 5, 7)
		
		OPEN_DEVICE_HANDLE_NEEDED(scope)
		EIO_NEW(control_transfer_request, control_transfer_req)

		// 2. param: bmRequestType
		if (!args[1]->IsInt32()) {
			THROW_BAD_ARGS("Device::ControlTransfer expects int as bmRequestType parameter")
		} else {
			control_transfer_req->bmRequestType = (uint8_t)args[1]->Int32Value();
		}

		// 3. param: bRequest
		if (!args[2]->IsInt32()) {
			THROW_BAD_ARGS("Device::ControlTransfer expects int as bRequest parameter")
		} else {
			control_transfer_req->bRequest = (uint8_t)args[2]->Int32Value();
		}

		// 4. param: wValue
		if (!args[3]->IsInt32()) {
			THROW_BAD_ARGS("Device::ControlTransfer expects int as wValue parameter")
		} else {
			control_transfer_req->wValue = (uint16_t)args[3]->Int32Value();
		}

		// 5. param: wIndex
		if (!args[4]->IsInt32()) {
			THROW_BAD_ARGS("Device::ControlTransfer expects int as wIndex parameter")
		} else {
			control_transfer_req->wIndex = (uint16_t)args[4]->Int32Value();
		}

		EIO_DELEGATION(control_transfer_req, 5)

		control_transfer_req->device = self;
		control_transfer_req->device->Ref();
		control_transfer_req->timeout = timeout;
		control_transfer_req->wLength = buflen;
		control_transfer_req->data = buf;
		DEBUG_OPT("bmRequestType 0x%X, bRequest: 0x%X, wValue: 0x%X, wIndex: 0x%X, wLength: 0x%X, timeout: 0x%X", control_transfer_req->bmRequestType, control_transfer_req->bRequest, control_transfer_req->wValue, control_transfer_req->wIndex, control_transfer_req->wLength, control_transfer_req->timeout);

		eio_custom(EIO_ControlTransfer, EIO_PRI_DEFAULT, EIO_After_ControlTransfer, control_transfer_req);
		ev_ref(EV_DEFAULT_UC);

		return Undefined();
	}

	int Device::EIO_ControlTransfer(eio_req *req) {
		// Inside EIO Threadpool, so don't touch V8.
		// Be careful!

		EIO_CAST(control_transfer_request, ct_req)

		Device * self = ct_req->device;
		libusb_device_handle * handle = self->device_container->handle;

		ct_req->errcode = libusb_control_transfer(handle, ct_req->bmRequestType, ct_req->bRequest, ct_req->wValue, ct_req->wIndex, ct_req->data, ct_req->wLength, ct_req->timeout);
		if (ct_req->errcode < LIBUSB_SUCCESS) {
			ct_req->errsource = "controlTransfer";
		}
		req->result = 0;

		return 0;
	}

	int Device::EIO_After_ControlTransfer(eio_req *req) {
		TRANSFER_REQUEST_FREE(control_transfer_request, device)
	}
}
