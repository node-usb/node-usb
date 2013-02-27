#ifndef SRC_BINDINGS_H
#define SRC_BINDINGS_H

#include "node_usb.h"
#include "libusb-1.0/libusb.h"

// Taken from node-libmysqlclient
#define V8STR(str) String::New(str)
#define V8SYM(str) String::NewSymbol(str)

Local<Value> libusbException(int errorno);
Local<v8::Value> makeBuffer(const uint8_t* buf, unsigned length);

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

#endif
