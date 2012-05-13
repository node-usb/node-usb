#ifndef SRC_STREAM_H
#define SRC_STREAM_H

#include "bindings.h"
#include "usb.h"
#include "device.h"
#include "uv_async_queue.h"
#include <vector>
#include <atomic>

namespace NodeUsb {
	
	class Stream{
		public:
		
		Stream(Handle<Object> endpoint, Handle<Function> cb, Handle<Function> scb);
		~Stream();
		
		struct CompletionData{
			Stream* stream;
			uint8_t* data;
			unsigned length;
			libusb_transfer_status status;
			bool dead;
		};
		
		static UVQueue<CompletionData> completionQueue;
		
		void start(unsigned nTransfers, unsigned transferSize);
		void stop();
		
		enum StreamStatus {STREAM_IDLE, STREAM_ACTIVE, STREAM_CANCELLING, STREAM_ABORTED};
		std::atomic_uchar state;
		unsigned activeTransfers;
		
		protected:
		std::vector <libusb_transfer*> transfers;
		Persistent<Object> v8endpoint;
		Persistent<Function> v8callback;
		Persistent<Function> v8stopCallback;
		Device* device;
		unsigned endpoint;
		libusb_transfer_type type;
		
		void afterStop();
		
		static void handleCompletion(CompletionData completion);
	};
}
#endif
