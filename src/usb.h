#ifndef SRC_USB_H
#define SRC_USB_H

#include "bindings.h"
#include <vector>

extern libusb_context *usb_context;

namespace NodeUsb {
	class Usb {
		public:
			static void Initalize(Handle<Object> target);
			static void LoadDevices();
			
		protected:
			Usb();
			~Usb();
			
			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> SetDebugLevel(const Arguments& args);
			static Handle<Value> Close(const Arguments& args);	
			
			static std::vector< Persistent<Object> > deviceList;	
			static Handle<Value> DeviceListLength(Local<String> property, const AccessorInfo &info);
			static Handle<Value> DeviceListGet(unsigned i, const AccessorInfo &info);
	};
}
#endif
