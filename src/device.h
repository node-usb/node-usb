#ifndef SRC_DEVICE_H
#define SRC_DEVICE_H

#include "bindings.h"
#include "usb.h"

namespace NodeUsb {
	// intermediate EIO structure for device
	struct device_request:request {
		Device * device;
	};

	struct nodeusb_transfer:device_request {
		unsigned char *data;
		unsigned int timeout;
	};

	struct control_transfer_request:nodeusb_transfer {
		uint8_t bmRequestType;
		uint8_t bRequest;
		uint16_t wValue;
		uint16_t wIndex;
		uint16_t wLength;
	};

	class Device : public ObjectWrap {
		public:
			// called from outside to initalize V8 class template
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Device(Handle<Object>, libusb_device*);
			~Device();

		protected:
			Persistent<Object> usb;

			struct nodeusb_device_container *device_container;
			struct libusb_device_descriptor device_descriptor;

			// V8 getter
			static Handle<Value> BusNumberGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceAddressGetter(Local<String> property, const AccessorInfo &info);

			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);
			static Handle<Value> AddReference(const Arguments& args);
			static Handle<Value> RemoveReference(const Arguments& args);
			static Handle<Value> Reset(const Arguments& args);

			// Reset -> Async
			static void EIO_Reset(uv_work_t *req);
			static void EIO_After_Reset(uv_work_t *req);
			static Handle<Value> GetConfigDescriptor(const Arguments& args);
			static Handle<Value> GetDeviceDescriptor(const Arguments& args);
			static Handle<Value> GetExtraData(const Arguments& args);
			static Handle<Value> GetInterfaces(const Arguments& args);
			static Handle<Value> ControlTransfer(const Arguments& args);
			static void EIO_ControlTransfer(uv_work_t *req);
			static void EIO_After_ControlTransfer(uv_work_t *req);
	};
}
#endif
