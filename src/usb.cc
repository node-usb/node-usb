#include "usb.h"
#include "device.h"

namespace NodeUsb {
	/**
	 * @param usb.isLibusbInitalized: boolean
	 */
	void Usb::Initalize(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope  scope;
		
		Local<FunctionTemplate> t = FunctionTemplate::New(Usb::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Usb"));
		Device::constructor_template = Persistent<FunctionTemplate>::New(t);

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
		// libusb_transfer_status
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_COMPLETED);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_ERROR);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_TIMED_OUT);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_CANCELLED);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_STALL);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_NO_DEVICE);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_OVERFLOW);
		// libusb_transfer_flags
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_SHORT_NOT_OK);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_FREE_BUFFER);
		NODE_DEFINE_CONSTANT(instance_template, LIBUSB_TRANSFER_FREE_TRANSFER);

		// Properties
		// TODO: should get_device_list be a property?
		instance_template->SetAccessor(V8STR("isLibusbInitalized"), Usb::IsLibusbInitalizedGetter);

		// Bindings to nodejs
		NODE_SET_PROTOTYPE_METHOD(t, "refresh", Usb::Refresh);
		NODE_SET_PROTOTYPE_METHOD(t, "setDebugLevel", Usb::SetDebugLevel);
		NODE_SET_PROTOTYPE_METHOD(t, "getDevices", Usb::GetDeviceList);
		NODE_SET_PROTOTYPE_METHOD(t, "close", Usb::Close);

		// Export Usb class to V8/node.js environment
		target->Set(String::NewSymbol("Usb"), t->GetFunction());
		DEBUG("Leave")
	}

	Usb::Usb() : ObjectWrap() {
		is_initalized = false;
		num_devices = 0;
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
	int Usb::Init() {
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
		LOCAL(Usb, self, info.Holder())
		
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
		// create new object
		Usb *usb = new Usb();
		// wrap object to arguments
		usb->Wrap(args.This());
		args.This()->Set(V8STR("revision"),V8STR(NODE_USB_REVISION));

		// increment object reference, otherwise object will be GCed by V8
		usb->Ref();

		return args.This();
	}

	/**
	 * Refresh libusb environment
	 */
	Handle<Value> Usb::Refresh(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		CHECK_USB(self->Init(), scope);
		return scope.Close(True());
	}

	/**
	 * Set debug level
	 */
	Handle<Value> Usb::SetDebugLevel(const Arguments& args) {
		LOCAL(Usb, self, args.This())
		CHECK_USB(self->Init(), scope);

		// need libusb_device structure as first argument
		if (args.Length() != 1 || !args[0]->IsUint32() || !(((uint32_t)args[0]->Uint32Value() >= 0) && ((uint32_t)args[0]->Uint32Value() < 4))) {
			THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-3]]!") 
		}
		
		libusb_set_debug(NULL, args[0]->Uint32Value());

		return Undefined();
	}
	
	/**
	 * Returns the devices discovered by libusb
	 * @return array[Device]
	 */
	Handle<Value> Usb::GetDeviceList(const Arguments& args) {
		LOCAL(Usb, self, args.This())

		CHECK_USB(self->Init(), scope);

		// dynamic array (sic!) which contains the devices discovered later
		Local<Array> discoveredDevices = Array::New();

		// TODO Google codeguide for ssize_t?
		ssize_t i = 0;

		// if no devices were covered => get device list
		if (self->devices == NULL) {
			DEBUG("Discover device list");
			self->num_devices = libusb_get_device_list(NULL, &(self->devices));
			CHECK_USB(self->num_devices, scope);
		}

		// js_device contains the Device instance
		Local<Object> js_device;

		for (; i < self->num_devices; i++) {
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
		LOCAL(Usb, self, args.This())

		if (false == self->is_initalized) {
			return scope.Close(False());
		}

		delete self;

		return scope.Close(True());
	}
}
