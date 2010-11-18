#include "bindings.h"
#include "interface.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Interface::constructor_template;

	Interface::Interface(libusb_device* _device, libusb_interface_descriptor* _interface_descriptor) {
		DEBUG("Assigning libusb_device and libusb_interface_descriptor structure to self")
		device = _device;
		descriptor = _interface_descriptor;
	}

	Interface::~Interface() {
		// TODO Close
		DEBUG("Device object destroyed")
	}


	void Interface::Initalize(Handle<Object> target) {
		DEBUG("Entering...")
		HandleScope  scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(Interface::New);

		// Constructor
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Interface"));
		Interface::constructor_template = Persistent<FunctionTemplate>::New(t);

		Local<ObjectTemplate> instance_template = t->InstanceTemplate();

		// Constants
		// no constants at the moment
	
		// Properties
		instance_template->SetAccessor(V8STR("__isKernelDriverAttached"), Interface::IsKernelDriverActiveGetter);

		// methods exposed to node.js

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Interface"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Interface::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Device object created")

		// need libusb_device structure as first argument
		if (args.Length() != 2 || !args[0]->IsExternal() || !args[1]->IsExternal()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:external:libusb_device, object:external:libusb_interface_descriptor]!") 
		}

		// assign arguments as local references
		Local<External> refDevice = Local<External>::Cast(args[0]);
		Local<External> refInterfaceDescriptor = Local<External>::Cast(args[1]);

		libusb_device *libusbDevice = static_cast<libusb_device*>(refDevice->Value());
		libusb_interface_descriptor *libusbInterfaceDescriptor = static_cast<libusb_interface_descriptor*>(refInterfaceDescriptor->Value());

		// create new Devicehandle object
		Interface *interface = new Interface(libusbDevice, libusbInterfaceDescriptor);
		// initalize handle

		// wrap created Device object to v8
		interface->Wrap(args.This());

#define LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Integer::New(interface->descriptor->name));
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bLength)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bDescriptorType)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceNumber)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bAlternateSetting)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bNumEndpoints)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceClass)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceSubClass)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(bInterfaceProtocol)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(iInterface)
		LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(extra_length)

		return args.This();
	}


	Handle<Value> Interface::IsKernelDriverActiveGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Interface, self, info.Holder())
		
		int isKernelDriverActive = 0;
			
		if ((isKernelDriverActive = libusb_open(self->device, &(self->handle))) >= 0) {
			isKernelDriverActive = libusb_kernel_driver_active(self->handle, self->descriptor->bInterfaceNumber);
		}

		return scope.Close(Integer::New(isKernelDriverActive));
	}
}
