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

namespace NodeUsb  {
	class Usb : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			Usb();
			~Usb();

		protected:
			// members
			bool is_initalized;
			int num_devices;
			libusb_device **devices;
			// internal methods
			int Init();

			// V8 getter
			static Handle<Value> IsLibusbInitalizedGetter(Local<String> property, const AccessorInfo &info);
			
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> GetDeviceList(const Arguments& args);
			static Handle<Value> Refresh(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);		
	};

	class Device : public EventEmitter {
		public:
			// called from outside to initalize V8 class template
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Device(libusb_device*);
			~Device();

		protected:
			// members
			struct libusb_device *device;
			struct libusb_device_descriptor device_descriptor;
			struct libusb_config_descriptor *config_descriptor;

			// V8 getter
			static Handle<Value> BusNumberGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceAddressGetter(Local<String> property, const AccessorInfo &info);

			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);
			static Handle<Value> Reset(const Arguments& args);
			static Handle<Value> GetConfigDescriptor(const Arguments& args);
			static Handle<Value> GetDeviceDescriptor(const Arguments& args);
	};

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

	class Callback {
		public:
			static void DispatchAsynchronousUsbTransfer(libusb_transfer *transfer);
	};

	class Endpoint : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			// Dispatcher / callback handler must be static
			Endpoint(libusb_device*, libusb_endpoint_descriptor*);
			~Endpoint();
		protected:
			// members
			struct libusb_device *device;
			struct libusb_device_handle *handle;
			struct libusb_endpoint_descriptor *descriptor;
			int endpoint_type;
			int transfer_type;

			int FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, void *_user_data, uint32_t _timeout, unsigned int num_iso_packets = 0);

			// v8 getter
			static Handle<Value> EndpointTypeGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> TransferTypeGetter(Local<String> property, const AccessorInfo &info);
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Write(const Arguments& args);

	};
}
#endif
