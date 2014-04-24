#include "node_usb.h"

ProtoBuilder::ProtoList ProtoBuilder::initProto;

Handle<Value> SetDebugLevel(const Arguments& args);
Handle<Value> GetDeviceList(const Arguments& args);
void initConstants(Handle<Object> target);

libusb_context* usb_context;

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
uv_thread_t usb_thread;

void USBThreadFn(void*){
	while(1) libusb_handle_events(usb_context);
}
#endif

extern "C" void Initialize(Handle<Object> target) {
	HandleScope  scope;

	// Initialize libusb. On error, halt initialization.
	int res = libusb_init(&usb_context);
	target->Set(String::NewSymbol("INIT_ERROR"), Number::New(res));
	if (res != 0) {
		return;
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
	uv_thread_create(&usb_thread, USBThreadFn, NULL);
	#endif

	ProtoBuilder::initAll(target);

	NODE_SET_METHOD(target, "setDebugLevel", SetDebugLevel);
	NODE_SET_METHOD(target, "getDeviceList", GetDeviceList);
	initConstants(target);
}

NODE_MODULE(usb_bindings, Initialize)

Handle<Value> SetDebugLevel(const Arguments& args) {
	HandleScope scope;
	if (args.Length() != 1 || !args[0]->IsUint32() || args[0]->Uint32Value() > 4) {
		THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-4]]!")
	}
	
	libusb_set_debug(usb_context, args[0]->Uint32Value());
	return Undefined();
}

Handle<Value> GetDeviceList(const Arguments& args) {
	HandleScope scope;
	libusb_device **devs;
	int cnt = libusb_get_device_list(usb_context, &devs);
	CHECK_USB(cnt);

	Handle<Array> arr = Array::New(cnt);

	for(int i = 0; i < cnt; i++) {
		arr->Set(i, Device::get(devs[i]));
	}
	libusb_free_device_list(devs, true);
	return scope.Close(arr);
}

void initConstants(Handle<Object> target){
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PER_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_AUDIO);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_COMM);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HID);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PRINTER);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_PTP);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_MASS_STORAGE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_HUB);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_DATA);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_WIRELESS);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_APPLICATION);
	NODE_DEFINE_CONSTANT(target, LIBUSB_CLASS_VENDOR_SPEC);
	// libusb_standard_request
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_STATUS);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_CLEAR_FEATURE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_FEATURE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_ADDRESS );
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_DESCRIPTOR);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_DESCRIPTOR);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_CONFIGURATION);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_CONFIGURATION );
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_GET_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SET_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_SYNCH_FRAME);
	// libusb_descriptor_type
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_DEVICE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_CONFIG);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_STRING);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_ENDPOINT);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_HID);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_REPORT);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_PHYSICAL);
	NODE_DEFINE_CONSTANT(target, LIBUSB_DT_HUB);
	// libusb_endpoint_direction
	NODE_DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_IN);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ENDPOINT_OUT);
	// libusb_transfer_type
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_CONTROL);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_ISOCHRONOUS);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_BULK);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TYPE_INTERRUPT);
	// libusb_iso_sync_type
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_NONE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ASYNC);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_ADAPTIVE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_SYNC_TYPE_SYNC);
	// libusb_iso_usage_type
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_DATA);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_FEEDBACK);
	NODE_DEFINE_CONSTANT(target, LIBUSB_ISO_USAGE_TYPE_IMPLICIT);
	// libusb_transfer_status
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_COMPLETED);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_ERROR);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_TIMED_OUT);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_CANCELLED);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_STALL);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_NO_DEVICE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_OVERFLOW);
	// libusb_transfer_flags
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_SHORT_NOT_OK);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_BUFFER);
	NODE_DEFINE_CONSTANT(target, LIBUSB_TRANSFER_FREE_TRANSFER);
	// libusb_request_type
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_STANDARD);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_CLASS);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_VENDOR);
	NODE_DEFINE_CONSTANT(target, LIBUSB_REQUEST_TYPE_RESERVED);
	// libusb_request_recipient
	NODE_DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_DEVICE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_INTERFACE);
	NODE_DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_ENDPOINT);
	NODE_DEFINE_CONSTANT(target, LIBUSB_RECIPIENT_OTHER);

	NODE_DEFINE_CONSTANT(target, LIBUSB_CONTROL_SETUP_SIZE);
}

Local<Value> libusbException(int errorno) {
	const char* err = libusb_error_name(errorno);	
	Local<Value> e  = Exception::Error(String::NewSymbol(err));
	e->ToObject()->Set(V8SYM("errno"), Integer::New(errorno));
	return e;
}
