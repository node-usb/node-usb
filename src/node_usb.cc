#include "node_usb.h"
#include "bindings.h"
#include <thread>

extern ProtoBuilder::ProtoList ProtoBuilder::initProto;

Handle<Value> SetDebugLevel(const Arguments& args);
Handle<Value> GetDeviceList(const Arguments& args);
void initConstants(Handle<Object> target);

libusb_context* usb_context;
std::thread usb_thread;

void USBThreadFn(){
	while(1) libusb_handle_events(usb_context);
}

extern "C" void Initialize(Handle<Object> target) {
	HandleScope  scope;

	libusb_init(&usb_context);
	usb_thread = std::thread(USBThreadFn);
	usb_thread.detach();

	ProtoBuilder::initAll(target);

	NODE_SET_METHOD(target, "setDebugLevel", SetDebugLevel);
	NODE_SET_METHOD(target, "getDeviceList", GetDeviceList);
	initConstants(target);
}

NODE_MODULE(usb_bindings, Initialize)

Handle<Value> SetDebugLevel(const Arguments& args) {
	HandleScope scope;
	if (args.Length() != 1 || !args[0]->IsUint32() || args[0]->Uint32Value() >= 4) {
		THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-3]]!") 
	}
	
	libusb_set_debug(usb_context, args[0]->Uint32Value());
	return Undefined();
}

Handle<Value> GetDeviceList(const Arguments& args) {
	HandleScope scope;
	libusb_device **devs;
	int cnt = libusb_get_device_list(usb_context, &devs);
	CHECK_USB(cnt);

	Handle<Array> arr = Array::New(cnt);

	for(int i = 0; i < cnt; i++) {
		arr->Set(i, Device::get(devs[i]));
	}
	libusb_free_device_list(devs, true);
	return scope.Close(arr);
}

void initConstants(Handle<Object> target){
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PER_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_AUDIO);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_COMM);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HID);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PRINTER);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PTP);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_MASS_STORAGE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HUB);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_DATA);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_WIRELESS);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_APPLICATION);
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
}
