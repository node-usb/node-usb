#include "node_usb.h"
#include "thread_name.h"
#include "hotplug/hotplug.h"

Napi::Value SetDebugLevel(const Napi::CallbackInfo& info);
Napi::Value UseUsbDkBackend(const Napi::CallbackInfo& info);
Napi::Value GetDeviceList(const Napi::CallbackInfo& info);
Napi::Value GetLibusbCapability(const Napi::CallbackInfo& info);
Napi::Value SupportedHotplugEvents(const Napi::CallbackInfo& info);
Napi::Value EnableHotplugEvents(const Napi::CallbackInfo& info);
Napi::Value DisableHotplugEvents(const Napi::CallbackInfo& info);
Napi::Value RefHotplugEvents(const Napi::CallbackInfo& info);
Napi::Value UnrefHotplugEvents(const Napi::CallbackInfo& info);
void initConstants(Napi::Object target);

void USBThreadFn(ModuleData* instanceData) {
    SetThreadName("node-usb events");
    libusb_context* usb_context = instanceData->usb_context;

    while(true) {
        if (instanceData->handlingEvents == false) {
            break;
        }
        libusb_handle_events(usb_context);
    }
}

ModuleData::ModuleData(libusb_context* usb_context) : usb_context(usb_context), hotplugQueue(handleHotplug) {
    handlingEvents = true;
    usb_thread = std::thread(USBThreadFn, this);
    hotplugManager = HotPlugManager::create();
}

ModuleData::~ModuleData() {
    handlingEvents = false;
    libusb_interrupt_event_handler(usb_context);
    usb_thread.join();

    if (usb_context != nullptr) {
        libusb_exit(usb_context);
        usb_context = nullptr;
    }
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    initConstants(exports);

    // Initialize libusb. On error, halt initialization.
    libusb_context* usb_context = nullptr;
    int res = libusb_init(&usb_context);

    exports.Set("INIT_ERROR", Napi::Number::New(env, res));
    if (res != 0) {
        return exports;
    }

    env.SetInstanceData(new ModuleData(usb_context));

    Device::Init(env, exports);
    Transfer::Init(env, exports);

    exports.Set("setDebugLevel", Napi::Function::New(env, SetDebugLevel));
    exports.Set("useUsbDkBackend", Napi::Function::New(env, UseUsbDkBackend));
    exports.Set("getDeviceList", Napi::Function::New(env, GetDeviceList));
    exports.Set("_getLibusbCapability", Napi::Function::New(env, GetLibusbCapability));
    exports.Set("_supportedHotplugEvents", Napi::Function::New(env, SupportedHotplugEvents));
    exports.Set("_enableHotplugEvents", Napi::Function::New(env, EnableHotplugEvents));
    exports.Set("_disableHotplugEvents", Napi::Function::New(env, DisableHotplugEvents));
    exports.Set("refHotplugEvents", Napi::Function::New(env, RefHotplugEvents));
    exports.Set("unrefHotplugEvents", Napi::Function::New(env, UnrefHotplugEvents));
    return exports;
}

NODE_API_MODULE(usb_bindings, Init)

Napi::Value SetDebugLevel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() != 1 || !info[0].IsNumber() || info[0].As<Napi::Number>().Uint32Value() > 4) {
        THROW_BAD_ARGS("Usb::SetDebugLevel argument is invalid. [uint:[0-4]]!")
    }

    libusb_context* usb_context = env.GetInstanceData<ModuleData>()->usb_context;
    libusb_set_option(usb_context, LIBUSB_OPTION_LOG_LEVEL, info[0].As<Napi::Number>().Int32Value());
    return env.Undefined();
}

Napi::Value UseUsbDkBackend(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    libusb_context* usb_context = env.GetInstanceData<ModuleData>()->usb_context;
    libusb_set_option(usb_context, LIBUSB_OPTION_USE_USBDK);
    return env.Undefined();
}

Napi::Value GetDeviceList(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    libusb_device** devs;

    libusb_context* usb_context = env.GetInstanceData<ModuleData>()->usb_context;
    int cnt = libusb_get_device_list(usb_context, &devs);
    CHECK_USB(cnt);

    Napi::Array arr = Napi::Array::New(env, cnt);

    for(int i = 0; i < cnt; i++) {
        arr.Set(i, Device::get(env, devs[i]));
    }
    libusb_free_device_list(devs, true);
    return arr;
}

Napi::Value GetLibusbCapability(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsNumber()) {
        THROW_BAD_ARGS("Usb::GetLibusbCapability argument is invalid!")
    }

    int res = libusb_has_capability(info[0].As<Napi::Number>().Int32Value());
    return Napi::Number::New(env, res);
}

Napi::Value SupportedHotplugEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    ModuleData* instanceData = env.GetInstanceData<ModuleData>();

    int res = instanceData->hotplugManager->supportedHotplugEvents();
    return Napi::Number::New(env, res);
}

Napi::Value EnableHotplugEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    ModuleData* instanceData = env.GetInstanceData<ModuleData>();

    if (!instanceData->hotplugEnabled) {
        instanceData->hotplugThis.Reset(info.This().As<Napi::Object>(), 1);

        // Start queue, then enable hotplug events
        instanceData->hotplugQueue.start(env);
        instanceData->hotplugManager->enableHotplug(env, instanceData);
        
        instanceData->hotplugEnabled = true;
    }
    return env.Undefined();
}

Napi::Value DisableHotplugEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    ModuleData* instanceData = env.GetInstanceData<ModuleData>();

    if (instanceData->hotplugEnabled) {

        // Disable events, then stop queue
        instanceData->hotplugManager->disableHotplug(env, instanceData);
        instanceData->hotplugQueue.stop();

        instanceData->hotplugEnabled = false;
    }
    return env.Undefined();
}

Napi::Value RefHotplugEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    ModuleData* instanceData = env.GetInstanceData<ModuleData>();

    if (instanceData->hotplugEnabled) {
        instanceData->hotplugQueue.ref(env);
    }
    return env.Undefined();
}

Napi::Value UnrefHotplugEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    ModuleData* instanceData = env.GetInstanceData<ModuleData>();

    if (instanceData->hotplugEnabled) {
        instanceData->hotplugQueue.unref(env);
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

    // libusb_capability
    DEFINE_CONSTANT(target, LIBUSB_CAP_HAS_CAPABILITY);
    DEFINE_CONSTANT(target, LIBUSB_CAP_HAS_HOTPLUG);
    DEFINE_CONSTANT(target, LIBUSB_CAP_HAS_HID_ACCESS);
    DEFINE_CONSTANT(target, LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER);

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

Napi::Error libusbException(Napi::Env env, int errorno) {
    const char* err = libusb_error_name(errorno);
    Napi::Error e  = Napi::Error::New(env, err);
    e.Set("errno", (double)errorno);
    return e;
}
