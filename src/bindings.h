#ifndef SRC_BINDINGS_H_
#define SRC_BINDINGS_H_

#include "node_usb.h"
#include "libusb.h"

// Taken from node-libmysqlclient
#define OBJUNWRAP ObjectWrap::Unwrap
#define V8STR(str) String::New(str)

#ifdef ENABLE_DEBUG
  #define DEBUG(str) fprintf(stderr, "node-usb [%s:%s() %d]: %s", __FILE__, __FUNCTION__, __LINE__, str); fprintf(stderr, "\n");
#endif

#ifndef ENABLE_DEBUG
  #define DEBUG(str)
#endif


<<<<<<< HEAD
		protected:
			// members
			bool is_initalized;
			int num_devices;
			libusb_device **devices;
			// internal methods
			int InitalizeLibusb();
=======
#define THROW_BAD_ARGS(fail) return ThrowException(Exception::TypeError(V8STR(fail)));
#define THROW_NOT_YET return ThrowException(Exception::TypeError(String::Concat(String::New(__FUNCTION__), String::New("not yet supported"))));
#define CHECK_USB(r, scope) \
	if (r < LIBUSB_SUCCESS) { \
		return scope.Close(ThrowException(errno_exception(r)));\
	}
>>>>>>> write-support

#define LOCAL(type, varname, ref) \
		HandleScope scope;\
		type *varname = OBJUNWRAP<type>(ref);

namespace NodeUsb  {
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





<<<<<<< HEAD
	class Endpoint : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Endpoint(libusb_device*, libusb_endpoint_descriptor*);
			~Endpoint();
		protected:
			// members
			struct libusb_device *device;
			struct libusb_device_handle *handle;
			struct libusb_endpoint_descriptor *descriptor;
			int endpoint_type;
			void DispatchAsynchronousUsbTransfer(libusb_transfer *transfer);
			// v8 getter
			static Handle<Value> EndpointTypeGetter(Local<String> property, const AccessorInfo &info);
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Submit(const Arguments& args);

	};
=======
	class Callback {
		public:
			static void DispatchAsynchronousUsbTransfer(libusb_transfer *transfer);
	};

>>>>>>> write-support
}
#endif
