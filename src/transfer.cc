#include "bindings.h"
#include "transfer.h"
#include <assert.h>

using namespace NodeUsb;

UVQueue<Transfer*> Transfer::completionQueue(Transfer::handleCompletion);

Transfer::Transfer(Handle<Object> _device, Handle<Function> _callback):
	v8device(Persistent<Object>::New(_device)),
	v8callback(Persistent<Function>::New(_callback)),
	device(ObjectWrap::Unwrap<Device>(_device)){
	transfer = libusb_alloc_transfer(0);
}

Transfer::~Transfer(){
	v8device.Dispose();
	v8callback.Dispose();
	free(transfer->buffer);
	libusb_free_transfer(transfer);
}

extern "C" void LIBUSB_CALL usbThreadCb(libusb_transfer *t);

Transfer* Transfer::newControlTransfer(Handle<Object> device,
                                         uint8_t bmRequestType,
                                         uint8_t bRequest,
                                         uint16_t wValue,
                                         uint16_t wIndex,
                                         uint8_t* data,
                                         uint16_t wLength,
                                         unsigned timeout,
                                         Handle<Function> callback){
	Transfer *t = new Transfer(device, callback);
	
	uint8_t *buffer = (uint8_t*) malloc(LIBUSB_CONTROL_SETUP_SIZE+wLength);
	libusb_fill_control_setup(buffer, bmRequestType, bRequest, wValue, wIndex, wLength);
	memcpy(buffer+LIBUSB_CONTROL_SETUP_SIZE, data, wLength);
	libusb_fill_control_transfer(t->transfer, t->device->handle, buffer, usbThreadCb, (void*) t, timeout);
	
	return t;
}

Transfer* Transfer::newTransfer(libusb_transfer_type type,
                                Handle<Object> device,
                                uint8_t endpoint,
                                unsigned char *data,
                                int length,
                                unsigned int timeout,
                                Handle<Function> callback){
	Transfer *t = new Transfer(device, callback);
	uint8_t *buffer = (uint8_t*) malloc(length);
	if (data) memcpy(buffer, data, length);
	
	if (type == LIBUSB_TRANSFER_TYPE_BULK){
		libusb_fill_bulk_transfer(t->transfer, t->device->handle, endpoint,
			                      buffer, length,
			                      usbThreadCb, (void*) t, timeout);
	}else{
		libusb_fill_interrupt_transfer(t->transfer, t->device->handle, endpoint,
			                           buffer, length,
			                           usbThreadCb, (void*) t, timeout);
	}
	
	return t;
}

void Transfer::submit(){
	libusb_submit_transfer(transfer);
}

Local<v8::Value> makeBuffer(const uint8_t* buf, unsigned length){
	HandleScope scope;
	Buffer *slowBuffer = Buffer::New((char *)buf, length);
	Local<Object> globalObj = v8::Context::GetCurrent()->Global();
	Local<Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
	Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(length), v8::Integer::New(0) };
	Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
	return scope.Close(actualBuffer);
}

void Transfer::handleCompletion(Transfer* t){
	HandleScope scope;
	Local<Value> cbvalue;
	Local<Value> cberror;
	if (t->transfer->status == LIBUSB_TRANSFER_COMPLETED){		
		if (t->direction == IN){
			uint8_t* buffer = t->transfer->buffer;
			unsigned length = t->transfer->actual_length;
			if (t->transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL){
				buffer += LIBUSB_CONTROL_SETUP_SIZE;
			}
			cbvalue = makeBuffer(buffer, length);
		}
	}else{
		
		//TODO: pass exception
	}
	
	Local<Value> argv[2] = {cbvalue, cberror};
	TryCatch try_catch;
	t->v8callback->Call(Context::GetCurrent()->Global(), 2, argv);
	if (try_catch.HasCaught()) {
		FatalException(try_catch);
	}
	delete t;
}

extern "C" void LIBUSB_CALL usbThreadCb(libusb_transfer *transfer){
	Transfer* t = static_cast<Transfer*>(transfer->user_data);
	if (t){
		Transfer::completionQueue.post(t);
	}
}
