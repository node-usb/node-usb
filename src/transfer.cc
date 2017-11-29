#include "node_usb.h"

extern "C" void LIBUSB_CALL usbCompletionCb(libusb_transfer *transfer);
void handleCompletion(Transfer* t);

#ifndef USE_POLL
#include "uv_async_queue.h"
UVQueue<Transfer*> completionQueue(handleCompletion);
#endif

Transfer::Transfer(){
	transfer = libusb_alloc_transfer(0);
	transfer->callback = usbCompletionCb;
	transfer->user_data = this;
	DEBUG_LOG("Created Transfer %p", this);
}

Transfer::~Transfer(){
	DEBUG_LOG("Freed Transfer %p", this);
	v8callback.Reset();
	libusb_free_transfer(transfer);
}

// new Transfer(device, endpointAddr, type, timeout)
NAN_METHOD(Transfer_constructor) {
	ENTER_CONSTRUCTOR(5);
	UNWRAP_ARG(Device, device, 0);
	int endpoint, type, timeout;
	INT_ARG(endpoint, 1);
	INT_ARG(type, 2);
	INT_ARG(timeout, 3);
	CALLBACK_ARG(4);

	setConst(info.This(), "device", info[0]);
	auto self = new Transfer();
	self->attach(info.This());
	self->device = device;
	self->transfer->endpoint = endpoint;
	self->transfer->type = type;
	self->transfer->timeout = timeout;

	self->v8callback.Reset(callback);

	info.GetReturnValue().Set(info.This());
}

// Transfer.submit(buffer, callback)
NAN_METHOD(Transfer_Submit) {
	ENTER_METHOD(Transfer, 1);

	if (self->transfer->buffer){
		THROW_ERROR("Transfer is already active")
	}

	if (!Buffer::HasInstance(info[0])){
		THROW_BAD_ARGS("Buffer arg [0] must be Buffer");
	}
	Local<Object> buffer_obj = info[0]->ToObject();
	if (!self->device->device_handle){
		THROW_ERROR("Device is not open");
	}

	// Can't be cached in constructor as device could be closed and re-opened
	self->transfer->dev_handle = self->device->device_handle;

	self->v8buffer.Reset(buffer_obj);
	self->transfer->buffer = (unsigned char*) Buffer::Data(buffer_obj);
	self->transfer->length = Buffer::Length(buffer_obj);

	self->ref();
	self->device->ref();

	#ifndef USE_POLL
	completionQueue.ref();
	#endif

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
	info.GetReturnValue().Set(info.This());
}

extern "C" void LIBUSB_CALL usbCompletionCb(libusb_transfer *transfer){
	Transfer* t = static_cast<Transfer*>(transfer->user_data);
	DEBUG_LOG("Completion callback %p", t);
	assert(t != NULL);

	#ifdef USE_POLL
	handleCompletion(t);
	#else
	completionQueue.post(t);
	#endif
}

void handleCompletion(Transfer* self){
	Nan::HandleScope scope;
	DEBUG_LOG("HandleCompletion %p", self);

	self->device->unref();
	#ifndef USE_POLL
	completionQueue.unref();
	#endif

	// The callback may resubmit and overwrite these, so need to clear the
	// persistent first.
	Local<Object> buffer = Nan::New<Object>(self->v8buffer);
	self->v8buffer.Reset();
	self->transfer->buffer = NULL;

	if (!self->v8callback.IsEmpty()) {
		Local<Value> error = Nan::Undefined();
		if (self->transfer->status != 0){
			error = libusbException(self->transfer->status);
		}
		Local<Value> argv[] = {error, buffer,
			Nan::New<Uint32>((uint32_t) self->transfer->actual_length)};
		Nan::TryCatch try_catch;
		Nan::MakeCallback(self->handle(), Nan::New(self->v8callback), 3, argv);
		if (try_catch.HasCaught()) {
			Nan::FatalException(try_catch);
		}
	}

	self->unref();
}

NAN_METHOD(Transfer_Cancel){
	ENTER_METHOD(Transfer, 0);
	DEBUG_LOG("Cancel %p %i", self, !!self->transfer->buffer);
	int r = libusb_cancel_transfer(self->transfer);
	if (r == LIBUSB_ERROR_NOT_FOUND){
		// Not useful to throw an error for this case
		info.GetReturnValue().Set(Nan::False());
	} else {
		CHECK_USB(r);
		info.GetReturnValue().Set(Nan::True());
	}
}

void Transfer::Init(Local<Object> target){
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Transfer_constructor);
	tpl->SetClassName(Nan::New("Transfer").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "submit", Transfer_Submit);
	Nan::SetPrototypeMethod(tpl, "cancel", Transfer_Cancel);

	target->Set(Nan::New("Transfer").ToLocalChecked(), tpl->GetFunction());
}
