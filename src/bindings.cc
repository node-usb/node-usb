#include "bindings.h"

using namespace NodeUsb;

bool AllowConstructor::m_current;

void NodeUsb::doTransferCallback(Handle<Function> v8callback, Handle<Object> v8this, libusb_transfer_status status, uint8_t* buffer, unsigned length){
	HandleScope scope;
	Local<Value> cbvalue = Local<Value>::New(Undefined());
	Local<Value> cberror = Local<Value>::New(Undefined());
	if (buffer && !status){		
		cbvalue = makeBuffer(buffer, length);
	}
	
	if (status){
		cberror = Local<Value>::New(Number::New(status));
		//TODO: pass exception
	}
	
	Local<Value> argv[2] = {cbvalue, cberror};
	TryCatch try_catch;
	v8callback->Call(v8this, 2, argv);
	if (try_catch.HasCaught()) {
		FatalException(try_catch);
	}
}
