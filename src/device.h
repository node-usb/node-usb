#ifndef SRC_DEVICE_H
#define SRC_DEVICE_H

#include "bindings.h"
#include "usb.h"

#include <map>


struct Device: public node::ObjectWrap {
	~Device(){
		printf("Freed device %p\n", device);
		libusb_close(handle);
		libusb_unref_device(device);
	}

	libusb_device* device;
	libusb_device_handle* handle;

	static Handle<Value> get(libusb_device* handle);

	private:
		static std::map<libusb_device*, Persistent<Value> > byPtr;
		Device(libusb_device* d): device(d), handle(0) {
			libusb_ref_device(device);
			printf("Created device %p\n", device);
		}
		static void weakCallback(Persistent<Value> object, void *parameter);
};

#endif
