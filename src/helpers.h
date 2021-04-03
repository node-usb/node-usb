#include <vector>
#include <napi.h>

#define THROW_BAD_ARGS(FAIL_MSG) throw Napi::TypeError::New(env, FAIL_MSG);
#define THROW_ERROR(FAIL_MSG) throw Napi::Error::New(env, FAIL_MSG);
#define CHECK_N_ARGS(MIN_ARGS) if (info.Length() < MIN_ARGS) { THROW_BAD_ARGS("Expected " #MIN_ARGS " arguments") }

const napi_property_attributes CONST_PROP = static_cast<napi_property_attributes>(napi_enumerable | napi_configurable);

#define ENTER_CONSTRUCTOR(MIN_ARGS) \
	Napi::HandleScope scope(env);              \
	if (!info.IsConstructCall()) throw Napi::Error::New(env, "Must be called with `new`!"); \
	CHECK_N_ARGS(MIN_ARGS);

#define ENTER_CONSTRUCTOR_POINTER(CLASS, MIN_ARGS) \
	ENTER_CONSTRUCTOR(MIN_ARGS)                    \
	if (!info.Length() || !info[0].IsExternal()){ \
		throw Napi::Error::New(env, "This type cannot be created directly!"); \
	}                                                \
	auto self = this;

#define ENTER_METHOD(CLASS, MIN_ARGS) \
	Napi::Env env = info.Env(); \
	Napi::HandleScope scope(env);                \
	CHECK_N_ARGS(MIN_ARGS);           \
	auto self = this;

#define ENTER_ACCESSOR(CLASS) \
		Napi::HandleScope scope(env);                \
		auto self = info.Holder().Unwrap<CLASS>();

#define UNWRAP_ARG(CLASS, NAME, ARGNO)     \
	if (!info[ARGNO].IsObject())          \
		THROW_BAD_ARGS("Parameter " #NAME " is not an object"); \
	auto NAME = Napi::ObjectWrap<CLASS>::Unwrap(info[ARGNO].As<Napi::Object>()); \
	if (!NAME)                             \
		THROW_BAD_ARGS("Parameter " #NAME " (" #ARGNO ") is of incorrect type");

#define STRING_ARG(NAME, N) \
	if (info.Length() > N){ \
		if (!info[N].IsString()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be string"); \
		NAME = *String::Utf8Value(info[N].ToString()); \
	}

#define DOUBLE_ARG(NAME, N) \
	if (!info[N].IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N].ToNumber()->Value();

#define INT_ARG(NAME, N) \
	if (!info[N].IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N].As<Napi::Number>().Int32Value();

#define BOOL_ARG(NAME, N) \
	NAME = false;    \
	if (info.Length() > N){ \
		if (!info[N].IsBoolean()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be bool"); \
		NAME = info[N].ToBoolean()->Value(); \
	}
