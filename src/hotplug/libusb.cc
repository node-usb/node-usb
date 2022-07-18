#include "hotplug.h"

class HotPlugManagerLibUsb: public HotPlugManager {
    bool supportsHotplug() {
        int res = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);

        return res > 0;
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

int LIBUSB_CALL hotplug_callback(libusb_context* ctx, libusb_device* device,
                     libusb_hotplug_event event, void* user_data) {
	libusb_ref_device(device);
	ModuleData* instanceData = (ModuleData*)user_data;
	instanceData->hotplugQueue.post(new HotPlug {device, nullptr, event, &instanceData->hotplugThis});
	return 0;
}

void handleHotplug(HotPlug* info) {
	Napi::ObjectReference* hotplugThis = info->hotplugThis;
	Napi::Env env = hotplugThis->Env();
	Napi::HandleScope scope(env);

	libusb_device* dev = info->device;
	libusb_hotplug_event event = info->event;

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

	hotplugThis->Get("emit").As<Napi::Function>().MakeCallback(hotplugThis->Value(), { eventName, v8dev });
	delete info;
}
