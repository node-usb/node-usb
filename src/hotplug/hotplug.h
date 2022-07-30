#ifndef _USB_HOTPLUG_H
#define _USB_HOTPLUG_H

#include "../node_usb.h"

#define HOTPLUG_SUPPORTS_NONE 0
#define HOTPLUG_SUPPORTS_DEVICES 1
#define HOTPLUG_SUPPORTS_IDS 2

class HotPlugManager {
public:
    static std::unique_ptr<HotPlugManager> create();

    virtual int supportedHotplugEvents() = 0;

    virtual void enableHotplug(const Napi::Env& env, ModuleData* instanceData) = 0;
    virtual void disableHotplug(const Napi::Env& env, ModuleData* instanceData) = 0;
};

void handleHotplug(HotPlug* info);

#endif
