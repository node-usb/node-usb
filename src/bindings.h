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










	
#define BUF_LEN_ARG(ARG) \
	libusb_endpoint_direction modus; \
	if (Buffer::HasInstance(ARG)) { \
		modus = LIBUSB_ENDPOINT_OUT; \
	} else { \
		modus = LIBUSB_ENDPOINT_IN; \
		if (!ARG->IsUint32()) { \
		      THROW_BAD_ARGS("IN transfer requires number of bytes. OUT transfer requires buffer.") \
		} \
	} \
	\
	if (modus == LIBUSB_ENDPOINT_OUT) { \
	  	Local<Object> _buffer = ARG->ToObject(); \
		length = Buffer::Length(_buffer); \
		buf = reinterpret_cast<unsigned char*>(Buffer::Data(_buffer)); \
	}else{ \
		length = ARG->Uint32Value(); \
		buf = 0; \
	}

namespace NodeUsb  {
	class Device;
	class Endpoint;
	class Transfer;
	class Stream;
	
	const PropertyAttribute CONST_PROP = static_cast<v8::PropertyAttribute>(ReadOnly|DontDelete);
	
	void doTransferCallback(Handle<Function> v8callback, Handle<Object> v8this, libusb_transfer_status status, uint8_t* buffer, unsigned length);

}
#endif
