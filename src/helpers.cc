#include "node_usb.h"

Local<Value> libusbException(int errorno) {
	const char* err = libusb_error_name(errorno);	
	Local<Value> e  = Exception::Error(String::NewSymbol(err));
	e->ToObject()->Set(V8SYM("errno"), Integer::New(errorno));
	return e;
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

