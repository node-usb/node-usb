#ifndef SRC_USB_H
#define SRC_USB_H

#include "bindings.h"

extern libusb_context *usb_context;

namespace NodeUsb {
	class Usb {
		public:
			static void Initalize(Handle<Object> target);
			
		protected:
			Usb();
			~Usb();
			
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
