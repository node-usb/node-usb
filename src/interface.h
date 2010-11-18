#ifndef SRC_INTERFACE_H_
#define SRC_INTERFACE_H

#include "bindings.h"

namespace NodeUsb {
	class Interface : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Interface(libusb_device*, libusb_interface_descriptor*);
			~Interface();

		protected:
			// members
			struct libusb_device *device;
			struct libusb_device_handle *handle;
			struct libusb_interface_descriptor *descriptor;
			// V8 getter
			static Handle<Value> IsKernelDriverActiveGetter(Local<String> property, const AccessorInfo &info);
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
	};
}

#endif
