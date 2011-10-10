#ifndef SRC_ENDPOINT_H
#define SRC_ENDPOINT_H

namespace NodeUsb {
	class Callback {
		public:
			// Dispatcher / callback handler must be static
			static void DispatchAsynchronousUsbTransfer(libusb_transfer *_transfer);
	};

	class Endpoint : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Endpoint(nodeusb_device_container*, const libusb_endpoint_descriptor*, uint32_t);
			~Endpoint();
		protected:
			// members
			struct nodeusb_device_container *device_container;
			const struct libusb_endpoint_descriptor *descriptor;

			struct bulk_interrupt_transfer_request:nodeusb_transfer {
				unsigned char endpoint;
				int length;
				int transferred;
			};

			int32_t endpoint_type;
			uint32_t transfer_type;
			uint32_t idx_endpoint;

			int FillTransferStructure(libusb_transfer *_transfer, unsigned char *_buffer, int32_t _buflen, Persistent<Function> _callback, uint32_t _timeout, unsigned int num_iso_packets = 0);

			// v8 getter
			static Handle<Value> EndpointTypeGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> TransferTypeGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> MaxPacketSizeGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> MaxIsoPacketSizeGetter(Local<String> property, const AccessorInfo &info);
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> Submit(const Arguments& args);
			static Handle<Value> GetExtraData(const Arguments& args);
			static Handle<Value> BulkTransfer(const Arguments& args);
			static int EIO_BulkTransfer(eio_req *req);
			static int EIO_After_BulkTransfer(eio_req *req);
			static Handle<Value> InterruptTransfer(const Arguments& args);
			static int EIO_InterruptTransfer(eio_req *req);
			static int EIO_After_InterruptTransfer(eio_req *req);

	};
	
}

#endif
