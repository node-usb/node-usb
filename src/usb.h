#ifndef SRC_USB_H
#define SRC_USB_H

#include "bindings.h"

namespace NodeUsb {
	class Usb : public EventEmitter {
		public:
			static void Initalize(Handle<Object> target);
			Usb();
			~Usb();

		protected:
			// members
			libusb_context *context;
			int num_devices;
			libusb_device **devices;
			// internal methods
			int Init();

			// V8 getter
			static Handle<Value> IsLibusbInitalizedGetter(Local<String> property, const AccessorInfo &info);
			
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> SetDebugLevel(const Arguments& args);
			static Handle<Value> GetDeviceList(const Arguments& args);
			static Handle<Value> Refresh(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);		
	};
}
#endif
