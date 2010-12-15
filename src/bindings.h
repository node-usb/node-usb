#ifndef SRC_BINDINGS_H_
#define SRC_BINDINGS_H_

#include "node_usb.h"
#include "libusb.h"

// Taken from node-libmysqlclient
#define OBJUNWRAP ObjectWrap::Unwrap
#define V8STR(str) String::New(str)

#ifdef ENABLE_DEBUG
  #define DEBUG_HEADER fprintf(stderr, "node-usb [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG(str) DEBUG_HEADER fprintf(stderr, "%s", str); DEBUG_FOOTER
  #define DEBUG_OPT(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
  #define DUMP_BYTE_STREAM(stream, len) DEBUG_HEADER for (int i = 0; i < buflen; i++) { fprintf(stderr, "0x%02X ", stream[i]); }
#endif

#ifndef ENABLE_DEBUG
  #define DEBUG(...)
  #define DEBUG(str)
  #define DUMP_BYTE_STREAM(stream)
#endif

#define THROW_BAD_ARGS(fail) return ThrowException(Exception::TypeError(V8STR(fail)));
#define THROW_ERROR(fail) return ThrowException(Exception::Error(V8STR(fail))));
#define THROW_NOT_YET return ThrowException(Exception::Error(String::Concat(String::New(__FUNCTION__), String::New("not yet supported"))));
#define CREATE_ERROR_OBJECT_AND_CLOSE_SCOPE(errno) \
		Local<Object> error = Object::New();\
		error->Set(V8STR("errno"), Integer::New(errno));\
		error->Set(V8STR("error"), errno_exception(errno));\
		return scope.Close(error);\

#define CHECK_USB(r, scope) \
	if (r < LIBUSB_SUCCESS) { \
		CREATE_ERROR_OBJECT_AND_CLOSE_SCOPE(r); \
	}

#define OPEN_DEVICE_HANDLE_NEEDED(scope) \
	if (self->device_container->handle_status == UNINITIALIZED) {\
		if ((self->device_container->last_error = libusb_open(self->device_container->device, &(self->device_container->handle))) < 0) {\
			self->device_container->handle_status = FAILED;\
		} else {\
			self->device_container->handle_status = OPENED;\
		}\
	}\
	if (self->device_container->handle_status == FAILED) { \
		CREATE_ERROR_OBJECT_AND_CLOSE_SCOPE(self->device_container->last_error) \
	} \

#define LOCAL(type, varname, ref) \
		HandleScope scope;\
		type *varname = OBJUNWRAP<type>(ref);
		
#define	EIO_CAST(type, varname) struct type *varname = reinterpret_cast<struct type *>(req->data);
#define	EIO_NEW(type, varname) struct type *varname = (struct type *) calloc(1, sizeof(struct type));
#define EIO_DELEGATION(varname) \
		Local<Function> callback; \
		if (args.Length() == 1 && args[0]->IsFunction()) { \
			callback = Local<Function>::Cast(args[0]); \
		} \
		if (!varname) { \
			V8::LowMemoryNotification(); \
		} \
		varname->callback = Persistent<Function>::New(callback); \
		varname->error = Persistent<Object>::New(Object::New()); \

#define EIO_AFTER(varname) HandleScope scope; \
		ev_unref(EV_DEFAULT_UC); \
		if (sizeof(varname->callback) > 0) { \
			Local<Value> argv[1]; \
			argv[0] = Local<Value>::New(scope.Close(varname->error)); \
			varname->callback->Call(Context::GetCurrent()->Global(), 1, argv); \
			varname->callback.Dispose(); \
		}
		

namespace NodeUsb  {
	// status of device handle
	enum nodeusb_device_handle_status { 
		UNINITIALIZED, 
		FAILED, 
		OPENED
	};

	// structure for device and device handle
	struct nodeusb_device_container {
		libusb_device *device;
		libusb_device_handle *handle;
		libusb_config_descriptor *config_descriptor;
		nodeusb_device_handle_status handle_status;
		int last_error;
	};

	struct nodeusb_endpoint_selection {
		int interface_number;
		int interface_alternate_setting;
		int endpoint_number;
	};	

	// intermediate EIO structure for device
	struct device_request {
		Persistent<Function> callback;
		Persistent<Object> error;
		libusb_device *device;
	};

	static inline Local<Value> errno_exception(int errorno) {
		Local<Value> e  = Exception::Error(String::NewSymbol(strerror(errorno)));
		Local<Object> obj = e->ToObject();
		std::string err = "";

		obj->Set(NODE_PSYMBOL("errno"), Integer::New(errorno));
		// taken from pyusb
		switch (errorno) {
			case LIBUSB_ERROR_IO:
				err = "Input/output error";
				break;
			case LIBUSB_ERROR_INVALID_PARAM:
				err  = "Invalid parameter";
				break;
			case LIBUSB_ERROR_ACCESS:
				err  = "Access denied (insufficient permissions)";
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				err = "No such device (it may have been disconnected)";
				break;
			case LIBUSB_ERROR_NOT_FOUND:
				err = "Entity not found";
				break;
			case LIBUSB_ERROR_BUSY:
				err = "Resource busy";
				break;
			case LIBUSB_ERROR_TIMEOUT:
				err = "Operation timed out";
				break;
			case LIBUSB_ERROR_OVERFLOW:
				err = "Overflow";
				break;
			case LIBUSB_ERROR_PIPE:
				err = "Pipe error";
				break;
			case LIBUSB_ERROR_INTERRUPTED:
				err = "System call interrupted (perhaps due to signal)";
				break;
			case LIBUSB_ERROR_NO_MEM:
				err = "Insufficient memory";
				break;
			case LIBUSB_ERROR_NOT_SUPPORTED:
				err = "Operation not supported or unimplemented on this platform";
				break;
			default:
				err = "Unknown error";
				break;
		}
		// convert err to const char* with help of c_str()
		obj->Set(NODE_PSYMBOL("msg"), String::New(err.c_str()));
		return e;
	}
}
#endif
