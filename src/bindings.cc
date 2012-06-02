#include "bindings.h"

using namespace NodeUsb;

bool AllowConstructor::m_current;

Local<v8::Value> NodeUsb::makeBuffer(const uint8_t* buf, unsigned length){
	HandleScope scope;
	Buffer *slowBuffer = Buffer::New((char *)buf, length);
	Local<Object> globalObj = v8::Context::GetCurrent()->Global();
	Local<Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
	Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(length), v8::Integer::New(0) };
	Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
	return scope.Close(actualBuffer);
}

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
