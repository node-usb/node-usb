#ifndef SRC_BINDINGS_H_
#define SRC_BINDINGS_H_

#include "node_usb.h"

// Taken from node-libmysqlclient
#define OBJUNWRAP ObjectWrap::Unwrap
#define V8STR(str) String::New(str)
#define DEBUG(str) fprintf(stderr, "node-usb [%s:%s() %d]: %s", __FILE__, __FUNCTION__, __LINE__, str); fprintf(stderr, "\n");

namespace NodeUsb  {
	class Usb : public EventEmitter {
		public:
			/** called from outside to initalize V8 class template */
			static void InitalizeUsb(Handle<Object> target);
			Usb();
			~Usb();

		protected:
			/** members */
			bool is_initalized;
			libusb_device **devices;
			/** methods */
			int InitalizeLibusb();
			/** exposed to V8 */
			static Handle<Value> New(const Arguments& args);
			/** V8 getter */
			static Handle<Value> IsLibusbInitalizedGetter(Local<String> property, const AccessorInfo &info);
			/** V8 functions */
			static Handle<Value> GetDeviceList(const Arguments& args);
			static Handle<Value> Refresh(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);		
	};

	class Device : public EventEmitter {
		public:
			/** called from outside to initalize V8 class template */
			static void InitalizeDevice(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Device(libusb_device*);
			~Device();

		protected:
			/** members */
			bool is_opened;
			struct libusb_device *device;
			struct libusb_device_handle *handle;
			struct libusb_device_descriptor device_descriptor;
			struct libusb_config_descriptor *config_descriptor;

			/** exposed to V8 */
			static Handle<Value> New(const Arguments& args);
			/** V8 getter */
			static Handle<Value> BusNumberGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceAddressGetter(Local<String> property, const AccessorInfo &info);
			/** V8 functions */
			static Handle<Value> Open(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);
			static Handle<Value> Reset(const Arguments& args);
			static Handle<Value> GetConfigDescriptor(const Arguments& args);
			static Handle<Value> GetDeviceDescriptor(const Arguments& args);
	};
}
#endif
