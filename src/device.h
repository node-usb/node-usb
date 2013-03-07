#ifndef SRC_DEVICE_H
#define SRC_DEVICE_H

#include "usb.h"

#include <map>

struct Device: public node::ObjectWrap {
	libusb_device* device;
	libusb_device_handle* handle;

	static Handle<Value> get(libusb_device* handle);

	inline void ref(){Ref();}
	inline void unref(){Unref();}
	inline bool canClose(){return refs_ == 0;}
	inline void attach(Handle<Object> o){Wrap(o);}

	~Device();

	protected:
		static std::map<libusb_device*, Persistent<Value> > byPtr;
		Device(libusb_device* d);
		static void weakCallback(Persistent<Value> object, void *parameter);
};

#endif
