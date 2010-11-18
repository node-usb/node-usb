#ifndef SRC_ENDPOINT_H_
#define SRC_ENDPOINT_H

namespace NodeUsb {
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

			int FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, Persistent<Function> _callback, uint32_t _timeout, unsigned int num_iso_packets = 0);

			// v8 getter
			static Handle<Value> EndpointTypeGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> TransferTypeGetter(Local<String> property, const AccessorInfo &info);
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Write(const Arguments& args);

	};
}

#endif
