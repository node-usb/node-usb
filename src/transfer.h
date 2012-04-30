#ifndef SRC_TRANSFER_H
#define SRC_TRANSFER_H

#include "bindings.h"
#include "usb.h"
#include "device.h"
#include "uv_async_queue.h"

namespace NodeUsb {
	
	class Transfer{
		public:
		//Named contstructors
		static Transfer* newControlTransfer(Handle<Object> device,
		                               uint8_t bmRequestType,
		                               uint8_t bRequest,
		                               uint16_t wValue,
		                               uint16_t wIndex,
		                               uint8_t* data,
		                               uint16_t wLength,
		                               unsigned timeout,
		                               Handle<Function> callback);
		
		static Transfer* newTransfer(libusb_transfer_type type,
		                             Handle<Object> device,
		                             uint8_t endpoint,
		                             unsigned char *data,
		                             int length,
		                             unsigned int timeout,
		                             Handle<Function> callback);

		
		void submit();
		
		~Transfer();
		
		static UVQueue<Transfer*> completionQueue;
		
		protected:
		Transfer(Handle<Object> _device, Handle<Function> _callback);
		
		libusb_transfer* transfer;
		Persistent<Object> v8device;
		Persistent<Function> v8callback;
		Device* device;
		uint32_t direction;
		
		static void handleCompletion(Transfer *transfer);
	};
}
#endif
