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
		NODE_SET_PROTOTYPE_METHOD(t, "getDeviceDescriptor", Device::GetDeviceDescriptor);
		NODE_SET_PROTOTYPE_METHOD(t, "getConfigDescriptor", Device::GetConfigDescriptor);
		NODE_SET_PROTOTYPE_METHOD(t, "getInterfaces", Device::GetInterfaces);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Device"), t->GetFunction());	
		DEBUG("Leave")
	}

	Device::Device(libusb_device* _device) {
		device_container = (nodeusb_device_container*)malloc(sizeof(nodeusb_device_container));
		device_container->device = _device;
	}

	Device::~Device() {
		// free configuration descriptor
		if (device_container) {
			libusb_free_config_descriptor(device_container->config_descriptor);
			free(device_container);
		}

		DEBUG("Device object destroyed")
	}
	
	Handle<Value> Device::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Device object created")

		// need libusb_device structure as first argument
		if (args.Length() <= 0 || !args[0]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. Must be external!") 
		}

		Local<External> refDevice = Local<External>::Cast(args[0]);

		// cast local reference to local libusb_device structure 
		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());

		// create new Device object
		Device *device = new Device(libusbDevice);

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

	Handle<Value> Device::Ref(const Arguments& args) {
		LOCAL(Device, self, args.This())
		libusb_ref_device(self->device_container->device);
		
		return Undefined();
	}

	Handle<Value> Device::Unref(const Arguments& args) {
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
		EIO_DELEGATION(reset_req)
		
		reset_req->device = self->device_container->device;
		
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
		EIO_CAST(device_request, reset_req)
		
		libusb_device_handle *handle;

		int errcode = 0;
		
		if ((errcode = libusb_open(reset_req->device, &handle)) >= LIBUSB_SUCCESS) {
			if ((errcode = libusb_reset_device(handle)) < LIBUSB_SUCCESS) {
				libusb_close(handle);
				reset_req->error->Set(V8STR("error_source"), V8STR("reset"));
			}
		} else {
			reset_req->error->Set(V8STR("error_source"), V8STR("open"));
		}
		
		reset_req->error->Set(V8STR("error_code"), Uint32::New(errcode));
		
		// needed for EIO so that the EIO_After_Reset method will be called
		req->result = 0;
		
		return 0;
	}

	int Device::EIO_After_Reset(eio_req *req) {
		EIO_CAST(device_request, reset_req)
		EIO_AFTER(reset_req)
		
		// release intermediate structure
		free(reset_req);
		
		return 0;
	}
	

// TODO: Read-Only
#define LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New((*self->device_container->config_descriptor).name));

	/**
	 * Returns configuration descriptor structure
	 */
	Handle<Value> Device::GetConfigDescriptor(const Arguments& args) {
		// make local value reference to first parameter
		Local<External> refDevice = Local<External>::Cast(args[0]);

		LOCAL(Device, self, args.This())
		assert((self->device_container->device != NULL));
#if defined(__APPLE__) && defined(__MACH__)
		DEBUG("Open device handle for getConfigDescriptor (Darwin fix)")
		OPEN_DEVICE_HANDLE_NEEDED(scope)
#endif
		DEBUG("Get active config descriptor");
		CHECK_USB(libusb_get_active_config_descriptor(self->device_container->device, &(self->device_container->config_descriptor)), scope)
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
	
	Handle<Value> Device::GetInterfaces(const Arguments& args) {
		LOCAL(Device, self, args.This())
#if defined(__APPLE__) && defined(__MACH__)
		DEBUG("Open device handle for GetInterfacse (Darwin fix)")
		OPEN_DEVICE_HANDLE_NEEDED(scope)
#endif

		if (!self->device_container->config_descriptor) {
			CHECK_USB(libusb_get_active_config_descriptor(self->device_container->device, &(self->device_container->config_descriptor)), scope)
		}

		Local<Array> r = Array::New();
		int idx = 0;

		// iterate interfaces
		int numInterfaces = (*self->device_container->config_descriptor).bNumInterfaces;

		for (int idxInterface = 0; idxInterface < numInterfaces; idxInterface++) {
			int numAltSettings = ((*self->device_container->config_descriptor).interface[idxInterface]).num_altsetting;

			for (int idxAltSetting = 0; idxAltSetting < numAltSettings; idxAltSetting++) {
				// passing a pointer of libusb_interface_descriptor does not work. struct is lost by V8
				// idx of interface and alt_setting is passed so that Interface class can extract the given interface
				Local<Value> args_new_interface[3] = {
					External::New(self->device_container),
					Uint32::New(idxInterface),
					Uint32::New(idxAltSetting),
				};


				// create new object instance of class NodeUsb::Interface  
				Persistent<Object> js_interface(Interface::constructor_template->GetFunction()->NewInstance(3, args_new_interface));
				r->Set(idx++, js_interface);
			}
		}
		
		return scope.Close(r);
	}

// TODO: Read-Only
#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Uint32::New(self->device_descriptor.name));

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

		CHECK_USB(libusb_get_device_descriptor(self->device_container->device, &(self->device_descriptor)), scope)
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
}
