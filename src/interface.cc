#include "bindings.h"
#include "interface.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Interface::constructor_template;

	Interface::Interface(nodeusb_device_container* _device_container, libusb_interface_descriptor* _interface_descriptor) {
		device_container = _device_container;
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
		NODE_SET_PROTOTYPE_METHOD(t, "detach", Interface::DetachKernelDriver); 
		NODE_SET_PROTOTYPE_METHOD(t, "attach", Interface::AttachKernelDriver); 
		NODE_SET_PROTOTYPE_METHOD(t, "claim", Interface::Claim); 
		NODE_SET_PROTOTYPE_METHOD(t, "release", Interface::Release); 
		NODE_SET_PROTOTYPE_METHOD(t, "setAlternateSetting", Interface::AlternateSetting); 

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
		Local<External> refDeviceContainer = Local<External>::Cast(args[0]);
		Local<External> refInterfaceDescriptor = Local<External>::Cast(args[1]);

		nodeusb_device_container *deviceContainer = static_cast<nodeusb_device_container*>(refDeviceContainer->Value());
		libusb_interface_descriptor *libusbInterfaceDescriptor = static_cast<libusb_interface_descriptor*>(refInterfaceDescriptor->Value());

		// create new Devicehandle object
		Interface *interface = new Interface(deviceContainer, libusbInterfaceDescriptor);
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
		CHECK_USB_HANDLE_OPENED(&(self->device_container->handle), scope)

		int isKernelDriverActive = 0;
			
		CHECK_USB((isKernelDriverActive = libusb_kernel_driver_active(self->device_container->handle, self->descriptor->bInterfaceNumber)), scope)

		return scope.Close(Integer::New(isKernelDriverActive));
	}	
	
	Handle<Value> Interface::DetachKernelDriver(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		CHECK_USB_HANDLE_OPENED(&(self->device_container->handle), scope)
		CHECK_USB(libusb_detach_kernel_driver(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();
	}

	/**
	 * Attach kernel driver to current interface; delegates to libusb_attach_kernel_driver()
	 */
	Handle<Value> Interface::AttachKernelDriver(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		CHECK_USB_HANDLE_OPENED(&(self->device_container->handle), scope)
		CHECK_USB(libusb_attach_kernel_driver(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();
	}
	
	/**
	 * Claim current interface; delegates to libusb_claim_interface()
	 */
	Handle<Value> Interface::Claim(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		CHECK_USB_HANDLE_OPENED(&(self->device_container->handle), scope)
		CHECK_USB(libusb_claim_interface(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();	
	}


	/**
	 * Releases current interface; delegates to libusb_release_interface()
	 * non-blocking!
	 */
	Handle<Value> Interface::Release(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		
		// allocation of intermediate EIO structure
		EIO_NEW(release_request, release_req)

		// create default delegation
		EIO_DELEGATION(release_req)
		
		release_req->handle = self->device_container->handle;
		release_req->interface_number = self->descriptor->bInterfaceNumber;
		eio_custom(EIO_Release, EIO_PRI_DEFAULT, EIO_After_Release, release_req);
	
		// add reference
		ev_ref(EV_DEFAULT_UC);
		
		return Undefined();		
	}
	
	int Interface::EIO_Release(eio_req *req) {
		EIO_CAST(release_request, release_req)
		

		int errcode = 0;
		
		if ((errcode = libusb_open(release_req->device, &(release_req->handle))) >= LIBUSB_SUCCESS) {
			if ((errcode = libusb_release_interface(release_req->handle, release_req->interface_number)) < LIBUSB_SUCCESS) {
				libusb_close(release_req->handle);
				release_req->error->Set(V8STR("error_source"), V8STR("release"));
			}
		} else {
			release_req->error->Set(V8STR("error_source"), V8STR("open"));
		}
		
		release_req->error->Set(V8STR("error_code"), Uint32::New(errcode));
		
		// needed for EIO so that the EIO_After_Reset method will be called
		req->result = 0;
		
		return 0;
	}

	int Interface::EIO_After_Release(eio_req *req) {
		EIO_CAST(release_request, release_req)
		EIO_AFTER(release_req)
		
		free(release_req);
		
		return 0;
	}


	/**
	 * alternate setting for interface current interface; delegates to libusb_set_interface_alt_setting()
	 * non-blocking!
	 */
	Handle<Value> Interface::AlternateSetting(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		
		if (args.Length() != 1 || args[0]->IsUint32()) {
			THROW_BAD_ARGS("Interface::AlternateSetting expects [uint32:setting] as first parameter")
		}
		
		// allocation of intermediate EIO structure
		EIO_NEW(alternate_setting_request, alt_req)

		// create default delegation
		EIO_DELEGATION(alt_req)
		
		alt_req->handle = self->device_container->handle;
		alt_req->interface_number = self->descriptor->bInterfaceNumber;
		alt_req->alternate_setting = args[0]->Uint32Value();
		eio_custom(EIO_AlternateSetting, EIO_PRI_DEFAULT, EIO_After_AlternateSetting, alt_req);
	
		// add reference
		ev_ref(EV_DEFAULT_UC);
		
		return Undefined();		
	}
	
	int Interface::EIO_AlternateSetting(eio_req *req) {
		EIO_CAST(alternate_setting_request, alt_req)
		
		int errcode = 0;
		
		if ((errcode = libusb_open(alt_req->device, &(alt_req->handle))) >= LIBUSB_SUCCESS) {
			if ((errcode = libusb_set_interface_alt_setting(alt_req->handle, alt_req->interface_number, alt_req->alternate_setting)) < LIBUSB_SUCCESS) {
				libusb_close(alt_req->handle);
				alt_req->error->Set(V8STR("error_source"), V8STR("release"));
			}
		} else {
			alt_req->error->Set(V8STR("error_source"), V8STR("open"));
		}
		
		alt_req->error->Set(V8STR("error_code"), Uint32::New(errcode));
		
		req->result = 0;
		
		return 0;
	}

	int Interface::EIO_After_AlternateSetting(eio_req *req) {
		EIO_CAST(alternate_setting_request, alt_req)
		EIO_AFTER(alt_req)
		
		free(alt_req);
		
		return 0;
	}

}
