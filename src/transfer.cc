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
	NanDisposePersistent(v8callback);
	libusb_free_transfer(transfer);
}

// new Transfer(device, endpointAddr, type, timeout)
NAN_METHOD(Transfer_constructor) {
	ENTER_CONSTRUCTOR(5);
	UNWRAP_ARG(pDevice, device, 0);
	int endpoint, type, timeout;
	INT_ARG(endpoint, 1);
	INT_ARG(type, 2);
	INT_ARG(timeout, 3);
	CALLBACK_ARG(4);

	setConst(args.This(), "device", args[0]);
	auto self = new Transfer();
	self->attach(args.This());
	self->device = device;
	self->transfer->endpoint = endpoint;
	self->transfer->type = type;
	self->transfer->timeout = timeout;

	NanAssignPersistent(self->v8callback, callback);

	NanReturnValue(args.This());
}

// Transfer.submit(buffer, callback)
NAN_METHOD(Transfer_Submit) {
	ENTER_METHOD(pTransfer, 1);

	if (self->transfer->buffer){
		THROW_ERROR("Transfer is already active")
	}
	
	if (!Buffer::HasInstance(args[0])){
		THROW_BAD_ARGS("Buffer arg [0] must be Buffer");
	}
	Local<Object> buffer_obj = args[0]->ToObject();
	if (!self->device->device_handle){
		THROW_ERROR("Device is not open");
	}

	// Can't be cached in constructor as device could be closed and re-opened
	self->transfer->dev_handle = self->device->device_handle;

	NanAssignPersistent(self->v8buffer, buffer_obj);
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
	NanReturnValue(args.This());
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
	NanScope();
	DEBUG_LOG("HandleCompletion %p", self);

	self->device->unref();
	#ifndef USE_POLL
	completionQueue.unref();
	#endif

	// The callback may resubmit and overwrite these, so need to clear the
	// persistent first.
	Local<Object> buffer = NanNew<Object>(self->v8buffer);
	NanDisposePersistent(self->v8buffer);
	self->transfer->buffer = NULL;

	if (!self->v8callback.IsEmpty()) {
		Handle<Value> error = NanUndefined();
		if (self->transfer->status != 0){
			error = libusbException(self->transfer->status);
		}
		Handle<Value> argv[] = {error, buffer,
			NanNew<Uint32>((uint32_t) self->transfer->actual_length)};
		TryCatch try_catch;
		NanNew(self->v8callback)->Call(NanObjectWrapHandle(self), 3, argv);
		if (try_catch.HasCaught()) {
			FatalException(try_catch);
		}
	}

	self->unref();
}

NAN_METHOD(Transfer_Cancel){
	ENTER_METHOD(pTransfer, 0);
	DEBUG_LOG("Cancel %p %i", self, !!self->transfer->buffer);
	int r = libusb_cancel_transfer(self->transfer);
	if (r == LIBUSB_ERROR_NOT_FOUND){
		// Not useful to throw an error for this case
		NanReturnValue(NanFalse());
	} else {
		CHECK_USB(r);
		NanReturnValue(NanTrue());
	}
}

static void init(Handle<Object> target){
	pTransfer.init(&Transfer_constructor);
	pTransfer.addMethod("submit", Transfer_Submit);
	pTransfer.addMethod("cancel", Transfer_Cancel);
}

Proto<Transfer> pTransfer("Transfer", &init);
