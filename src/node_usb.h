#ifndef SRC_NODE_USB_H
#define SRC_NODE_USB_H

#include <assert.h>
#include <string>
#include <map>

#ifdef _WIN32
#include <WinSock2.h>
#endif

#include <thread>
#include <atomic>
#include <libusb.h>
#include <napi.h>

#include "helpers.h"
#include "uv_async_queue.h"

struct Transfer;

struct HotPlug;
class HotPlugManager;

Napi::Error libusbException(Napi::Env env, int errorno);
void handleCompletion(Transfer* self);

struct Device: public Napi::ObjectWrap<Device> {
    Napi::Env env;
    libusb_device* device;
    libusb_device_handle* device_handle;

    int refs_;
    UVQueue<Transfer*> completionQueue;

    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object get(Napi::Env env, libusb_device* handle);

    inline void ref(){ refs_ = Ref();}
    inline void unref(){ refs_ = Unref();}
    inline bool canClose(){return refs_ == 0;}

    Device(const Napi::CallbackInfo& info);
    ~Device();

    static Napi::Object cdesc2V8(Napi::Env env, libusb_config_descriptor * cdesc);

    Napi::Value GetConfigDescriptor(const Napi::CallbackInfo& info);
    Napi::Value GetAllConfigDescriptors(const Napi::CallbackInfo& info);
    Napi::Value SetConfiguration(const Napi::CallbackInfo& info);

    Napi::Value GetParent(const Napi::CallbackInfo& info);
    Napi::Value Open(const Napi::CallbackInfo& info);
    Napi::Value Reset(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);

    Napi::Value IsKernelDriverActive(const Napi::CallbackInfo& info);
    Napi::Value DetachKernelDriver(const Napi::CallbackInfo& info);
    Napi::Value AttachKernelDriver(const Napi::CallbackInfo& info);
    Napi::Value SetAutoDetachKernelDriver(const Napi::CallbackInfo& info);

    Napi::Value ClaimInterface(const Napi::CallbackInfo& info);
    Napi::Value SetInterface(const Napi::CallbackInfo& info);
    Napi::Value ReleaseInterface(const Napi::CallbackInfo& info);

    Napi::Value ClearHalt(const Napi::CallbackInfo& info);
protected:
    Napi::Value Constructor(const Napi::CallbackInfo& info);
};

struct ModuleData {
    libusb_context* usb_context;
    std::thread usb_thread;
    std::atomic<bool> handlingEvents;

    bool hotplugEnabled = 0;
    std::unique_ptr<HotPlugManager> hotplugManager;
    UVQueue<HotPlug*> hotplugQueue;
    Napi::ObjectReference hotplugThis;
    std::map<libusb_device*, Device*> byPtr;
    Napi::FunctionReference deviceConstructor;

    ModuleData(libusb_context* usb_context);
    ~ModuleData();
};

struct Transfer: public Napi::ObjectWrap<Transfer> {
    libusb_transfer* transfer;
    Device* device;
    Napi::ObjectReference v8buffer;
    Napi::FunctionReference v8callback;

    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    inline void ref(){Ref();}
    inline void unref(){Unref();}

    Transfer(const Napi::CallbackInfo& info);
    ~Transfer();

    Napi::Value Submit(const Napi::CallbackInfo& info);
    Napi::Value Cancel(const Napi::CallbackInfo& info);
private:
    Napi::Value Constructor(const Napi::CallbackInfo& info);
};



#define CHECK_USB(r) \
    if (r < LIBUSB_SUCCESS) { \
        throw libusbException(env, r); \
    }

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
    Napi::Function callback; \
    if (info.Length() > (CALLBACK_ARG_IDX)) { \
        if (!info[CALLBACK_ARG_IDX].IsFunction()) { \
            throw Napi::TypeError::New(env, "Argument " #CALLBACK_ARG_IDX " must be a function"); \
        } \
        callback = info[CALLBACK_ARG_IDX].As<Napi::Function>(); \
    } \

#ifdef DEBUG
  #define DEBUG_HEADER fprintf(stderr, "node-usb [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
  #define DEBUG_LOG(...)
#endif

#endif
