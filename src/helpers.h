#include <vector>
#include <v8.h>
#include <nan.h>
#include "polyfill.h"

using namespace v8;

#define V8STR(str) Nan::New<String>(str).ToLocalChecked()
#define V8SYM(str) Nan::New<String>(str).ToLocalChecked()

#define THROW_BAD_ARGS(FAIL_MSG) return Nan::ThrowTypeError(FAIL_MSG);
#define THROW_ERROR(FAIL_MSG) return Nan::ThrowError(FAIL_MSG);
#define CHECK_N_ARGS(MIN_ARGS) if (info.Length() < MIN_ARGS) { THROW_BAD_ARGS("Expected " #MIN_ARGS " arguments") }

const PropertyAttribute CONST_PROP = static_cast<PropertyAttribute>(ReadOnly|DontDelete);

inline static void setConst(Local<Object> obj, const char* const name, Local<Value> value){
	obj->ForceSet(Nan::New<String>(name).ToLocalChecked(), value, CONST_PROP);
}

#define ENTER_CONSTRUCTOR(MIN_ARGS) \
	Nan::HandleScope scope;              \
	if (!info.IsConstructCall()) return Nan::ThrowError("Must be called with `new`!"); \
	CHECK_N_ARGS(MIN_ARGS);

#define ENTER_CONSTRUCTOR_POINTER(CLASS, MIN_ARGS) \
	ENTER_CONSTRUCTOR(MIN_ARGS)                    \
	if (!info.Length() || !info[0]->IsExternal()){ \
		return Nan::ThrowError("This type cannot be created directly!"); \
	}                                               \
	auto self = static_cast<CLASS*>(External::Cast(*info[0])->Value()); \
	self->attach(info.This())

#define ENTER_METHOD(CLASS, MIN_ARGS) \
	Nan::HandleScope scope;                \
	CHECK_N_ARGS(MIN_ARGS);           \
	auto self = Nan::ObjectWrap::Unwrap<CLASS>(info.This()); \
	if (self == NULL) { THROW_BAD_ARGS(#CLASS " method called on invalid object") }

#define ENTER_ACCESSOR(CLASS) \
		Nan::HandleScope scope;                \
		auto self = Nan::ObjectWrap::Unwrap<CLASS>(info.Holder());

#define UNWRAP_ARG(CLASS, NAME, ARGNO)     \
	if (!info[ARGNO]->IsObject())          \
		THROW_BAD_ARGS("Parameter " #NAME " is not an object"); \
	auto NAME = Nan::ObjectWrap::Unwrap<CLASS>(Local<Object>::Cast(info[ARGNO])); \
	if (!NAME)                             \
		THROW_BAD_ARGS("Parameter " #NAME " (" #ARGNO ") is of incorrect type");

#define STRING_ARG(NAME, N) \
	if (info.Length() > N){ \
		if (!info[N]->IsString()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be string"); \
		NAME = *String::Utf8Value(info[N]->ToString()); \
	}

#define DOUBLE_ARG(NAME, N) \
	if (!info[N]->IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N]->ToNumber()->Value();

#define INT_ARG(NAME, N) \
	if (!info[N]->IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N]->ToInt32()->Value();

#define BOOL_ARG(NAME, N) \
	NAME = false;    \
	if (info.Length() > N){ \
		if (!info[N]->IsBoolean()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be bool"); \
		NAME = info[N]->ToBoolean()->Value(); \
	}
