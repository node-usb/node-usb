#include "hotplug.h"

struct HotPlug {
    libusb_device* device;
    libusb_hotplug_event event;
    Napi::ObjectReference* hotplugThis;
};

int LIBUSB_CALL hotplug_callback(libusb_context* ctx, libusb_device* device, libusb_hotplug_event event, void* user_data) {
    libusb_ref_device(device);
    ModuleData* instanceData = (ModuleData*)user_data;
    instanceData->hotplugQueue.post(new HotPlug {device, event, &instanceData->hotplugThis});
    return 0;
}

class HotPlugManagerLibUsb: public HotPlugManager {
    int supportedHotplugEvents() {
        int res = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);

        return res > 0 ? HOTPLUG_SUPPORTS_DEVICES : HOTPLUG_SUPPORTS_NONE;
    }

    void enableHotplug(const Napi::Env& env, ModuleData* instanceData) {
        libusb_context* usb_context = instanceData->usb_context;
        CHECK_USB(libusb_hotplug_register_callback(
            usb_context,
            (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
            (libusb_hotplug_flag)0,
            LIBUSB_HOTPLUG_MATCH_ANY,
            LIBUSB_HOTPLUG_MATCH_ANY,
            LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback,
            instanceData,
            &hotplugHandle
        ));
    }

    void disableHotplug(const Napi::Env& env, ModuleData* instanceData) {
        libusb_context* usb_context = instanceData->usb_context;
        libusb_hotplug_deregister_callback(usb_context, hotplugHandle);
    }
    
    libusb_hotplug_callback_handle hotplugHandle;
};

std::unique_ptr<HotPlugManager> HotPlugManager::create() {
    return std::make_unique<HotPlugManagerLibUsb>();
}

void handleHotplug(HotPlug* info) {
    Napi::ObjectReference* hotplugThis = info->hotplugThis;
    Napi::Env env = hotplugThis->Env();
    Napi::HandleScope scope(env);

    libusb_device* dev = info->device;
    libusb_hotplug_event event = info->event;
    delete info;

    DEBUG_LOG("HandleHotplug %p %i", dev, event);

    Napi::Object v8dev = Device::get(env, dev);
    libusb_unref_device(dev);

    Napi::Object v8VidPid = Napi::Object::New(env);
    auto deviceDescriptor = v8dev.Get("deviceDescriptor");
    if (deviceDescriptor.IsObject()) {
        v8VidPid.Set("idVendor", deviceDescriptor.As<Napi::Object>().Get("idVendor"));
        v8VidPid.Set("idProduct", deviceDescriptor.As<Napi::Object>().Get("idProduct"));
    }

    Napi::String eventName;
    Napi::String changeEventName;
    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
        DEBUG_LOG("Device arrived");
        eventName = Napi::String::New(env, "attach");
        changeEventName = Napi::String::New(env, "attachIds");

    } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
        DEBUG_LOG("Device left");
        eventName = Napi::String::New(env, "detach");
        changeEventName = Napi::String::New(env, "detachIds");

    } else {
        DEBUG_LOG("Unhandled hotplug event %d\n", event);
        return;
    }

    hotplugThis->Get("emit").As<Napi::Function>().MakeCallback(hotplugThis->Value(), { eventName, v8dev });
    hotplugThis->Get("emit").As<Napi::Function>().MakeCallback(hotplugThis->Value(), { changeEventName, v8VidPid });
}
