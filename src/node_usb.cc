#include "usb.h"
#include "device.h"
#include "interface.h"
#include "endpoint.h"

namespace NodeUsb {
	void InitalizeAll(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope scope;
		Usb::Initalize(target);
		Device::Initalize(target);
		Interface::Initalize(target);
		Endpoint::Initalize(target);
		target->Set(String::NewSymbol("version"),
				String::New(NODE_USB_VERSION));
	}

	void init(Handle<v8::Object> target) {
		DEBUG("Initalizing NodeUsb")
		HandleScope scope;
		InitalizeAll(target);
	}
}

NODE_MODULE(usb_bindings, NodeUsb::init)
