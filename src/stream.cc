#include "bindings.h"
#include "stream.h"
#include "endpoint.h"
#include <assert.h>

using namespace NodeUsb;

UVQueue<Stream::CompletionData> Stream::completionQueue(Stream::handleCompletion);
extern "C" void LIBUSB_CALL usbThreadStreamCb(libusb_transfer *t);

Stream::Stream(Handle<Object> _v8endpoint, Handle<Function> cb, Handle<Function> scb){
	Endpoint *ep = ObjectWrap::Unwrap<Endpoint>(_v8endpoint);
	device = ep->device;
	v8endpoint = Persistent<Object>::New(_v8endpoint);
	v8callback = Persistent<Function>::New(cb);
	v8stopCallback = Persistent<Function>::New(scb);
	endpoint = ep->descriptor->bEndpointAddress;
	type = ep->transfer_type;
	
	activeTransfers = 0;
	state = STREAM_IDLE;
	
	DEBUG_OPT("Created stream %p", this);
}

Stream::~Stream(){
	assert(state == STREAM_IDLE);
	v8endpoint.Dispose();
	v8callback.Dispose();
	v8stopCallback.Dispose();
}

void Stream::start(unsigned nTransfers, unsigned transferSize){
	assert(state == STREAM_IDLE);
	state = STREAM_ACTIVE;
	
	transfers.resize(nTransfers);
	
	for(unsigned i=0; i<nTransfers; i++){
		transfers[i] = libusb_alloc_transfer(0);
		libusb_fill_interrupt_transfer(transfers[i], device->handle, endpoint,
			                       (uint8_t*) malloc(transferSize), transferSize,
			                       usbThreadStreamCb, (void*) this, 0);
		transfers[i]->type = type;
		libusb_submit_transfer(transfers[i]);	
	}
	
	activeTransfers = nTransfers;
	uv_ref(uv_default_loop());
}

void Stream::stop(){
	assert(state == STREAM_ACTIVE);
	state = STREAM_CANCELLING;
	
	for (unsigned i=0; i<transfers.size(); i++){
		libusb_cancel_transfer(transfers[i]);
	}
}

void Stream::afterStop(){
	state = STREAM_IDLE;

	for (unsigned i=0; i<transfers.size(); i++){
		libusb_free_transfer(transfers[i]);
	}
	
	transfers.clear();
	uv_unref(uv_default_loop());
	
	HandleScope scope;
	TryCatch try_catch;
	v8stopCallback->Call(v8endpoint, 0, {});
	if (try_catch.HasCaught()) {
		FatalException(try_catch);
	}
}

extern "C" void LIBUSB_CALL usbThreadStreamCb(libusb_transfer *t){
	Stream* s = static_cast<Stream*>(t->user_data);
	assert(s != NULL);
	
	bool dead = (s->state != Stream::STREAM_ACTIVE);	
	Stream::completionQueue.post({s, t->buffer, t->actual_length, t->status, dead});
	
	if (!dead){
		t->buffer = (uint8_t*) malloc(t->length);
		libusb_submit_transfer(t);
	}
}
		
void Stream::handleCompletion(CompletionData c){
	doTransferCallback(c.stream->v8callback, c.status, c.data, c.length);
	free(c.data);

	if (c.dead){
		assert(c.stream->state == Stream::STREAM_CANCELLING);
		if (!--c.stream->activeTransfers){
			c.stream->afterStop();
		}
	}
}
