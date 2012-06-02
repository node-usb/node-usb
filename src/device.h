#ifndef SRC_DEVICE_H
#define SRC_DEVICE_H

#include "bindings.h"
#include "usb.h"

namespace NodeUsb {
	// intermediate EIO structure for device
	struct device_request:nodeusb_transfer {
		Device * device;
	};

	class Device : public ObjectWrap {
		public:
			// called from outside to initalize V8 class template
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Device(libusb_device*);
			~Device();
			
			int openHandle();
			void close();
			
			libusb_device *device;
			libusb_device_handle *handle;
			libusb_config_descriptor *config_descriptor;
			struct libusb_device_descriptor device_descriptor;
			unsigned timeout;
			
		protected:
			Persistent<Object> v8ConfigDescriptor;
			Persistent<Array>  v8Interfaces;
		
			// V8 getter
			static Handle<Value> BusNumberGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceAddressGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> TimeoutGetter(Local<String> property, const AccessorInfo &info);
			static void TimeoutSetter(Local<String> property, Local<Value> value, const AccessorInfo &info);
			static Handle<Value> ConfigDescriptorGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> InterfacesGetter(Local<String> property, const AccessorInfo &info);

			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);
			static Handle<Value> Reset(const Arguments& args);

			// Reset -> Async
			static void EIO_Reset(uv_work_t *req);
			static void EIO_After_Reset(uv_work_t *req);
			static Handle<Value> ControlTransfer(const Arguments& args);
			static void EIO_ControlTransfer(uv_work_t *req);
			static void EIO_After_ControlTransfer(uv_work_t *req);
	};
}
#endif
