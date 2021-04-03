#include "node_usb.h"

extern "C" void LIBUSB_CALL usbCompletionCb(libusb_transfer *transfer);

Transfer::Transfer(const Napi::CallbackInfo& info)
	: Napi::ObjectWrap<Transfer>(info) {
	transfer = libusb_alloc_transfer(0);
	transfer->callback = usbCompletionCb;
	transfer->user_data = this;
	DEBUG_LOG("Created Transfer %p", this);
	Constructor(info);
}

Transfer::~Transfer(){
	DEBUG_LOG("Freed Transfer %p", this);
	v8callback.Reset();
	libusb_free_transfer(transfer);
}

// new Transfer(device, endpointAddr, type, timeout)
Napi::Value Transfer::Constructor(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	ENTER_CONSTRUCTOR(5);
	UNWRAP_ARG(Device, device, 0);
	int endpoint, type, timeout;
	INT_ARG(endpoint, 1);
	INT_ARG(type, 2);
	INT_ARG(timeout, 3);
	CALLBACK_ARG(4);

	info.This().As<Napi::Object>().DefineProperty(Napi::PropertyDescriptor::Value(std::string("device"), info[0], CONST_PROP));
	auto self = this;
	self->device = device;
	self->transfer->endpoint = endpoint;
	self->transfer->type = type;
	self->transfer->timeout = timeout;

	self->v8callback.Reset(callback, 1);

	return info.This();
}

// Transfer.submit(buffer, callback)
Napi::Value Transfer::Submit(const Napi::CallbackInfo& info) {
	ENTER_METHOD(Transfer, 1);

	if (self->transfer->buffer){
		THROW_ERROR("Transfer is already active")
	}

	if (!info[0].IsBuffer()){
		THROW_BAD_ARGS("Buffer arg [0] must be Buffer");
	}
	Napi::Buffer<unsigned char> buffer_obj = info[0].As<Napi::Buffer<unsigned char>>();
	if (!self->device->device_handle){
		THROW_ERROR("Device is not open");
	}

	// Can't be cached in constructor as device could be closed and re-opened
	self->transfer->dev_handle = self->device->device_handle;

	self->v8buffer.Reset(buffer_obj, 1);
	self->transfer->buffer = (unsigned char*) buffer_obj.Data();
	self->transfer->length = buffer_obj.ByteLength();

	DEBUG_LOG("Submitting, %p %p %x %i %i %i %p",
		self,
		self->transfer->dev_handle,
		self->transfer->endpoint,
		self->transfer->type,
		self->transfer->timeout,
		self->transfer->length,
		self->transfer->buffer
	);

	CHECK_USB(libusb_submit_transfer(self->transfer));
	self->ref();
	self->device->ref();

	return info.This();
}

extern "C" void LIBUSB_CALL usbCompletionCb(libusb_transfer *transfer){
	Transfer* t = static_cast<Transfer*>(transfer->user_data);
	DEBUG_LOG("Completion callback %p", t);
	assert(t != NULL);

	#ifdef USE_POLL
	handleCompletion(t);
	#else
	t->device->completionQueue.post(t);
	#endif
}

void handleCompletion(Transfer* self){
	Napi::Env env = self->Env();
	Napi::HandleScope scope(env);
	DEBUG_LOG("HandleCompletion %p", self);

	self->device->unref();

	// The callback may resubmit and overwrite these, so need to clear the
	// persistent first.
	Napi::Object buffer = self->v8buffer.Value();
	self->v8buffer.Reset();
	self->transfer->buffer = NULL;

	if (!self->v8callback.IsEmpty()) {
		Napi::Value error = env.Undefined();
		if (self->transfer->status != 0){
			error = libusbException(env, self->transfer->status).Value();
		}
		try {
			self->v8callback.MakeCallback(self->Value(), { error, buffer,
				Napi::Number::New(env, (uint32_t)self->transfer->actual_length) });
		}
		catch (const Napi::Error& e) {
			Napi::Error::Fatal("", e.what());
		}
	}

	self->unref();
}

Napi::Value Transfer::Cancel(const Napi::CallbackInfo& info){
	ENTER_METHOD(Transfer, 0);
	DEBUG_LOG("Cancel %p %i", self, !!self->transfer->buffer);
	int r = libusb_cancel_transfer(self->transfer);
	if (r == LIBUSB_ERROR_NOT_FOUND){
		// Not useful to throw an error for this case
		return Napi::Boolean::New(env, false);
	} else {
		CHECK_USB(r);
		return Napi::Boolean::New(env, true);
	}
}

Napi::Object Transfer::Init(Napi::Env env, Napi::Object exports) {
	exports.Set("Transfer", Transfer::DefineClass(
		env,
		"Transfer",
		{
			Transfer::InstanceMethod("submit", &Transfer::Submit),
			Transfer::InstanceMethod("cancel", &Transfer::Cancel),
		}));

	return exports;
}
