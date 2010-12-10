#ifndef SRC_DEVICE_H_
#define SRC_DEVICE_H

#include "bindings.h"

namespace NodeUsb {
	class Device : public EventEmitter {
		public:
			// called from outside to initalize V8 class template
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Device(libusb_device*);
			~Device();

		protected:
			// members
			struct nodeusb_device_container *device_container;
			struct libusb_device_descriptor device_descriptor;

			// V8 getter
			static Handle<Value> BusNumberGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceAddressGetter(Local<String> property, const AccessorInfo &info);

			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);
			static Handle<Value> Ref(const Arguments& args);
			static Handle<Value> Unref(const Arguments& args);
			static Handle<Value> Reset(const Arguments& args);
			// Reset -> Async
			static int EIO_After_Reset(eio_req *req);
			static int EIO_Reset(eio_req *req);
			static Handle<Value> GetConfigDescriptor(const Arguments& args);
			static Handle<Value> GetDeviceDescriptor(const Arguments& args);
			static Handle<Value> GetInterfaces(const Arguments& args);
	};
}
#endif
