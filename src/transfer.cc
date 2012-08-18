#include "bindings.h"
#include "transfer.h"
#include <assert.h>

using namespace NodeUsb;

UVQueue<Transfer*> Transfer::completionQueue(Transfer::handleCompletion);

Transfer::Transfer(Handle<Object> _device, Handle<Object> _v8this, Handle<Function> _callback):
	v8device(Persistent<Object>::New(_device)),
	v8this(Persistent<Object>::New(_v8this)),
	v8callback(Persistent<Function>::New(_callback)),
	device(ObjectWrap::Unwrap<Device>(_device)){
	transfer = libusb_alloc_transfer(0);
}

Transfer::~Transfer(){
	v8this.Dispose();
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
	Transfer *t = new Transfer(device, device, callback);
	
	uint8_t *buffer = (uint8_t*) malloc(LIBUSB_CONTROL_SETUP_SIZE+wLength);
	libusb_fill_control_setup(buffer, bmRequestType, bRequest, wValue, wIndex, wLength);
	if (data) memcpy(buffer+LIBUSB_CONTROL_SETUP_SIZE, data, wLength);
	t->direction = data?LIBUSB_ENDPOINT_OUT:LIBUSB_ENDPOINT_IN;
	libusb_fill_control_transfer(t->transfer, t->device->handle, buffer, usbThreadCb, (void*) t, timeout);
	
	return t;
}

Transfer* Transfer::newTransfer(libusb_transfer_type type,
                                Handle<Object> device,
                                Handle<Object> v8endpoint,
                                uint8_t endpoint,
                                unsigned char *data,
                                int length,
                                unsigned int timeout,
                                Handle<Function> callback){
	Transfer *t = new Transfer(device, v8endpoint, callback);
	uint8_t *buffer = (uint8_t*) malloc(length);
	if (data) memcpy(buffer, data, length);
	t->direction = data?LIBUSB_ENDPOINT_OUT:LIBUSB_ENDPOINT_IN;
	

	libusb_fill_interrupt_transfer(t->transfer, t->device->handle, endpoint,
			                       buffer, length,
			                       usbThreadCb, (void*) t, timeout);
	
	t->transfer->type = type;
	
	return t;
}

void Transfer::submit(){
	completionQueue.ref();
	libusb_submit_transfer(transfer);
}

void Transfer::handleCompletion(Transfer* t){
	uint8_t* buffer = 0;
	unsigned length = t->transfer->actual_length;
	
	if (t->direction == LIBUSB_ENDPOINT_IN){
		buffer = t->transfer->buffer;
		if (t->transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL){
			buffer += LIBUSB_CONTROL_SETUP_SIZE;
		}
	}
	
	doTransferCallback(t->v8callback, t->v8this, t->transfer->status, buffer, length);
	
	completionQueue.unref();
	delete t;
}

extern "C" void LIBUSB_CALL usbThreadCb(libusb_transfer *transfer){
	Transfer* t = static_cast<Transfer*>(transfer->user_data);
	assert(t != NULL);
	Transfer::completionQueue.post(t);
}
