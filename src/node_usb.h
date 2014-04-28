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

#include "protobuilder.h"

Local<Value> libusbException(int errorno);


struct Device: public node::ObjectWrap {
	libusb_device* device;
	libusb_device_handle* handle;

	static Handle<Value> get(libusb_device* handle);

	inline void ref(){Ref();}
	inline void unref(){Unref();}
	inline bool canClose(){return refs_ == 0;}
	inline void attach(Handle<Object> o){Wrap(o);}

	~Device();
	static void unpin(libusb_device* device);

	protected:
		static std::map<libusb_device*, Persistent<Value> > byPtr;
		Device(libusb_device* d);
		static void weakCallback(Persistent<Value> object, void *parameter);
};


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

extern Proto<Device> pDevice;
extern Proto<Transfer> pTransfer;

#define CHECK_USB(r) \
	if (r < LIBUSB_SUCCESS) { \
		ThrowException(libusbException(r)); \
		return scope.Close(Undefined());   \
	}

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
	Local<Function> callback; \
	if (args.Length() > (CALLBACK_ARG_IDX)) { \
		if (!args[CALLBACK_ARG_IDX]->IsFunction()) { \
			return ThrowException(Exception::TypeError( String::New("Argument " #CALLBACK_ARG_IDX " must be a function"))); \
		} \
		callback = Local<Function>::Cast(args[CALLBACK_ARG_IDX]); \
	} \

#ifdef DEBUG
  #define DEBUG_HEADER fprintf(stderr, "node-usb [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
  #define DEBUG_LOG(...)
#endif

#endif
