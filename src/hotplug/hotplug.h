#ifndef _USB_HOTPLUG_H
#define _USB_HOTPLUG_H

#include "../node_usb.h"

class HotPlugManager {
public:
	static std::unique_ptr<HotPlugManager> create();

	virtual bool supportsHotplug() = 0;

	virtual void enableHotplug(const Napi::Env& env, ModuleData* instanceData) = 0;
	virtual void disableHotplug(const Napi::Env& env, ModuleData* instanceData) = 0;
};

void handleHotplug(HotPlug* info);

#endif
