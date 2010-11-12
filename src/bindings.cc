#include "./bindings.h"

#define THROW_BAD_ARGS(fail) return ThrowException(Exception::TypeError(V8STR(fail)));
#define THROW_NOT_YET return ThrowException(Exception::TypeError(String::Concat(String::New(__FUNCTION__), String::New("not yet supported"))));
#define CHECK_USB(r, scope) \
	if (r < LIBUSB_SUCCESS) { \
		return scope.Close(ThrowException(errno_exception(r)));\
	}

#define LOCAL(type, varname, ref) \
		HandleScope scope;\
		type *varname = OBJUNWRAP<type>(ref);

/**
 * Remarks
 *  * variable name "self" always refers to the unwraped object instance of static method class
 *  * there is no node-usb support for libusbs context. Want to keep it simple.
 */

namespace NodeUsb {
/******************************* Helper functions */
	static inline Local<Value> errno_exception(int errorno) {
		Local<Value> e  = Exception::Error(String::NewSymbol(strerror(errorno)));
		Local<Object> obj = e->ToObject();
		std::string err = "";

		obj->Set(NODE_PSYMBOL("errno"), Integer::New(errorno));
		// taken from pyusb
		switch (errorno) {
			case LIBUSB_ERROR_IO:
				err = "Input/output error";
				break;
			case LIBUSB_ERROR_INVALID_PARAM:
				err  = "Invalid parameter";
				break;
			case LIBUSB_ERROR_ACCESS:
				err  = "Access denied (insufficient permissions)";
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				err = "No such device (it may have been disconnected)";
				break;
			case LIBUSB_ERROR_NOT_FOUND:
				err = "Entity not found";
				break;
			case LIBUSB_ERROR_BUSY:
				err = "Resource busy";
				break;
			case LIBUSB_ERROR_TIMEOUT:
				err = "Operation timed out";
				break;
			case LIBUSB_ERROR_OVERFLOW:
				err = "Overflow";
				break;
			case LIBUSB_ERROR_PIPE:
				err = "Pipe error";
				break;
			case LIBUSB_ERROR_INTERRUPTED:
				err = "System call interrupted (perhaps due to signal)";
				break;
			case LIBUSB_ERROR_NO_MEM:
				err = "Insufficient memory";
				break;
			case LIBUSB_ERROR_NOT_SUPPORTED:
				err = "Operation not supported or unimplemented on this platform";
				break;
			default:
				err = "Unknown error";
				break;
		}
		// convert err to const char* with help of c_str()
		obj->Set(NODE_PSYMBOL("msg"), String::New(err.c_str()));
		return e;
	}

/******************************* USB */
	/**
	 * @param usb.isLibusbInitalized: boolean
	 */
	void Usb::InitalizeUsb(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope  scope;
		
		Local<FunctionTemplate> t = FunctionTemplate::New(Usb::New);

		// Constructor
		t->Inherit(EventEmitter::constructor_template);
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Usb"));

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants must be passed to ObjectTemplate and *not* to FunctionTemplate
		// libusb_class_node
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PER_INTERFACE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_AUDIO);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_COMM);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_HID);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PRINTER);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_PTP);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_MASS_STORAGE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_HUB);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_DATA);
		// both does not exist?
		// NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_WIRELESS);
		// NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_APPLICATION);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_CLASS_VENDOR_SPEC);
		// libusb_descriptor_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_DEVICE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_CONFIG);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_STRING);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_INTERFACE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_ENDPOINT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_HID);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_REPORT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_PHYSICAL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_DT_HUB);
		// libusb_endpoint_direction
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ENDPOINT_IN);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ENDPOINT_OUT);
		// libusb_transfer_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_CONTROL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_ISOCHRONOUS);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_BULK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TYPE_INTERRUPT);
		// libusb_iso_sync_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_NONE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_ASYNC);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_ADAPTIVE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_SYNC_TYPE_SYNC);
		// libusb_iso_usage_type
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_DATA);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_FEEDBACK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_ISO_USAGE_TYPE_IMPLICIT);

		// Properties
		// TODO: should get_device_list be a property?
		instance_template->SetAccessor(V8STR("isLibusbInitalized"), Usb::IsLibusbInitalizedGetter);

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "refresh", Usb::Refresh);
		NODE_SET_PROTOTYPE_METHOD(t, "getDevices", Usb::GetDeviceList);
		NODE_SET_PROTOTYPE_METHOD(t, "close", Usb::Close);

		// Export Usb class to V8/node.js environment
		target->Set(String::NewSymbol("Usb"), t->GetFunction());
		DEBUG("Leave")
	}

	Usb::Usb() : EventEmitter() {
		is_initalized = false;
		devices = NULL;
	}

	Usb::~Usb() {
		if (devices != NULL) {
			libusb_free_device_list(devices, 1);
		}

		libusb_exit(NULL);
		DEBUG("NodeUsb::Usb object destroyed")
		// TODO Freeing opened USB devices?
	}

	/**
	 * Methods not exposed to v8 - used only internal
	 */
	int Usb::InitalizeLibusb() {
		if (is_initalized) {
			return LIBUSB_SUCCESS;
		}

		int r = libusb_init(NULL);

		if (0 == r) {
			is_initalized = true;
		}

		return r;
	}
	
	/**
	 * Returns libusb initalization status 
	 * @return boolean
	 */
	Handle<Value> Usb::IsLibusbInitalizedGetter(Local<String> property, const AccessorInfo &info) {
		HandleScope scope;
		
		Usb *self = OBJUNWRAP<Usb>(info.Holder());
		
		if (self->is_initalized == true) {
			return scope.Close(True());
		}

		return scope.Close(False());
	}


	/**
	 * Creates a new Usb object
	 */
	Handle<Value> Usb::New(const Arguments& args) {
		HandleScope scope;
		// create a new object
		Usb *self = new Usb();
		// wrap self object to arguments
		self->Wrap(args.This());

		return args.This();
	}

	/**
	 * Refresh libusb environment
	 */
	Handle<Value> Usb::Refresh(const Arguments& args) {
		HandleScope scope;
		Usb *self = OBJUNWRAP<Usb>(args.This());

		CHECK_USB(self->InitalizeLibusb(), scope);
		return scope.Close(True());
	}

	/**
	 * Returns the devices discovered by libusb
	 * @return array[Device]
	 */
	Handle<Value> Usb::GetDeviceList(const Arguments& args) {
		HandleScope scope;
		Usb *self = OBJUNWRAP<Usb>(args.This());

		CHECK_USB(self->InitalizeLibusb(), scope);

		// dynamic array (sic!) which contains the devices discovered later
		Local<Array> discoveredDevices = Array::New();

		// TODO Google codeguide for ssize_t?
		ssize_t cntDevices = 0;
		ssize_t i = 0;

		// if no devices were covered => get device list
		if (self->devices == NULL) {
			cntDevices = libusb_get_device_list(NULL, &(self->devices));
			CHECK_USB(cntDevices, scope);
		}
		
		// js_device contains the Device instance
		Local<Object> js_device;

		for (; i < cntDevices; i++) {
			// wrap libusb_device structure into a Local<Value>
			Local<Value> arg = External::New(self->devices[i]);

			// create new object instance of class NodeUsb::Device  
			Persistent<Object> js_device(Device::constructor_template->GetFunction()->NewInstance(1, &arg));
			
			// push to array
			discoveredDevices->Set(Integer::New(i), js_device);
		}

		return scope.Close(discoveredDevices);
	}

	/**
	 * Close current libusb context
	 * @return boolean
	 */
	Handle<Value> Usb::Close(const Arguments& args) {
		HandleScope scope;
		Usb *self = OBJUNWRAP<Usb>(args.This());

		if (false == self->is_initalized) {
			return scope.Close(False());
		}

		delete self;

		return scope.Close(True());
	}

/******************************* Device */
#define ENSURE_DEVICE_IS_OPEN \
	if (false == self->is_opened) { \
		CHECK_USB(libusb_open(self->device, &(self->handle)), scope); \
		self->is_opened = true; \
	}
	
	/** constructor template is needed for creating new Device objects from outside */
	Persistent<FunctionTemplate> Device::constructor_template;

	/**
	 * @param device.busNumber integer
	 * @param device.deviceAddress integer
	 */
	void Device::InitalizeDevice(Handle<Object> target) {
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
		NODE_SET_PROTOTYPE_METHOD(t, "close", Device::Close); // TODO Stream! 
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
		config_descriptor = NULL;
		is_opened = false;
	}

	Device::~Device() {
		// TODO Closing opened streams/device handles
		if (handle != NULL) {
			libusb_close(handle);
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

		// make local value reference to first parameter
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
		HandleScope scope;
		
		Device *self = OBJUNWRAP<Device>(info.Holder());
		uint8_t bus_number = libusb_get_bus_number(self->device);

		return scope.Close(Integer::New(bus_number));
	}

	/**
	 * @return integer
	 */
	Handle<Value> Device::DeviceAddressGetter(Local<String> property, const AccessorInfo &info) {
		HandleScope scope;
		
		Device *self = OBJUNWRAP<Device>(info.Holder());
		uint8_t address = libusb_get_device_address(self->device);

		return scope.Close(Integer::New(address));
	}

	Handle<Value> Device::Close(const Arguments& args) {
		LOCAL(Device, self, args.This())
		
		if (true == self->is_opened) {
			// libusb does not return any value so no CHECK_USB is needed
			libusb_close(self->handle);
			return scope.Close(True());
		}

		return scope.Close(False());
	}

	Handle<Value> Device::Reset(const Arguments& args) {
		HandleScope scope;
		THROW_NOT_YET
		return scope.Close(True());
	}


// TODO: Read-Only
#define LIBUSB_CONFIG_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New((*self->config_descriptor).name));
#define LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(name) \
		interface->Set(V8STR(#name), Integer::New(interface_descriptor.name));
#define LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(name) \
		endpoint->Set(V8STR(#name), Integer::New(endpoint_descriptor.name));

	Handle<Value> Device::GetConfigDescriptor(const Arguments& args) {
		LOCAL(Device, self, args.This())
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
		
		// make new byte array. not very elegant but works. anyhow.
		Local<Array> config_extra = Array::New();
		for (int i = 0; i < (*self->config_descriptor).extra_length; i++) {
			config_extra->Set(i, Integer::New((*self->config_descriptor).extra[i]));
		}
		r->Set(V8STR("extra"), config_extra);

		Local<Array> interfaces = Array::New();
		
		r->Set(V8STR("interfaces"), interfaces);
		
		// iterate interfaces
		for (int i = 0; i < (*self->config_descriptor).bNumInterfaces; i++) {
			Local<Object> interface  = Object::New();
			libusb_interface_descriptor interface_descriptor = ((*self->config_descriptor).interface)->altsetting[i];

			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bLength)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceNumber)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bAlternateSetting)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bNumEndpoints)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceClass)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceSubClass)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceProtocol)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(iInterface)
			LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(extra_length)
			// read extra settings
			Local<Array> interface_extra = Array::New();
			for (int j = 0; j < interface_descriptor.extra_length; j++) {
				interface_extra->Set(j, Integer::New(interface_descriptor.extra[j]));
			}
			interface->Set(V8STR("extra"), interface_extra);

			Local<Array> endpoints = Array::New();
			// interate endpoints
			for (int j = 0; j < interface_descriptor.bNumEndpoints; j++) {
				Local<Object> endpoint = Object::New();
				libusb_endpoint_descriptor endpoint_descriptor = interface_descriptor.endpoint[j];
				
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bLength)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bEndpointAddress)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bmAttributes)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(wMaxPacketSize)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bInterval)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bRefresh)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(bSynchAddress)
				LIBUSB_ENDPOINT_DESCRIPTOR_STRUCT_TO_V8(extra_length)
				
				// read extra settings
				Local<Array> endpoint_extra = Array::New();
				for (int k = 0; k < endpoint_descriptor.extra_length; k++) {
					endpoint_extra->Set(k, Integer::New(endpoint_descriptor.extra[k]));
				}
				endpoint->Set(V8STR("extra"), endpoint_extra);
				endpoints->Set(j, endpoint);
			}

			interface->Set(V8STR("endpoints"), endpoints);
			interfaces->Set(i, interface);
		}
		
		// free it
		libusb_free_config_descriptor(self->config_descriptor);

		return scope.Close(r);
	}

// TODO: Read-Only
#define LIBUSB_DEVICE_DESCRIPTOR_STRUCT_TO_V8(name) \
		r->Set(V8STR(#name), Integer::New(self->device_descriptor.name));

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
