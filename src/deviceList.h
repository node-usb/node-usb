#ifndef _DEVICE_LIST_H
#define _DEVICE_LIST_H

#include <string>
#include <list>
#include <map>
#include <memory>
#include <mutex>

struct ListResultItem_t
{
public:
	int locationId;
	int vendorId;
	int productId;
	std::string deviceName;
	std::string manufacturer;
	std::string serialNumber;
	int deviceAddress;
	void *data;
};

enum DeviceState_t
{
	DeviceState_Connect,
	DeviceState_Disconnect,
};

class DeviceMap
{
public:
	void addItem(std::string key, std::shared_ptr<ListResultItem_t> item);
	std::shared_ptr<ListResultItem_t> popItem(std::string key);

	std::list<std::shared_ptr<ListResultItem_t> > filterItems(int vid, int pid);

	std::list<std::shared_ptr<ListResultItem_t> > popAll();

private:
	std::map<std::string, std::shared_ptr<ListResultItem_t> > deviceMap;
	std::mutex mapLock;
};

#endif
