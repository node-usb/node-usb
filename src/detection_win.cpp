#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <algorithm>

// Include Windows headers
#include <windows.h>
// Include `CM_DEVCAP_UNIQUEID`
#include <Cfgmgr32.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <Setupapi.h>
#include <dbt.h>

#include "detection.h"
#include "deviceList.h"

using namespace std;

typedef std::basic_string<TCHAR> tstring;

/**********************************
 * Local defines
 **********************************/
#define VID_TAG "VID_"
#define PID_TAG "PID_"

#define LIBRARY_NAME ("setupapi.dll")

#define DllImport __declspec(dllimport)

#define MAX_THREAD_WINDOW_NAME 64

/**********************************
 * Local typedefs
 **********************************/

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

HINSTANCE hinstLib;

typedef BOOL(WINAPI *_SetupDiEnumDeviceInfo)(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData);
typedef HDEVINFO(WINAPI *_SetupDiGetClassDevs)(const GUID *ClassGuid, PCTSTR Enumerator, HWND hwndParent, DWORD Flags);
typedef BOOL(WINAPI *_SetupDiDestroyDeviceInfoList)(HDEVINFO DeviceInfoSet);
typedef BOOL(WINAPI *_SetupDiGetDeviceInstanceId)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PTSTR DeviceInstanceId, DWORD DeviceInstanceIdSize, PDWORD RequiredSize);
typedef BOOL(WINAPI *_SetupDiGetDeviceRegistryProperty)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD Property, PDWORD PropertyRegDataType, PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize);

_SetupDiEnumDeviceInfo DllSetupDiEnumDeviceInfo;
_SetupDiGetClassDevs DllSetupDiGetClassDevs;
_SetupDiDestroyDeviceInfoList DllSetupDiDestroyDeviceInfoList;
_SetupDiGetDeviceInstanceId DllSetupDiGetDeviceInstanceId;
_SetupDiGetDeviceRegistryProperty DllSetupDiGetDeviceRegistryProperty;

/**********************************
 * Local Helper Functions
 **********************************/

std::string Utf8Encode(const std::string &str)
{
	if (str.empty())
	{
		return std::string();
	}

	//System default code page to wide character
	int wstr_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	std::wstring wstr_tmp(wstr_size, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr_tmp[0], wstr_size);

	//Wide character to Utf8
	int str_size = WideCharToMultiByte(CP_UTF8, 0, &wstr_tmp[0], (int)wstr_tmp.size(), NULL, 0, NULL, NULL);
	std::string str_utf8(str_size, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr_tmp[0], (int)wstr_tmp.size(), &str_utf8[0], str_size, NULL, NULL);

	return str_utf8;
}

void NormalizeSlashes(char *buf)
{
	char *c = buf;
	while (*c != '\0')
	{
		if (*c == '/')
			*c = '\\';
		c++;
	}
}

void ToUpper(char *buf)
{
	char *c = buf;
	while (*c != '\0')
	{
		*c = toupper((unsigned char)*c);
		c++;
	}
}

void extractVidPid(char *buf, ListResultItem_t *item)
{
	if (buf == NULL)
	{
		return;
	}

	ToUpper(buf);

	char *string;
	char *temp;
	char *pidStr, *vidStr;
	int vid = 0;
	int pid = 0;

	string = new char[strlen(buf) + 1];
	memcpy(string, buf, strlen(buf) + 1);

	vidStr = strstr(string, VID_TAG);
	pidStr = strstr(string, PID_TAG);

	if (vidStr != NULL)
	{
		temp = (char *)(vidStr + strlen(VID_TAG));
		temp[4] = '\0';
		vid = strtol(temp, NULL, 16);
	}

	if (pidStr != NULL)
	{
		temp = (char *)(pidStr + strlen(PID_TAG));
		temp[4] = '\0';
		pid = strtol(temp, NULL, 16);
	}
	item->vendorId = vid;
	item->productId = pid;

	delete string;
}


bool LoadFunctions()
{

	bool success;

	hinstLib = LoadLibrary(LIBRARY_NAME);

	if (hinstLib != NULL)
	{
		DllSetupDiEnumDeviceInfo = (_SetupDiEnumDeviceInfo)GetProcAddress(hinstLib, "SetupDiEnumDeviceInfo");

		DllSetupDiGetClassDevs = (_SetupDiGetClassDevs)GetProcAddress(hinstLib, "SetupDiGetClassDevsA");

		DllSetupDiDestroyDeviceInfoList = (_SetupDiDestroyDeviceInfoList)GetProcAddress(hinstLib, "SetupDiDestroyDeviceInfoList");

		DllSetupDiGetDeviceInstanceId = (_SetupDiGetDeviceInstanceId)GetProcAddress(hinstLib, "SetupDiGetDeviceInstanceIdA");

		DllSetupDiGetDeviceRegistryProperty = (_SetupDiGetDeviceRegistryProperty)GetProcAddress(hinstLib, "SetupDiGetDeviceRegistryPropertyA");

		success = (DllSetupDiEnumDeviceInfo != NULL &&
				   DllSetupDiGetClassDevs != NULL &&
				   DllSetupDiDestroyDeviceInfoList != NULL &&
				   DllSetupDiGetDeviceInstanceId != NULL &&
				   DllSetupDiGetDeviceRegistryProperty != NULL);
	}
	else
	{
		success = false;
	}

	return success;
}

LRESULT CALLBACK DetectCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**********************************
 * Public Functions
 **********************************/

class WindowsDetection : public Detection, public Napi::ObjectWrap<WindowsDetection>
{
public:
	WindowsDetection(const Napi::CallbackInfo &info)
		: Napi::ObjectWrap<WindowsDetection>(info)
	{
	}

	static void Initialize(Napi::Env &env, Napi::Object &target)
	{
		Napi::Function ctor = DefineClass(env, "Detection",
										  {
											  InstanceMethod("isMonitoring", &Detection::IsMonitoring),
											  InstanceMethod("startMonitoring", &Detection::StartMonitoring),
											  InstanceMethod("stopMonitoring", &Detection::StopMonitoring),
											  InstanceMethod("findDevices", &Detection::FindDevices),
										  });

		target.Set("Detection", ctor);
		// constructor = Napi::Persistent(ctor);
		// constructor.SuppressDestruct();
	}

	bool IsRunning()
	{
		return isRunning;
	}
	bool Start(const Napi::Env &env, const Napi::Function &callback)
	{
		if (isRunning)
		{
			return true;
		}

		isRunning = true;
		
		if (!LoadFunctions())
		{
			printf("Could not load library functions from dll -> abort (Check if %s is available)\r\n", LIBRARY_NAME);
			return false;
		}

		// Do initial scan
		BuildInitialDeviceList();

		notify_func = Napi::ThreadSafeFunction::New(
			env,
			callback,		 // JavaScript function called asynchronously
			"Resource Name", // Name
			0,				 // Unlimited queue
			1,				 // Only one thread will use this initially
			[=](Napi::Env) { // Finalizer used to clean threads up
				// Wait for end of the thread, if it wasnt the one to close up
				if (poll_thread.joinable())
				{
					poll_thread.join();
				}
			});

		poll_thread = std::thread([=]() {
			// We have this check in case we `Stop` before this thread starts,
			// otherwise the process will hang
			if (!isRunning)
			{
				return;
			}

			char className[MAX_THREAD_WINDOW_NAME];
			_snprintf_s(className, MAX_THREAD_WINDOW_NAME, "ListnerThreadUsbDetection_%d", GetCurrentThreadId());

			WNDCLASSA wincl = {0};
			wincl.hInstance = GetModuleHandle(0);
			wincl.lpszClassName = className;
			wincl.lpfnWndProc = DetectCallback;
			
			DEV_BROADCAST_DEVICEINTERFACE_A notifyFilter = {0};
			notifyFilter.dbcc_size = sizeof(notifyFilter);
			notifyFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
			notifyFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
			
			if (!RegisterClassA(&wincl))
			{
				DWORD le = GetLastError();
				printf("RegisterClassA() failed [Error: %x]\r\n", le);
				return;
			}

			hwnd = CreateWindowExA(WS_EX_TOPMOST, className, className, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
			if (!hwnd)
			{
				DWORD le = GetLastError();
				printf("CreateWindowExA() failed [Error: %x]\r\n", le);
				goto cleanup;
			}
			
			// Store a reference to this for the callback to use. Its a raw pointer, so will not extend the lifetime
            SetWindowLongPtrA( hwnd, GWLP_USERDATA, (LONG_PTR )this );

			hDevNotify = RegisterDeviceNotificationA(hwnd, &notifyFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
			if (!hDevNotify)
			{
				DWORD le = GetLastError();
				printf("RegisterDeviceNotificationA() failed [Error: %x]\r\n", le);
				goto cleanup;
			}

			MSG msg;
			while (TRUE)
			{
				BOOL bRet = GetMessage(&msg, hwnd, 0, 0);
				if ((bRet == 0) || (bRet == -1))
				{
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// cleanup the resources
			cleanup:

			if (hwnd) {
				if (!DestroyWindow(hwnd)) {
					DWORD le = GetLastError();
					printf("DestroyWindow() failed [Error: %x]\r\n", le);
				}
				hwnd = nullptr;
			}
			
			if (!UnregisterClassA(wincl.lpszClassName, wincl.hInstance)) {
				DWORD le = GetLastError();
				printf("UnregisterClassA() failed [Error: %x]\r\n", le);
			}


			notify_func.Release();
		});

		return true;
	}
	void Stop()
	{
		if (isRunning)
		{
			isRunning = false;

			if (hwnd) {
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}

			poll_thread.join();
		}
	}

	
	void UpdateDevice(PDEV_BROADCAST_DEVICEINTERFACE pDevInf, WPARAM wParam, DeviceState_t state)
	{
		// dbcc_name:
		// \\?\USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
		// convert to
		// USB\Vid_04e8&Pid_503b\0002F9A9828E0F06
		tstring szDevId = pDevInf->dbcc_name + 4;
		auto idx = szDevId.rfind(_T('#'));

		if (idx != tstring::npos)
			szDevId.resize(idx);
		std::replace(begin(szDevId), end(szDevId), _T('#'), _T('\\'));
		auto to_upper = [](TCHAR ch) { return std::use_facet<std::ctype<TCHAR>>(std::locale()).toupper(ch); };
		transform(begin(szDevId), end(szDevId), begin(szDevId), to_upper);

		tstring szClass;
		idx = szDevId.find(_T('\\'));
		if (idx != tstring::npos)
			szClass = szDevId.substr(0, idx);
		// if we are adding device, we only need present devices
		// otherwise, we need all devices
		DWORD dwFlag = DBT_DEVICEARRIVAL != wParam ? DIGCF_ALLCLASSES : (DIGCF_ALLCLASSES | DIGCF_PRESENT);
		HDEVINFO hDevInfo = DllSetupDiGetClassDevs(NULL, szClass.c_str(), NULL, dwFlag);
		if (INVALID_HANDLE_VALUE == hDevInfo)
		{
			return;
		}

		SP_DEVINFO_DATA *pspDevInfoData = (SP_DEVINFO_DATA *)HeapAlloc(GetProcessHeap(), 0, sizeof(SP_DEVINFO_DATA));
		if (pspDevInfoData)
		{
			pspDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
			for (int i = 0; DllSetupDiEnumDeviceInfo(hDevInfo, i, pspDevInfoData); i++)
			{
				DWORD nSize = 0;
				TCHAR buf[MAX_PATH];

				if (!DllSetupDiGetDeviceInstanceId(hDevInfo, pspDevInfoData, buf, sizeof(buf), &nSize))
				{
					break;
				}
				NormalizeSlashes(buf);

				if (szDevId == buf)
				{
					DWORD DataT;
					DWORD nSize;
					DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_LOCATION_INFORMATION, &DataT, (PBYTE)buf, MAX_PATH, &nSize);
					DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_HARDWAREID, &DataT, (PBYTE)(buf + nSize - 1), MAX_PATH - nSize, &nSize);

					if (state == DeviceState_Connect)
					{
						std::string key = buf;
						std::shared_ptr<ListResultItem_t> item = GetProperties(hDevInfo, pspDevInfoData, buf, MAX_PATH);

						deviceMap.addItem(key, item);

						DeviceAdded(item);
					}
					else
					{
						std::shared_ptr<ListResultItem_t> item = deviceMap.popItem(buf);
						if (item == nullptr)
						{
							item = GetProperties(hDevInfo, pspDevInfoData, buf, MAX_PATH);
						}

						DeviceRemoved(item);
					}

					break;
				}
			}

			HeapFree(GetProcessHeap(), 0, pspDevInfoData);
		}

		if (hDevInfo)
		{
			DllSetupDiDestroyDeviceInfoList(hDevInfo);
		}
	}

private:
	/**********************************
 	 * Local Functions
	 **********************************/
	std::shared_ptr<ListResultItem_t> GetProperties(HDEVINFO hDevInfo, SP_DEVINFO_DATA *pspDevInfoData, TCHAR *buf, DWORD buffSize)
	{
		std::shared_ptr<ListResultItem_t> item(new ListResultItem_t);

		DWORD DataT;
		DWORD nSize;
		static int dummy = 1;

		item->locationId = 0;
		item->deviceAddress = dummy++;

		// device found
		if (DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buf, buffSize, &nSize))
		{
			item->deviceName = Utf8Encode(buf);
		}
		else if (DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_DEVICEDESC, &DataT, (PBYTE)buf, buffSize, &nSize))
		{
			item->deviceName = Utf8Encode(buf);
		}
		if (DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_MFG, &DataT, (PBYTE)buf, buffSize, &nSize))
		{
			item->manufacturer = Utf8Encode(buf);
		}
		if (DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_HARDWAREID, &DataT, (PBYTE)buf, buffSize, &nSize))
		{
			// Use this to extract VID / PID
			extractVidPid(buf, item.get());
		}

		// Extract Serial Number
		//
		// Format: <device-ID>\<instance-specific-ID>
		//
		// Ex. `USB\VID_2109&PID_8110\5&376ABA2D&0&21`
		//  - `<device-ID>`: `USB\VID_2109&PID_8110`
		//  - `<instance-specific-ID>`: `5&376ABA2D&0&21`
		//
		// [Device instance IDs](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-instance-ids) ->
		//  - [Device IDs](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-ids) -> [Hardware IDs](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/hardware-ids) -> [Device identifier formats](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-identifier-formats) -> [Identifiers for USB devices](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-usb-devices)
		//     - [Standard USB Identifiers](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/standard-usb-identifiers)
		//     - [Special USB Identifiers](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/special-usb-identifiers)
		//  - [Instance specific ID](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/instance-ids)
		DWORD dwCapabilities = 0x0;
		if (DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_CAPABILITIES, &DataT, (PBYTE)&dwCapabilities, sizeof(dwCapabilities), &nSize))
		{
			if ((dwCapabilities & CM_DEVCAP_UNIQUEID) == CM_DEVCAP_UNIQUEID)
			{
				if (DllSetupDiGetDeviceInstanceId(hDevInfo, pspDevInfoData, buf, buffSize, &nSize))
				{
					string deviceInstanceId = buf;
					size_t serialNumberIndex = deviceInstanceId.find_last_of("\\");
					if (serialNumberIndex != string::npos)
					{
						item->serialNumber = deviceInstanceId.substr(serialNumberIndex + 1);
					}
				}
			}
		}

		return item;
	}

	void BuildInitialDeviceList()
	{
		DWORD dwFlag = (DIGCF_ALLCLASSES | DIGCF_PRESENT);
		HDEVINFO hDevInfo = DllSetupDiGetClassDevs(NULL, "USB", NULL, dwFlag);

		if (INVALID_HANDLE_VALUE == hDevInfo)
		{
			return;
		}

		SP_DEVINFO_DATA *pspDevInfoData = (SP_DEVINFO_DATA *)HeapAlloc(GetProcessHeap(), 0, sizeof(SP_DEVINFO_DATA));
		if (pspDevInfoData)
		{
			pspDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
			for (int i = 0; DllSetupDiEnumDeviceInfo(hDevInfo, i, pspDevInfoData); i++)
			{
				DWORD nSize = 0;
				TCHAR buf[MAX_PATH];

				if (!DllSetupDiGetDeviceInstanceId(hDevInfo, pspDevInfoData, buf, sizeof(buf), &nSize))
				{
					break;
				}
				NormalizeSlashes(buf);

				DWORD DataT;
				DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_LOCATION_INFORMATION, &DataT, (PBYTE)buf, MAX_PATH, &nSize);
				DllSetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, SPDRP_HARDWAREID, &DataT, (PBYTE)(buf + nSize - 1), MAX_PATH - nSize, &nSize);

				std::string key = buf;
				auto item = GetProperties(hDevInfo, pspDevInfoData, buf, MAX_PATH);
				deviceMap.addItem(key, item);
			}

			HeapFree(GetProcessHeap(), 0, pspDevInfoData);
		}

		if (hDevInfo)
		{
			DllSetupDiDestroyDeviceInfoList(hDevInfo);
		}
	}

	/**********************************
	 * Local Variables
	 **********************************/
	std::thread poll_thread;

	std::atomic<bool> isRunning = {false};

public:
	HWND hwnd;
	HDEVNOTIFY hDevNotify;
};

void InitializeDetection(Napi::Env &env, Napi::Object &target)
{
	WindowsDetection::Initialize(env, target);
}

/**
 * Callback when windows has a change to report
 * This is a C function, with access to the class handle via GWLP_USERDATA
 */
LRESULT CALLBACK DetectCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowsDetection * parent = (WindowsDetection *)GetWindowLongPtrA( hwnd, GWLP_USERDATA );
	if (parent != nullptr) {
		if (msg == WM_DEVICECHANGE)
		{
			// Notifier reporting on a device
			if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam)
			{
				PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
				PDEV_BROADCAST_DEVICEINTERFACE pDevInf;

				if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
					parent->UpdateDevice(pDevInf, wParam, (DBT_DEVICEARRIVAL == wParam) ? DeviceState_Connect : DeviceState_Disconnect);
				}
			}
		} else if (msg == WM_CLOSE) {
			// Notifier was closed by the os

			if (!UnregisterDeviceNotification(parent->hDevNotify))
			{
				DWORD le = GetLastError();
				printf("UnregisterDeviceNotification() failed [Error: %x]\r\n", le);
			}
			if (!DestroyWindow(parent->hwnd)) {
				DWORD le = GetLastError();
				printf("DestroyWindow() failed [Error: %x]\r\n", le);
			}
			parent->hwnd = nullptr;
		} else if (msg == WM_DESTROY) {
        	PostQuitMessage(0);
		}
	}

	return 1;
}
