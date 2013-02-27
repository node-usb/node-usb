#include "bindings.h"
#include "transfer.h"
#include <assert.h>

extern "C" void LIBUSB_CALL usbThreadCb(libusb_transfer *transfer);
void handleCompletion(Transfer* t);
UVQueue<Transfer*> Transfer::completionQueue(handleCompletion);

Transfer::Transfer(){
	transfer = libusb_alloc_transfer(0);
	transfer->callback = usbThreadCb;
	transfer->user_data = this;
	printf("Created Transfer %p\n", this);
}

Transfer::~Transfer(){
	printf("Freed Transfer %p\n", this);
	libusb_free_transfer(transfer);
}

static Handle<Value> Transfer_constructor(const Arguments& args){
	ENTER_CONSTRUCTOR(1);
	UNWRAP_ARG(pDevice, device, 0);

	setConst(args.This(), "device", args[0]);
	auto self = new Transfer();
	self->attach(args.This());
	self->device = device;

	return scope.Close(args.This());
}

// Transfer.submit(endpointAddr, endpointType, timeout, buffer, callback)
Handle<Value> Transfer_Submit(const Arguments& args) {
	ENTER_METHOD(pTransfer, 5);
	INT_ARG(self->transfer->endpoint, 0);
	INT_ARG(self->transfer->type, 1);
	INT_ARG(self->transfer->timeout, 2);
	if (!Buffer::HasInstance(args[3])){
		THROW_BAD_ARGS("Buffer arg [3] must be Buffer");
	}
	Local<Object> buffer_obj = args[3]->ToObject();
	if (!self->device->handle){
		THROW_ERROR("Device is not open");
	}
	CALLBACK_ARG(4)

	// Can't be cached in constructor as device could be closed and re-opened
	self->transfer->dev_handle = self->device->handle;

	self->transfer->buffer = (unsigned char*) Buffer::Data(buffer_obj);
	self->transfer->length = Buffer::Length(buffer_obj);

	self->v8buffer = Persistent<Object>::New(buffer_obj);
	self->v8callback = Persistent<Function>::New(callback);

	self->ref();
	self->device->ref();
	Transfer::completionQueue.ref();

	printf("Submitting, %p %x %i %i %i %p %p\n", 
		self->transfer->dev_handle,
		self->transfer->endpoint,
		self->transfer->type,
		self->transfer->timeout,
		self->transfer->length,
		self->transfer->user_data,
		self->transfer->buffer
	);

	CHECK_USB(libusb_submit_transfer(self->transfer));
	printf("Submitted transfer %p\n", self);
	return scope.Close(args.This());
}

extern "C" void LIBUSB_CALL usbThreadCb(libusb_transfer *transfer){
	Transfer* t = static_cast<Transfer*>(transfer->user_data);
	printf("Completion callback %p\n", t);
	assert(t != NULL);
	Transfer::completionQueue.post(t);
}

void handleCompletion(Transfer* self){
	printf("HandleCompletion %p\n", self);

	self->device->unref();
	Transfer::completionQueue.unref();
	
	if (!self->v8callback.IsEmpty()) {
		HandleScope scope;
		Handle<Value> error = Undefined();
		if (self->transfer->status != 0){
			error = libusbException(self->transfer->status);
		}
		Handle<Value> argv[] = {error, self->v8buffer,
			Uint32::New(self->transfer->actual_length)};
		TryCatch try_catch;
		self->v8callback->Call(self->handle_, 3, argv);
		if (try_catch.HasCaught()) {
			FatalException(try_catch);
		}
		self->v8callback.Dispose();
	}

	self->transfer->buffer = NULL;
	self->v8buffer.Dispose();
	self->unref();
}

Handle<Value> Transfer_Cancel(const Arguments& args) {
	ENTER_METHOD(pDevice, 1);
	return Undefined();
}

static void init(Handle<Object> target){
	pTransfer.init(&Transfer_constructor);
	pTransfer.addMethod("submit", Transfer_Submit);
	pTransfer.addMethod("cancel", Transfer_Cancel);
}

Proto<Transfer> pTransfer("Transfer", &init);
