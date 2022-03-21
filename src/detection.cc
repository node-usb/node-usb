#include "detection.h"

#define OBJECT_ITEM_LOCATION_ID "locationId"
#define OBJECT_ITEM_VENDOR_ID "vendorId"
#define OBJECT_ITEM_PRODUCT_ID "productId"
#define OBJECT_ITEM_DEVICE_NAME "deviceName"
#define OBJECT_ITEM_MANUFACTURER "manufacturer"
#define OBJECT_ITEM_SERIAL_NUMBER "serialNumber"
#define OBJECT_ITEM_DEVICE_ADDRESS "deviceAddress"

struct DeviceCallbackItem
{
	std::string type;
	std::shared_ptr<ListResultItem_t> item;
};

static void DeviceItemChangeCallback(Napi::Env env, Napi::Function jsCallback, DeviceCallbackItem *data)
{
	Napi::Value type = Napi::String::From(env, data->type);
	Napi::Value value = DeviceItemToObject(env, data->item);
	jsCallback.Call({type, value});

	// We're finished with the data.
	delete data;
};

Detection::~Detection()
{
}

Napi::Value Detection::StartMonitoring(const Napi::CallbackInfo &args)
{
	Napi::Env env = args.Env();

	if (args.Length() == 0)
	{
		Napi::TypeError::New(env, "First argument must be a function").ThrowAsJavaScriptException();
		return env.Null();
	}

	// callback
	if (!args[0].IsFunction())
	{
		Napi::TypeError::New(env, "First argument must be a function").ThrowAsJavaScriptException();
		return env.Null();
	}

	if (!Start(env, args[0].As<Napi::Function>()))
	{
		Napi::Error::New(args.Env(), "Failed to start monitoring").ThrowAsJavaScriptException();
		return env.Null();
	}

	// TODO - register a napi_add_env_cleanup_hook ?

	return env.Null();
}

void Detection::StopMonitoring(const Napi::CallbackInfo &args)
{
	Stop();
}

Napi::Value Detection::IsMonitoring(const Napi::CallbackInfo &args)
{
	Napi::Env env = args.Env();

	bool isRunning = IsRunning();
	return Napi::Boolean::From(env, isRunning);
}

Napi::Value Detection::FindDevices(const Napi::CallbackInfo &args)
{
	Napi::Env env = args.Env();

	int vid = 0;
	int pid = 0;

	if (args.Length() >= 2)
	{
		if (args[0].IsNumber() && args[1].IsNumber())
		{
			vid = (int)args[0].As<Napi::Number>().Int32Value();
			pid = (int)args[1].As<Napi::Number>().Int32Value();
		}
	}

	if (args.Length() == 1)
	{
		if (args[0].IsNumber())
		{
			vid = (int)args[0].As<Napi::Number>().Int32Value();
		}
	}

	auto devices = deviceMap.filterItems(vid, pid);

	Napi::Array results = Napi::Array::New(env);
	int i = 0;
	for (auto it = devices.begin(); it != devices.end(); it++, i++)
	{
		results.Set(i, DeviceItemToObject(env, *it));
	}

	return results;
}

void Detection::DeviceAdded(const std::shared_ptr<ListResultItem_t> item)
{
	DeviceCallbackItem *data = new DeviceCallbackItem;
	data->item = item;
	data->type = "add";

	napi_status status = notify_func.BlockingCall(data, DeviceItemChangeCallback);
	if (status != napi_ok)
	{
		// Handle error
		// TODO
	}
}

void Detection::DeviceRemoved(const std::shared_ptr<ListResultItem_t> item)
{
	DeviceCallbackItem *data = new DeviceCallbackItem;
	data->item = item;
	data->type = "remove";

	napi_status status = notify_func.BlockingCall(data, DeviceItemChangeCallback);
	if (status != napi_ok)
	{
		// Handle error
		// TODO
	}
}

Napi::Value DeviceItemToObject(const Napi::Env &env, std::shared_ptr<ListResultItem_t> it)
{
	Napi::Object item = Napi::Object::New(env);
	item.Set(Napi::String::New(env, OBJECT_ITEM_LOCATION_ID), Napi::Number::New(env, it->locationId));
	item.Set(Napi::String::New(env, OBJECT_ITEM_VENDOR_ID), Napi::Number::New(env, it->vendorId));
	item.Set(Napi::String::New(env, OBJECT_ITEM_PRODUCT_ID), Napi::Number::New(env, it->productId));
	item.Set(Napi::String::New(env, OBJECT_ITEM_DEVICE_NAME), Napi::String::New(env, it->deviceName.c_str()));
	item.Set(Napi::String::New(env, OBJECT_ITEM_MANUFACTURER), Napi::String::New(env, it->manufacturer.c_str()));
	item.Set(Napi::String::New(env, OBJECT_ITEM_SERIAL_NUMBER), Napi::String::New(env, it->serialNumber.c_str()));
	item.Set(Napi::String::New(env, OBJECT_ITEM_DEVICE_ADDRESS), Napi::Number::New(env, it->deviceAddress));
	return item;
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
	InitializeDetection(env, exports);
	return exports;
}

NODE_API_MODULE(detection, init);
