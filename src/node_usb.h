#ifndef SRC_NODE_USB_H
#define SRC_NODE_USB_H

#include <assert.h>
#include <string>
#include <map>

#ifdef _WIN32
#include <WinSock2.h>
#endif
#include <libusb.h>
#include <v8.h>

#include <node.h>
#include <node_buffer.h>
#include <uv.h>

using namespace v8;
using namespace node;

#include "helpers.h"

Local<Value> libusbException(int errorno);

struct Device: public Nan::ObjectWrap {
	libusb_device* device;
	libusb_device_handle* device_handle;

	static void Init(Local<Object> exports);
	static Local<Object> get(libusb_device* handle);

	inline void ref(){Ref();}
	inline void unref(){Unref();}
	inline bool canClose(){return refs_ == 0;}
	inline void attach(Local<Object> o){Wrap(o);}

	~Device();

	static Local<Object> cdesc2V8(libusb_config_descriptor * cdesc);

	protected:
		static std::map<libusb_device*, Device*> byPtr;
		Device(libusb_device* d);
};


struct Transfer: public Nan::ObjectWrap {
	libusb_transfer* transfer;
	Device* device;
	Nan::Persistent<Object> v8buffer;
	Nan::Persistent<Function> v8callback;

	static void Init(Local<Object> exports);

	inline void ref(){Ref();}
	inline void unref(){Unref();}
	inline void attach(Local<Object> o){Wrap(o);}

	Transfer();
	~Transfer();
};



#define CHECK_USB(r) \
	if (r < LIBUSB_SUCCESS) { \
		return Nan::ThrowError(libusbException(r)); \
	}

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
	Local<Function> callback; \
	if (info.Length() > (CALLBACK_ARG_IDX)) { \
		if (!info[CALLBACK_ARG_IDX]->IsFunction()) { \
			return Nan::ThrowTypeError("Argument " #CALLBACK_ARG_IDX " must be a function"); \
		} \
		callback = Local<Function>::Cast(info[CALLBACK_ARG_IDX]); \
	} \

#ifdef DEBUG
  #define DEBUG_HEADER fprintf(stderr, "node-usb [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
  #define DEBUG_LOG(...)
#endif

#endif
