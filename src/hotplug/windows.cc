#include "hotplug.h"

// Include Windows headers
#include <windows.h>
#include <Cfgmgr32.h>

#include <locale>
#include <codecvt>

/**********************************
 * Local Variables
 **********************************/
GUID GUID_DEVINTERFACE_USB_DEVICE = {
    0xA5DCBF10L,
    0x6530,
    0x11D2,
    0x90,
    0x1F,
    0x00,
    0xC0,
    0x4F,
    0xB9,
    0x51,
    0xED};
    
struct HotPlug {
	std::wstring path;
	CM_NOTIFY_ACTION event;
	Napi::ObjectReference* hotplugThis;
};

DWORD MyCMInterfaceNotification(HCMNOTIFICATION hNotify, PVOID Context, CM_NOTIFY_ACTION Action, PCM_NOTIFY_EVENT_DATA EventData, DWORD EventDataSize)
{
    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
    {
	    ModuleData* instanceData = (ModuleData*)Context;
        auto str = std::wstring(EventData->u.DeviceInterface.SymbolicLink);
	    instanceData->hotplugQueue.post(new HotPlug {str, Action, &instanceData->hotplugThis});
        break;
    }
    default:
        break;
    }
    return 0;
}

class HotPlugManagerWindows : public HotPlugManager
{
    bool supportsHotplug()
    {
        return true;
    }

    void enableHotplug(const Napi::Env &env, ModuleData *instanceData)
    {
        if (isRunning)
        {
            return;
        }

        isRunning = true;

        CM_NOTIFY_FILTER cmNotifyFilter = { 0 };
        cmNotifyFilter.cbSize = sizeof(cmNotifyFilter);
        cmNotifyFilter.Flags = 0;
        cmNotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        cmNotifyFilter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_USB_DEVICE;
    
        auto res = CM_Register_Notification(&cmNotifyFilter, (PVOID)instanceData, (PCM_NOTIFY_CALLBACK)&MyCMInterfaceNotification, &hcm);
        if (res != CR_SUCCESS)
        {
            printf("CM_Register_Notification() failed [Error: %x]\r\n", res);
        }
    }

    void disableHotplug(const Napi::Env &env, ModuleData *instanceData)
    {
        if (isRunning)
        {
            isRunning = false;

            if (hcm) {
                CM_Unregister_Notification(hcm);
                hcm = nullptr;
            }
        }
    }

private:
    std::atomic<bool> isRunning = {false};
    HCMNOTIFICATION hcm;
};

std::unique_ptr<HotPlugManager> HotPlugManager::create()
{
    return std::make_unique<HotPlugManagerWindows>();
}

void handleHotplug(HotPlug* info) {
	Napi::ObjectReference* hotplugThis = info->hotplugThis;
	Napi::Env env = hotplugThis->Env();
	Napi::HandleScope scope(env);

    std::string path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(info->path);
    auto v8Path = Napi::String::New(env, path);
	CM_NOTIFY_ACTION event = info->event;

	DEBUG_LOG("HandleHotplug %s %i", info->path, event);

	Napi::String eventName;
	if (CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL == event) {
		DEBUG_LOG("Device arrived");
		eventName = Napi::String::New(env, "attachPath");

	} else if (CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL == event) {
		DEBUG_LOG("Device left");
		eventName = Napi::String::New(env, "detachPath");

	} else {
		DEBUG_LOG("Unhandled hotplug event %d\n", event);
		return;
	}

	hotplugThis->Get("emit").As<Napi::Function>().MakeCallback(hotplugThis->Value(), { eventName, v8Path });
	delete info;
}
