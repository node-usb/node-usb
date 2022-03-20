
#ifndef _USB_DETECTION_H
#define _USB_DETECTION_H

#include <napi.h>
#include <list>
#include <string>
#include <stdio.h>

#include "deviceList.h"

void InitializeDetection(Napi::Env &env, Napi::Object &target);

class Detection
{
public:
	~Detection();

	virtual bool IsRunning() = 0;
	virtual bool Start(const Napi::Env &env, const Napi::Function &callback) = 0;
	virtual void Stop() = 0;

	Napi::Value StartMonitoring(const Napi::CallbackInfo &args);
	void StopMonitoring(const Napi::CallbackInfo &args);
	Napi::Value IsMonitoring(const Napi::CallbackInfo &args);

	Napi::Value FindDevices(const Napi::CallbackInfo &args);

	void DeviceAdded(const std::shared_ptr<ListResultItem_t> item);
	void DeviceRemoved(const std::shared_ptr<ListResultItem_t> item);

protected:
	DeviceMap deviceMap;
	Napi::ThreadSafeFunction notify_func;
};

// void EIO_Find(uv_work_t *req);

Napi::Value DeviceItemToObject(const Napi::Env &env, std::shared_ptr<ListResultItem_t> it);

struct ListBaton
{
public:
	Napi::FunctionReference *callback;
	std::list<ListResultItem_t *> results;
	char errorString[1024];
	int vid;
	int pid;
};

#endif

#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "node-usb-detection [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...)                         \
	DEBUG_HEADER fprintf(stderr, __VA_ARGS__); \
	DEBUG_FOOTER
#else
#define DEBUG_LOG(...)
#endif