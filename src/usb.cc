#include "usb.h"
#include "device.h"
#include <thread>

namespace NodeUsb {
	libusb_context* usb_context;
	std::thread usb_thread;
	
	void USBThreadFn(){
		while(1) libusb_handle_events(usb_context);
	}

	void Usb::Initalize(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope  scope;
		
		libusb_init(&usb_context);

		// Constants must be passed to ObjectTemplate and *not* to FunctionTemplate
		// libusb_class_node
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PER_INTERFACE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_AUDIO);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_COMM);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HID);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PRINTER);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PTP);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_MASS_STORAGE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HUB);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_DATA);
		// both does not exist?
		// NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_WIRELESS);
		// NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_APPLICATION);
		NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_VENDOR_SPEC);
		// libusb_descriptor_type
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_DEVICE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_CONFIG);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_STRING);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_INTERFACE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_ENDPOINT);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_HID);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_REPORT);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_PHYSICAL);
		NODE_DEFINE_CONSTANT(target, LIBUSB_DT_HUB);
		// libusb_endpoint_direction
		NODE_DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_IN);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_OUT);
		// libusb_transfer_type
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_CONTROL);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_ISOCHRONOUS);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_BULK);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_INTERRUPT);
		// libusb_iso_sync_type
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_NONE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ASYNC);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ADAPTIVE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_SYNC);
		// libusb_iso_usage_type
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_DATA);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_FEEDBACK);
		NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_IMPLICIT);
		// libusb_transfer_status
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_COMPLETED);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_ERROR);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TIMED_OUT);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_CANCELLED);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_STALL);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_NO_DEVICE);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_OVERFLOW);
		// libusb_transfer_flags
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_SHORT_NOT_OK);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_BUFFER);
		NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_TRANSFER);

		// Bindings to nodejs
		NODE_SET_METHOD(target, "setDebugLevel", Usb::SetDebugLevel);
		NODE_SET_METHOD(target, "_getDevices", Usb::GetDeviceList);
		
		usb_thread = std::thread(USBThreadFn);

		DEBUG("Leave")
	}


	/**
	 * Set debug level
	 */
	Handle<Value> Usb::SetDebugLevel(const Arguments& args) {
		HandleScope scope;

		// need libusb_device structure as first argument
		if (args.Length() != 1 || !args[0]->IsUint32() || !(((uint32_t)args[0]->Uint32Value() >= 0) && ((uint32_t)args[0]->Uint32Value() < 4))) {
			THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-3]]!") 
		}
		
		libusb_set_debug(usb_context, args[0]->Uint32Value());

		return Undefined();
	}
	
	/**
	 * Returns the devices discovered by libusb
	 * @return array[Device]
	 */
	Handle<Value> Usb::GetDeviceList(const Arguments& args) {
		HandleScope scope;
		
		// dynamic array (sic!) which contains the devices discovered later
		Local<Array> discoveredDevices = Array::New();

		libusb_device **devices;
		int num_devices = libusb_get_device_list(usb_context, &devices);
		CHECK_USB(num_devices, scope);

		for (int i=0; i < num_devices; i++) {
			// wrap libusb_device structure into a Local<Value>
			Local<Value> argv[2] = {
				args.This(),
				External::New(devices[i])
			};

			// create new object instance of class NodeUsb::Device
			Local<Object> js_device(Device::constructor_template->GetFunction()->NewInstance(2, argv));

			// push to array
			discoveredDevices->Set(Integer::New(i), js_device);
		}
		
		libusb_free_device_list(devices, false);

		return scope.Close(discoveredDevices);
	}
}
