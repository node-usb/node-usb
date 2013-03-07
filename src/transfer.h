#ifndef SRC_TRANSFER_H
#define SRC_TRANSFER_H

#include "usb.h"
#include "device.h"

struct Transfer: public node::ObjectWrap {
	libusb_transfer* transfer;
	Device* device;
	Persistent<Object> v8buffer;
	Persistent<Function> v8callback;


	inline void ref(){Ref();}
	inline void unref(){Unref();}
	inline void attach(Handle<Object> o){Wrap(o);}

	Transfer();
	~Transfer();
};

#endif
