#include "hotplug.h"

// Include Windows headers
#include <windows.h>
#include <initguid.h>
#include <Cfgmgr32.h>
#include <usbiodef.h>

#include <locale>
#include <codecvt>
#include <cwctype>

#define VID_TAG L"VID_"
#define PID_TAG L"PID_"

struct HotPlug {
    int vid;
    int pid;
    CM_NOTIFY_ACTION event;
    Napi::ObjectReference* hotplugThis;
};

void extractVidPid(wchar_t *buf, int *vid, int *pid)
{
    // Example input: \\?\USB#VID_0FD9&PID_0060#000000000000#{a5dcbf10-6530-11d2-901f-00c04fb951ed}

    *vid = 0;
    *pid = 0;

    if (buf == NULL)
    {
        return;
    }
    
    auto string = std::wstring(buf);
    std::transform(string.begin(), string.end(), string.begin(), std::towupper);

    wchar_t* temp = new wchar_t[5];
    temp[4] = L'\0';

    const wchar_t *vidStr = wcsstr(string.data(), VID_TAG);
    const wchar_t *pidStr = wcsstr(string.data(), PID_TAG);

    if (vidStr != nullptr)
    {
        memcpy(temp, vidStr + wcslen(VID_TAG), 4 * sizeof(wchar_t));
        *vid = wcstol(temp, NULL, 16);
    }

    if (pidStr != nullptr)
    {
        memcpy(temp, pidStr + wcslen(PID_TAG), 4 * sizeof(wchar_t));
        *pid = wcstol(temp, NULL, 16);
    }
}

DWORD WINAPI MyCMInterfaceNotification(HCMNOTIFICATION hNotify, PVOID Context, CM_NOTIFY_ACTION Action, PCM_NOTIFY_EVENT_DATA EventData, DWORD EventDataSize)
{
    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
    {
        ModuleData* instanceData = (ModuleData*)Context;

        int vid = 0;
        int pid = 0;
        extractVidPid(EventData->u.DeviceInterface.SymbolicLink, &vid, &pid);

        instanceData->hotplugQueue.post(new HotPlug {vid, pid, Action, &instanceData->hotplugThis});
        break;
    }
    default:
        break;
    }
    return ERROR_SUCCESS;
}

class HotPlugManagerWindows : public HotPlugManager
{
public:
    HotPlugManagerWindows()
    : hcm(nullptr)
    {
        cmNotifyFilter = { 0 };
        cmNotifyFilter.cbSize = sizeof(cmNotifyFilter);
        cmNotifyFilter.Flags = 0;
        cmNotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        cmNotifyFilter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_USB_DEVICE;
    }
    
    int supportedHotplugEvents()
    {
        return HOTPLUG_SUPPORTS_IDS;
    }

    void enableHotplug(const Napi::Env &env, ModuleData *instanceData)
    {
        if (isRunning)
        {
            return;
        }

        isRunning = true;
    
        auto res = CM_Register_Notification(&cmNotifyFilter, (PVOID)instanceData, (PCM_NOTIFY_CALLBACK)&MyCMInterfaceNotification, &hcm);
        if (res != CR_SUCCESS)
        {
            isRunning = false;
            THROW_ERROR("RegisterNotification failed")
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
    CM_NOTIFY_FILTER cmNotifyFilter;
};

std::unique_ptr<HotPlugManager> HotPlugManager::create()
{
    return std::make_unique<HotPlugManagerWindows>();
}

void handleHotplug(HotPlug* info) {
    Napi::ObjectReference* hotplugThis = info->hotplugThis;
    Napi::Env env = hotplugThis->Env();
    Napi::HandleScope scope(env);

    int vid = info->vid;
    int pid = info->pid;
    Napi::Object v8VidPid = Napi::Object::New(env);
    v8VidPid.Set("idVendor", Napi::Number::New(env, vid));
    v8VidPid.Set("idProduct", Napi::Number::New(env, pid));
    CM_NOTIFY_ACTION event = info->event;
    delete info;

    DEBUG_LOG("HandleHotplug %i %i %i", vid, pid, event);

    Napi::String eventName;
    if (CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL == event) {
        DEBUG_LOG("Device arrived");
        eventName = Napi::String::New(env, "attachIds");

    } else if (CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL == event) {
        DEBUG_LOG("Device left");
        eventName = Napi::String::New(env, "detachIds");

    } else {
        DEBUG_LOG("Unhandled hotplug event %d\n", event);
        return;
    }
    
    hotplugThis->Get("emit").As<Napi::Function>().MakeCallback(hotplugThis->Value(), { eventName, v8VidPid });
}
