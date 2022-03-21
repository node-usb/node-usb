#include "device_list.h"

void DeviceMap::addItem(std::string key, std::shared_ptr<ListResultItem_t> item)
{
	std::lock_guard<std::mutex> guard(mapLock);
	deviceMap.insert(std::pair<std::string, std::shared_ptr<ListResultItem_t> >(key, item));
}

std::shared_ptr<ListResultItem_t> DeviceMap::popItem(std::string key)
{
	std::lock_guard<std::mutex> guard(mapLock);
	auto it = deviceMap.find(key);
	if (it == deviceMap.end())
	{
		return nullptr;
	}
	else
	{
		std::shared_ptr<ListResultItem_t> item = it->second;
		deviceMap.erase(it);
		return item;
	}
}

std::list<std::shared_ptr<ListResultItem_t> > DeviceMap::filterItems(int vid, int pid)
{
	std::list<std::shared_ptr<ListResultItem_t> > list;
	std::lock_guard<std::mutex> guard(mapLock);
	for (auto it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		std::shared_ptr<ListResultItem_t> item = it->second;

		if (
			((vid != 0 && pid != 0) && (vid == item->vendorId && pid == item->productId)) ||
			((vid != 0 && pid == 0) && vid == item->vendorId) ||
			(vid == 0 && pid == 0))
		{
			list.push_back(item);
		}
	}

	return list;
}

std::list<std::shared_ptr<ListResultItem_t> > DeviceMap::popAll()
{
	std::list<std::shared_ptr<ListResultItem_t> > list;
	std::lock_guard<std::mutex> guard(mapLock);

	for (auto it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		std::shared_ptr<ListResultItem_t> item = it->second;
		list.push_back(item);
	}

	deviceMap.clear();

	return list;
}
