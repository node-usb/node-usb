#include "node_usb.h"
#include "uv_async_queue.h"
#include <thread>

Napi::Value SetDebugLevel(const Napi::CallbackInfo& info);
Napi::Value GetDeviceList(const Napi::CallbackInfo& info);
Napi::Value EnableHotplugEvents(const Napi::CallbackInfo& info);
Napi::Value DisableHotplugEvents(const Napi::CallbackInfo& info);
void initConstants(Napi::Object target);

libusb_context* usb_context;
struct HotPlug {
	libusb_device* device;
	libusb_hotplug_event event;
};

#ifdef USE_POLL
#include <poll.h>
#include <sys/time.h>

std::map<int, uv_poll_t*> pollByFD;

struct timeval zero_tv = {0, 0};

void onPollSuccess(uv_poll_t* handle, int status, int events){
	libusb_handle_events_timeout(usb_context, &zero_tv);
}

void LIBUSB_CALL onPollFDAdded(int fd, short events, void *user_data){
	uv_poll_t *poll_fd;
	auto it = pollByFD.find(fd);
	if (it != pollByFD.end()){
		poll_fd = it->second;
	}else{
		poll_fd = (uv_poll_t*) malloc(sizeof(uv_poll_t));
		uv_poll_init(uv_default_loop(), poll_fd, fd);
		pollByFD.insert(std::make_pair(fd, poll_fd));
	}

	DEBUG_LOG("Added pollfd %i, %p", fd, poll_fd);
	unsigned flags = ((events&POLLIN) ? UV_READABLE:0)
	               | ((events&POLLOUT)? UV_WRITABLE:0);
	uv_poll_start(poll_fd, flags, onPollSuccess);
}

void LIBUSB_CALL onPollFDRemoved(int fd, void *user_data){
	auto it = pollByFD.find(fd);
	if (it != pollByFD.end()){
		DEBUG_LOG("Removed pollfd %i, %p", fd, it->second);
		uv_poll_stop(it->second);
		uv_close((uv_handle_t*) it->second, (uv_close_cb) free);
		pollByFD.erase(it);
	}
}

#else
std::thread usb_thread;

void USBThreadFn(){
	while(1) libusb_handle_events(usb_context);
}
#endif

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	Napi::HandleScope scope(env);

	initConstants(exports);

	// Initialize libusb. On error, halt initialization.
	int res = libusb_init(&usb_context);
	exports.Set("INIT_ERROR", Napi::Number::New(env, res));
	if (res != 0) {
		return exports;
	}

	#ifdef USE_POLL
	assert(libusb_pollfds_handle_timeouts(usb_context));
	libusb_set_pollfd_notifiers(usb_context, onPollFDAdded, onPollFDRemoved, NULL);

	const struct libusb_pollfd** pollfds = libusb_get_pollfds(usb_context);
	assert(pollfds);
	for(const struct libusb_pollfd** i=pollfds; *i; i++){
		onPollFDAdded((*i)->fd, (*i)->events, NULL);
	}
	free(pollfds);

	#else
	usb_thread = std::thread(USBThreadFn);
	usb_thread.detach();
	#endif

	Device::Init(env, exports);
	Transfer::Init(env, exports);

	exports.Set("setDebugLevel", Napi::Function::New(env, SetDebugLevel));
	exports.Set("getDeviceList", Napi::Function::New(env, GetDeviceList));
	exports.Set("_enableHotplugEvents", Napi::Function::New(env, EnableHotplugEvents));
	exports.Set("_disableHotplugEvents", Napi::Function::New(env, DisableHotplugEvents));
	return exports;
}

NODE_API_MODULE(usb_bindings, Init)

Napi::Value SetDebugLevel(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	if (info.Length() != 1 || !info[0].IsNumber() || info[0].As<Napi::Number>().Uint32Value() > 4) {
		THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-4]]!")
	}

	libusb_set_debug(usb_context, info[0].As<Napi::Number>().Int32Value());
	return env.Undefined();
}

Napi::Value GetDeviceList(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	libusb_device **devs;
	int cnt = libusb_get_device_list(usb_context, &devs);
	CHECK_USB(cnt);

	Napi::Array arr = Napi::Array::New(env, cnt);

	for(int i = 0; i < cnt; i++) {
		arr.Set(i, Device::get(env, devs[i]));
	}
	libusb_free_device_list(devs, true);
	return arr;
}

Napi::ObjectReference hotplugThis;

void handleHotplug(HotPlug* info){
	Napi::Env env = hotplugThis.Env();
	Napi::HandleScope scope(env);

	libusb_device* dev = info->device;
	libusb_hotplug_event event = info->event;
	delete info;

	DEBUG_LOG("HandleHotplug %p %i", dev, event);

	Napi::Value v8dev = Device::get(env, dev);
	libusb_unref_device(dev);

	Napi::String eventName;
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		DEBUG_LOG("Device arrived");
		eventName = Napi::String::New(env, "attach");

	} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		DEBUG_LOG("Device left");
		eventName = Napi::String::New(env, "detach");

	} else {
		DEBUG_LOG("Unhandled hotplug event %d\n", event);
		return;
	}

	hotplugThis.Get("emit").As<Napi::Function>().MakeCallback(hotplugThis.Value(), { eventName, v8dev });
}

bool hotplugEnabled = 0;
libusb_hotplug_callback_handle hotplugHandle;
UVQueue<HotPlug*> hotplugQueue(handleHotplug);

int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *device,
                     libusb_hotplug_event event, void *user_data) {
	libusb_ref_device(device);
	hotplugQueue.post(new HotPlug {device, event});
	return 0;
}

Napi::Value EnableHotplugEvents(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (!hotplugEnabled) {
		hotplugThis.Reset(info.This().As<Napi::Object>(), 1);
		hotplugThis.SuppressDestruct();
		CHECK_USB(libusb_hotplug_register_callback(usb_context,
			(libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
			(libusb_hotplug_flag)0, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
			hotplug_callback, NULL, &hotplugHandle));
		hotplugQueue.start(env);
		hotplugEnabled = true;
	}
	return env.Undefined();
}

Napi::Value DisableHotplugEvents(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	if (hotplugEnabled) {
		libusb_hotplug_deregister_callback(usb_context, hotplugHandle);
		hotplugQueue.stop();
		hotplugEnabled = false;
	}
	return env.Undefined();
}

#define DEFINE_CONSTANT(OBJ, VALUE) \
	OBJ.DefineProperty(Napi::PropertyDescriptor::Value(#VALUE, Napi::Number::New(OBJ.Env(), VALUE), static_cast<napi_property_attributes>(napi_enumerable | napi_configurable)));


void initConstants(Napi::Object target){
	DEFINE_CONSTANT(target, LIBUSB_CLASS_PER_INTERFACE);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_AUDIO);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_COMM);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_HID);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_PRINTER);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_PTP);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_MASS_STORAGE);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_HUB);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_DATA);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_WIRELESS);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_APPLICATION);
	DEFINE_CONSTANT(target, LIBUSB_CLASS_VENDOR_SPEC);
	// libusb_standard_request
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_STATUS);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_CLEAR_FEATURE);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_FEATURE);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_ADDRESS );
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_DESCRIPTOR);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_DESCRIPTOR);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_CONFIGURATION);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_CONFIGURATION );
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_INTERFACE);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_INTERFACE);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_SYNCH_FRAME);
	// libusb_descriptor_type
	DEFINE_CONSTANT(target, LIBUSB_DT_DEVICE);
	DEFINE_CONSTANT(target, LIBUSB_DT_CONFIG);
	DEFINE_CONSTANT(target, LIBUSB_DT_STRING);
	DEFINE_CONSTANT(target, LIBUSB_DT_BOS);
	DEFINE_CONSTANT(target, LIBUSB_DT_INTERFACE);
	DEFINE_CONSTANT(target, LIBUSB_DT_ENDPOINT);
	DEFINE_CONSTANT(target, LIBUSB_DT_HID);
	DEFINE_CONSTANT(target, LIBUSB_DT_REPORT);
	DEFINE_CONSTANT(target, LIBUSB_DT_PHYSICAL);
	DEFINE_CONSTANT(target, LIBUSB_DT_HUB);
	// libusb_endpoint_direction
	DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_IN);
	DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_OUT);
	// libusb_transfer_type
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_CONTROL);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_ISOCHRONOUS);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_BULK);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_INTERRUPT);
	// libusb_iso_sync_type
	DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_NONE);
	DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ASYNC);
	DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ADAPTIVE);
	DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_SYNC);
	// libusb_iso_usage_type
	DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_DATA);
	DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_FEEDBACK);
	DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_IMPLICIT);
	// libusb_transfer_status
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_COMPLETED);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_ERROR);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TIMED_OUT);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_CANCELLED);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_STALL);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_NO_DEVICE);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_OVERFLOW);
	// libusb_transfer_flags
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_SHORT_NOT_OK);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_BUFFER);
	DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_TRANSFER);
	// libusb_request_type
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_STANDARD);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_CLASS);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_VENDOR);
	DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_RESERVED);
	// libusb_request_recipient
	DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_DEVICE);
	DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_INTERFACE);
	DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_ENDPOINT);
	DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_OTHER);

	DEFINE_CONSTANT(target, LIBUSB_CONTROL_SETUP_SIZE);
	DEFINE_CONSTANT(target, LIBUSB_DT_BOS_SIZE);

	// libusb_error
	// Input/output error
	DEFINE_CONSTANT(target, LIBUSB_ERROR_IO);
	// Invalid parameter
	DEFINE_CONSTANT(target, LIBUSB_ERROR_INVALID_PARAM);
	// Access denied (insufficient permissions)
	DEFINE_CONSTANT(target, LIBUSB_ERROR_ACCESS);
	// No such device (it may have been disconnected)
	DEFINE_CONSTANT(target, LIBUSB_ERROR_NO_DEVICE);
	// Entity not found
	DEFINE_CONSTANT(target, LIBUSB_ERROR_NOT_FOUND);
	// Resource busy
	DEFINE_CONSTANT(target, LIBUSB_ERROR_BUSY);
	// Operation timed out
	DEFINE_CONSTANT(target, LIBUSB_ERROR_TIMEOUT);
	// Overflow
	DEFINE_CONSTANT(target, LIBUSB_ERROR_OVERFLOW);
	// Pipe error
	DEFINE_CONSTANT(target, LIBUSB_ERROR_PIPE);
	// System call interrupted (perhaps due to signal)
	DEFINE_CONSTANT(target, LIBUSB_ERROR_INTERRUPTED);
	// Insufficient memory
	DEFINE_CONSTANT(target, LIBUSB_ERROR_NO_MEM);
	// Operation not supported or unimplemented on this platform
	DEFINE_CONSTANT(target, LIBUSB_ERROR_NOT_SUPPORTED);
	// Other error
	DEFINE_CONSTANT(target, LIBUSB_ERROR_OTHER);
}

Napi::Error libusbException(napi_env env, int errorno) {
	const char* err = libusb_error_name(errorno);
	Napi::Error e  = Napi::Error::New(env, err);
	e.Set("errno", (double)errorno);
	return e;
}
