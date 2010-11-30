#include "bindings.h"
#include "device.h"
#include "interface.h"
#include "endpoint.h"
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

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Device"), t->GetFunction());	
		DEBUG("Leave")
	}

	Device::Device(libusb_device* _device) {
		DEBUG("Assigning libusb_device structure to self")
		device = _device;
		config_descriptor = (libusb_config_descriptor*)malloc(sizeof(libusb_config_descriptor));
	}

	Device::~Device() {
		// free configuration descriptor
		free(config_descriptor);
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

	Handle<Value> Device::Ref(const Arguments& args) {
		LOCAL(Device, self, args.This())
		libusb_ref_device(self->device);
		
		return Undefined();
	}

	Handle<Value> Device::Unref(const Arguments& args) {
		LOCAL(Device, self, args.This())
		libusb_unref_device(self->device);
		
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
		
		reset_req->device = self->device;
		
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
		r->Set(V8STR(#name), Integer::New((*self->config_descriptor).name));

	/**
	 * Returns configuration descriptor structure
	 */
	Handle<Value> Device::GetConfigDescriptor(const Arguments& args) {
		// make local value reference to first parameter
		Local<External> refDevice = Local<External>::Cast(args[0]);

		LOCAL(Device, self, args.This())
		assert((self->device != NULL));
		CHECK_USB(libusb_get_active_config_descriptor(self->device, &(self->config_descriptor)), scope)
		Local<Object> r = Object::New();

		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(wTotalLength)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bNumInterfaces)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bConfigurationValue)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(iConfiguration)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(MaxPower)
		LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		Local<Array> interfaces = Array::New();
		
		// iterate interfaces
		for (int i = 0; i < (*self->config_descriptor).bNumInterfaces; i++) {
			libusb_interface interface_container = (*self->config_descriptor).interface[i];

			for (int j = 0; j < interface_container.num_altsetting; j++) {
				libusb_interface_descriptor interface_descriptor = interface_container.altsetting[j];

				Local<Value> args_new_interface[2] = {
					External::New(self->device),
					External::New(&interface_descriptor),
				};

				// create new object instance of class NodeUsb::Interface  
				Persistent<Object> js_interface(Interface::constructor_template->GetFunction()->NewInstance(2, args_new_interface));
				Local<Array> endpoints = Array::New();

				// interate endpoints
				for (int k = 0; k < interface_descriptor.bNumEndpoints; k++) {
					libusb_endpoint_descriptor endpoint_descriptor = interface_descriptor.endpoint[k];

					Local<Value> args_new_endpoint[2] = {
						External::New(self->device),
						External::New(&endpoint_descriptor),
					};

					// create new object instance of class NodeUsb::Endpoint
					Persistent<Object> js_endpoint(Endpoint::constructor_template->GetFunction()->NewInstance(2, args_new_endpoint));
					endpoints->Set(k, js_endpoint);
				}

				js_interface->Set(V8STR("endpoints"), endpoints);
				interfaces->Set(i, js_interface);
			}
		}
		
		r->Set(V8STR("interfaces"), interfaces);
		// free it
		libusb_free_config_descriptor(self->config_descriptor);

		return scope.Close(r);
	}
	
// TODO: Read-Only
#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New(self->device_descriptor.name));

	/**
	 * Returns the device descriptor of current device
	 * @return object
	 */
	Handle<Value> Device::GetDeviceDescriptor(const Arguments& args) {
		LOCAL(Device, self, args.This())
		CHECK_USB(libusb_get_device_descriptor(self->device, &(self->device_descriptor)), scope)
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
