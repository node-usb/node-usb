#include "bindings.h"
#include "interface.h"
#include "endpoint.h"
#include "device.h"

namespace NodeUsb {
	Persistent<FunctionTemplate> Interface::constructor_template;

	Interface::Interface(Handle<Object> _device, struct nodeusb_device_container * const _device_container, const libusb_interface_descriptor* _interface_descriptor, uint32_t _idx_interface, uint32_t _idx_alt_setting) :
		device(Persistent<Object>::New(_device)),
		device_container(_device_container),
		descriptor(_interface_descriptor),
		idx_interface(_idx_interface),
		idx_alt_setting(_idx_alt_setting)
	{
	}

	Interface::~Interface() {
		// TODO Close
		device.Dispose();
		DEBUG("Interface object destroyed")
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
		instance_template->SetAccessor(V8STR("__idxInterface"), Interface::IdxInterfaceGetter);
		instance_template->SetAccessor(V8STR("__idxAltSetting"), Interface::IdxAltSettingGetter);

		// methods exposed to node.js
		NODE_SET_PROTOTYPE_METHOD(t, "getEndpoints", Interface::GetEndpoints); 
		NODE_SET_PROTOTYPE_METHOD(t, "detachKernelDriver", Interface::DetachKernelDriver); 
		NODE_SET_PROTOTYPE_METHOD(t, "attachKernelDriver", Interface::AttachKernelDriver); 
		NODE_SET_PROTOTYPE_METHOD(t, "claim", Interface::Claim); 
		NODE_SET_PROTOTYPE_METHOD(t, "release", Interface::Release); 
		NODE_SET_PROTOTYPE_METHOD(t, "setAlternateSetting", Interface::AlternateSetting); 
		NODE_SET_PROTOTYPE_METHOD(t, "isKernelDriverActive", Interface::IsKernelDriverActive);
		NODE_SET_PROTOTYPE_METHOD(t, "getExtraData", Interface::GetExtraData);

		// Make it visible in JavaScript
		target->Set(String::NewSymbol("Interface"), t->GetFunction());	
		DEBUG("Leave")
	}

	Handle<Value> Interface::New(const Arguments& args) {
		HandleScope scope;
		DEBUG("New Interface object created")

		// need libusb_device structure as first argument

		if (args.Length() != 4 || !args[0]->IsObject() || !args[1]->IsExternal() || !args[2]->IsUint32() ||  !args[3]->IsUint32()) {
			THROW_BAD_ARGS("Device::New argument is invalid. [object:device, int:idx_interface, int:idx_alt_setting!") // TODO
		}

		// assign arguments as local references
		Local<Object> device = Local<Object>::Cast(args[0]);
		Local<External> refDeviceContainer = Local<External>::Cast(args[1]);
		uint32_t idxInterface  = args[2]->Uint32Value();
		uint32_t idxAltSetting = args[3]->Uint32Value();

		nodeusb_device_container *deviceContainer = static_cast<nodeusb_device_container*>(refDeviceContainer->Value());
		const libusb_interface_descriptor *libusbInterfaceDescriptor = &((*deviceContainer->config_descriptor).interface[idxInterface]).altsetting[idxAltSetting];

		// create new Devicehandle object
		Interface *interface = new Interface(device, deviceContainer, libusbInterfaceDescriptor, idxInterface, idxAltSetting);
		// initalize handle

		// wrap created Device object to v8
		interface->Wrap(args.This());

#define LIBUSB_INTERFACE_DESCRIPTOR_STRUCT_TO_V8(name) \
		args.This()->Set(V8STR(#name), Uint32::New(interface->descriptor->name));
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


	Handle<Value> Interface::IdxInterfaceGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Interface, self, info.Holder())
		
		return scope.Close(Uint32::New(self->idx_interface));
	}

	Handle<Value> Interface::IdxAltSettingGetter(Local<String> property, const AccessorInfo &info) {
		LOCAL(Interface, self, info.Holder())
		
		return scope.Close(Uint32::New(self->idx_alt_setting));
	}

	Handle<Value> Interface::GetExtraData(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		 
		int m = (*self->descriptor).extra_length;
		
		Local<Array> r = Array::New(m);
		
		for (int i = 0; i < m; i++) {
		  uint32_t c = (*self->descriptor).extra[i];
		  
		  r->Set(i, Uint32::New(c));
		}
		
		return scope.Close(r);
	}

	Handle<Value> Interface::IsKernelDriverActive(const Arguments& args) {
		LOCAL(Interface, self, args.This())

		OPEN_DEVICE_HANDLE_NEEDED(scope)

		int isKernelDriverActive = 0;
			
		CHECK_USB((isKernelDriverActive = libusb_kernel_driver_active(self->device_container->handle, self->descriptor->bInterfaceNumber)), scope)

		return scope.Close(Integer::New(isKernelDriverActive));
	}	
	
	Handle<Value> Interface::DetachKernelDriver(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		OPEN_DEVICE_HANDLE_NEEDED(scope)

		CHECK_USB(libusb_detach_kernel_driver(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();
	}

	/**
	 * Attach kernel driver to current interface; delegates to libusb_attach_kernel_driver()
	 */
	Handle<Value> Interface::AttachKernelDriver(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		OPEN_DEVICE_HANDLE_NEEDED(scope)

		CHECK_USB(libusb_attach_kernel_driver(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();
	}
	
	/**
	 * Claim current interface; delegates to libusb_claim_interface()
	 */
	Handle<Value> Interface::Claim(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		OPEN_DEVICE_HANDLE_NEEDED(scope)
		CHECK_USB(libusb_claim_interface(self->device_container->handle, self->descriptor->bInterfaceNumber), scope)
		
		return Undefined();	
	}


	/**
	 * Releases current interface; delegates to libusb_release_interface()
	 * non-blocking!
	 */
	Handle<Value> Interface::Release(const Arguments& args) {
		LOCAL(Interface, self, args.This())

		OPEN_DEVICE_HANDLE_NEEDED(scope)

		// allocation of intermediate EIO structure
		EIO_NEW(release_request, release_req)

		// create default delegation
		EIO_DELEGATION(release_req, 0)
		
		release_req->interface = self;
		release_req->interface->Ref();

		eio_custom(EIO_Release, EIO_PRI_DEFAULT, EIO_After_Release, release_req);
	
		// add reference
		ev_ref(EV_DEFAULT_UC);
		
		return Undefined();		
	}
	
	int Interface::EIO_Release(eio_req *req) {
		// Inside EIO Threadpool, so don't touch V8.
		// Be careful!

		EIO_CAST(release_request, release_req)

		Interface * self = release_req->interface;
		libusb_device_handle * handle = self->device_container->handle;
		int interface_number          = self->descriptor->bInterfaceNumber;

		release_req->errcode = libusb_release_interface(handle, interface_number);
		if (release_req->errcode < LIBUSB_SUCCESS) {
			release_req->errsource = "release";
		}

		// needed for EIO so that the EIO_After_Reset method will be called
		req->result = 0;

		return 0;
	}

	int Interface::EIO_After_Release(eio_req *req) {
		TRANSFER_REQUEST_FREE(release_request, interface);
	}

	/**
	 * alternate setting for interface current interface; delegates to libusb_set_interface_alt_setting()
	 * non-blocking!
	 */
	Handle<Value> Interface::AlternateSetting(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		
		if (args.Length() != 2 || args[0]->IsUint32()) {
			THROW_BAD_ARGS("Interface::AlternateSetting expects [uint32:setting] as first parameter")
		}
		
		OPEN_DEVICE_HANDLE_NEEDED(scope)

		// allocation of intermediate EIO structure
		EIO_NEW(alternate_setting_request, alt_req)

		// create default delegation
		EIO_DELEGATION(alt_req, 1)
		
		alt_req->interface = self;
		alt_req->interface->Ref();
		alt_req->alternate_setting = args[0]->Uint32Value();

		eio_custom(EIO_AlternateSetting, EIO_PRI_DEFAULT, EIO_After_AlternateSetting, alt_req);
	
		// add reference
		ev_ref(EV_DEFAULT_UC);
		
		return Undefined();		
	}
	
	int Interface::EIO_AlternateSetting(eio_req *req) {
                // Inside EIO Threadpool, so don't touch V8.
                // Be careful!

		EIO_CAST(alternate_setting_request, alt_req)

		Interface * self = alt_req->interface;
		libusb_device_handle * handle = self->device_container->handle;
		int interface_number          = self->descriptor->bInterfaceNumber;

		alt_req->errcode = libusb_set_interface_alt_setting(handle, interface_number, alt_req->alternate_setting);
		if (alt_req->errcode < LIBUSB_SUCCESS) {
			alt_req->errsource = "alt_setting";
		}

		req->result = 0;
		
		return 0;
	}

	int Interface::EIO_After_AlternateSetting(eio_req *req) {
		TRANSFER_REQUEST_FREE(alternate_setting_request, interface);
	}

	Handle<Value> Interface::GetEndpoints(const Arguments& args) {
		LOCAL(Interface, self, args.This())
		Local<Array> r = Array::New();

		// interate endpoints
		int numEndpoints = (*self->descriptor).bNumEndpoints;

		for (int i = 0; i < numEndpoints; i++) {
			Local<Value> args_new_endpoint[5] = {
				Local<Object>::New(self->device),
				External::New(self->device_container),
				Uint32::New(self->idx_interface),
				Uint32::New(self->idx_alt_setting),
				Uint32::New(i)
			};

			// create new object instance of class NodeUsb::Endpoint
			r->Set(i, Endpoint::constructor_template->GetFunction()->NewInstance(5, args_new_endpoint));
		}

		return r;
	}
}
