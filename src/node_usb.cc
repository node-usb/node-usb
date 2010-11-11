#include "./bindings.h"

namespace NodeUsb {

	void InitalizeAll(Handle<Object> target) {
		DEBUG("Entering")
		HandleScope scope;
		Usb::InitalizeUsb(target);
		Device::InitalizeDevice(target);
		
		target->Set(String::NewSymbol("version"),
				String::New(NODE_USB_VERSION));
		
		Handle<ObjectTemplate> global = ObjectTemplate::New();
		Handle<Context> context = Context::New(NULL, global);
		Context::Scope context_scope(context);
		context->Global()->Set(String::NewSymbol("Usb"), target);
	}

	extern "C" void init(Handle<Object> target) {
		DEBUG("Initalizing NodeUsb")
		HandleScope scope;
		InitalizeAll(target);
	}
}

