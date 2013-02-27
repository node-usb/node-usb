#ifndef SRC_BINDINGS_H
#define SRC_BINDINGS_H

#include "node_usb.h"
#include "libusb-1.0/libusb.h"

// Taken from node-libmysqlclient
#define OBJUNWRAP ObjectWrap::Unwrap
#define V8STR(str) String::New(str)
#define V8SYM(str) String::NewSymbol(str)

Local<Value> libusbException(int errorno);
Local<v8::Value> makeBuffer(const uint8_t* buf, unsigned length);

#define CHECK_USB(r) \
	if (r < LIBUSB_SUCCESS) { \
		ThrowException(libusbException(r)); \
		return scope.Close(Undefined());   \
	}

#define CALLBACK_ARG(VARNAME, CALLBACK_ARG_IDX) \
	Local<Function> callback; \
	if (args.Length() > (CALLBACK_ARG_IDX)) { \
		if (!args[CALLBACK_ARG_IDX]->IsFunction()) { \
			return ThrowException(Exception::TypeError( String::New("Argument " #CALLBACK_ARG_IDX " must be a function"))); \
		} \
		callback = Local<Function>::Cast(args[CALLBACK_ARG_IDX]); \
	} \
	VARNAME->callback = Persistent<Function>::New(callback);










#define LOCAL(TYPE, VARNAME, REF) \
		HandleScope scope;\
		TYPE *VARNAME = OBJUNWRAP<TYPE>(REF);
		
#define EIO_CUSTOM(FUNC, STRUCTURE, CALLBACK) \
		uv_work_t* req = new uv_work_t();\
		req->data = STRUCTURE;\
		uv_queue_work(uv_default_loop(), req, FUNC, (uv_after_work_cb) CALLBACK);\

#define	EIO_CAST(TYPE, VARNAME) struct TYPE *VARNAME = reinterpret_cast<struct TYPE *>(req->data);
#define	EIO_NEW(TYPE, VARNAME) \
		struct TYPE *VARNAME = (struct TYPE *) calloc(1, sizeof(struct TYPE)); \
		if (!VARNAME) { \
			V8::LowMemoryNotification(); \
		}

#define EIO_DELEGATION(VARNAME, CALLBACK_ARG_IDX) \
		Local<Function> callback; \
		if (args.Length() > (CALLBACK_ARG_IDX)) { \
			if (!args[CALLBACK_ARG_IDX]->IsFunction()) { \
				return ThrowException(Exception::TypeError( String::New("Argument " #CALLBACK_ARG_IDX " must be a function"))); \
			} \
			callback = Local<Function>::Cast(args[CALLBACK_ARG_IDX]); \
		} \
		VARNAME->callback = Persistent<Function>::New(callback);

#define EIO_HANDLE_ERROR(VARNAME) \
			if(VARNAME->errsource) { \
				Handle<Object> error = Object::New(); \
				error->Set(V8SYM("error_source"), V8STR(VARNAME->errsource)); \
				error->Set(V8SYM("error_code"), Uint32::New(VARNAME->errcode)); \
				Local<Value> argv[1]; \
				argv[0] = Local<Value>::New(scope.Close(error)); \
				TryCatch try_catch; \
				VARNAME->callback->Call(Context::GetCurrent()->Global(), 1, argv); \
				if (try_catch.HasCaught()) { \
					FatalException(try_catch); \
				} \
			}

#define EIO_AFTER(VARNAME, SELF) \
		if (!VARNAME->callback.IsEmpty()) { \
			HandleScope scope; \
			EIO_HANDLE_ERROR(VARNAME) \
			VARNAME->callback.Dispose(); \
		}\
		delete req;\
		VARNAME->SELF->Unref();

#define TRANSFER_REQUEST_FREE(STRUCT, SELF)\
	EIO_CAST(STRUCT, transfer_req)\
	EIO_AFTER(transfer_req, SELF)\
	free(transfer_req);
	
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
	
	

	struct nodeusb_endpoint_selection {
		int interface_number;
		int interface_alternate_setting;
		int endpoint_number;
	};	

	struct request {
		Persistent<Function> callback;
		int errcode;
		const char * errsource;
	};

	struct nodeusb_transfer:request {
		unsigned char *data;
		unsigned int timeout;
		uint16_t bytesTransferred;
	};
}
#endif
