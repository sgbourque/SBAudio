#include "SBAsioDevice.h"

#include "Windows.h"
#include "malloc.h"

//#include "iasiodrv.h"

#include <iostream>
#include <algorithm>
#include <unordered_map>

#include <emmintrin.h>

static constexpr wchar_t ASIO_PATH[] = L"software\\asio";
static constexpr wchar_t COM_CLSID[] = L"clsid";
static constexpr wchar_t INPROC_SERVER[] = L"InprocServer32";
static constexpr wchar_t ASIODRV_DESC[] = L"description";

// This is an horrible macro...
// At least it is as safe as it can be (can return nullptr if invalid)
// since align will most probably be known at compile time, all of this should be well optimized in the end, cleaning up the mess.
// Note that for _alloca to work, it can't be wrapped in a function so not much other choice to have something that will not corrupt the stack.
//#define SB_ALLOCA_BUFFER_ALIGN( type, size, align )	(align == 0 || (align & (align - 1)) != 0) ? nullptr : reinterpret_cast<type*>((reinterpret_cast<uintptr_t>(_alloca(size + (std::max<size_t>(align, alignof(type)) - 1))) + (std::max<uintptr_t>(align, alignof(type)) - 1)) & ~(std::max<uintptr_t>(align, alignof(type)) - 1))
//#define SB_ALLOCA_ARRAY_ALIGN( type, count, align )	SB_ALLOCA_BUFFER_ALIGN( type, count * sizeof(type), std::max<uintptr_t>(align, alignof(type)) )
//#define SB_ALLOCA_BUFFER( type, size )				SB_ALLOCA_BUFFER_ALIGN( type, size, alignof(type) )
//#define SB_ALLOCA_ARRAY( type, count )				SB_ALLOCA_ARRAY_ALIGN( type, count, alignof(type) )

#ifndef assert
#define assert(X) \
	if (!(X)) \
	{ \
		__debugbreak(); \
	}
#endif // #ifndef assert

static size_t s_coInitializeCount = false;

struct SBASIODriver
{
	IASIO* handle;
	mutable volatile long refcount; // refCount are the most common never-const member
};

//
// CLSID
//
bool operator ==(const SBAsioDevice::CLSID& clsidA, const SBAsioDevice::CLSID& clsidB)
{
	return memcmp(&clsidA, &clsidB, sizeof(clsidA)) == 0;
}
struct CLSIDHaser
{
	size_t operator ()(const SBAsioDevice::CLSID& clsid) const
	{
		static_assert(sizeof(SBAsioDevice::CLSID) == sizeof(__m128i), "");
		return *reinterpret_cast<const size_t*>(&clsid);
	}
};
static std::unordered_map<SBAsioDevice::CLSID, SBASIODriver, CLSIDHaser> s_asioDrivers;

//
// Registry
//
class SBRegistryKey
{
public:
	enum AccessMask : DWORD
	{
		Query = KEY_QUERY_VALUE,
		Read = KEY_READ,
		Enumerate = KEY_ENUMERATE_SUB_KEYS,
	};

	SBRegistryKey(const SBRegistryKey& key) = delete;
	SBRegistryKey(SBRegistryKey&& key)
		: hkey(key)
	{
		key.hkey = nullptr;
	}

	SBRegistryKey(HKEY parentKey, AccessMask access, const wchar_t* keyname = nullptr)
	{
		if (ERROR_SUCCESS != RegOpenKeyExW(parentKey, keyname, 0, access, &hkey))
		{
			hkey = nullptr;
		}
	}
	SBRegistryKey(const SBRegistryKey& parentKey, AccessMask access, const wchar_t* keyname = nullptr)
	{
		if (ERROR_SUCCESS != RegOpenKeyExW(parentKey, keyname, 0, access, &hkey))
		{
			hkey = nullptr;
		}
	}

	~SBRegistryKey()
	{
		if (hkey && (reinterpret_cast<uintptr_t>(hkey) & reinterpret_cast<uintptr_t>(HKEY_CLASSES_ROOT)) == 0)
		{
			RegCloseKey(hkey);
			hkey = nullptr;
		}
	}

	operator bool() const { return hkey; }
	operator HKEY() const { return hkey; }

	template<typename type = std::wstring, DWORD regDataTypeId = REG_SZ>
	type getValue(const wchar_t* keyname = nullptr)
	{
		DWORD dataType = regDataTypeId;
		DWORD maxDataSize = 0;
		RegQueryValueExW(hkey, keyname, nullptr, nullptr, nullptr, &maxDataSize);

		type value;
		if (maxDataSize > 0)
		{
			value.resize((maxDataSize + sizeof(wchar_t) - 1) / sizeof(wchar_t));
			DWORD dataSize = maxDataSize;
			unsigned char* data = reinterpret_cast<unsigned char*>(&value[0]);
			RegQueryValueExW(hkey, keyname, nullptr, &dataType, data, &dataSize);
			assert(dataSize <= maxDataSize);
			assert(dataType != REG_SZ || value[(dataSize + sizeof(wchar_t) - 1) / sizeof(wchar_t) - 1] == L'\0');
			value.resize((dataSize + sizeof(wchar_t) - 1) / sizeof(wchar_t));
		}
		return value;
	}

private:
	HKEY hkey;
};
inline SBRegistryKey::AccessMask operator|(SBRegistryKey::AccessMask a, SBRegistryKey::AccessMask b) { return static_cast<SBRegistryKey::AccessMask>(static_cast<DWORD>(a) | static_cast<DWORD>(b)); }

class SBRegistryKeyIterator
{
public:
	SBRegistryKeyIterator(HKEY key)
		: hkey(key), index(0), maxNameCharCount(0)
	{
		RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, &maxNameCharCount, NULL, NULL, NULL, NULL, NULL, NULL);
		++maxNameCharCount;
	}

	std::wstring operator *() const
	{
		DWORD nameCharCount = maxNameCharCount;
		std::wstring value;
		value.resize(maxNameCharCount);
		if (ERROR_SUCCESS != RegEnumKeyExW(hkey, index, &value[0], &nameCharCount, NULL, NULL, NULL, NULL))
		{
			index = -1;
			nameCharCount = 0;
		}
		value.resize(nameCharCount);
		return value;
	}

	SBRegistryKeyIterator& operator ++()
	{
		if (index >= 0)
			++index;
		return *this;
	}
	bool operator ==(const SBRegistryKeyIterator & other) const
	{
		return hkey == other.hkey && index == other.index;
	}
	bool operator !=(const SBRegistryKeyIterator & other) const
	{
		return hkey != other.hkey || index != other.index;
	}

	static const SBRegistryKeyIterator end(HKEY key)
	{
		return SBRegistryKeyIterator(key, -1, 0);
	}
private:
	explicit SBRegistryKeyIterator(HKEY key, int index, DWORD maxNameCharCount)
		: hkey(key), index(index), maxNameCharCount(maxNameCharCount)
	{
	}

	HKEY hkey;
	DWORD maxNameCharCount = 0;
	mutable int index = 0;
};

SBRegistryKeyIterator begin(const HKEY hkey)
{
	return SBRegistryKeyIterator(hkey);
}
SBRegistryKeyIterator end(const HKEY hkey)
{
	return SBRegistryKeyIterator::end(hkey);
}


std::wstring SB_GetAsioDllPath(const wchar_t deviceCLSID[])
{
	std::wstring path;
	if (auto clsidKey = SBRegistryKey(HKEY_CLASSES_ROOT, SBRegistryKey::Read | SBRegistryKey::Query, COM_CLSID))
	{
		if (auto subKey = SBRegistryKey(clsidKey, SBRegistryKey::Read, deviceCLSID))
		{
			if (auto pathKey = SBRegistryKey(subKey, SBRegistryKey::Query, INPROC_SERVER))
			{
				auto value = pathKey.getValue();
				if (!value.empty())
				{
					HANDLE dll = CreateFileW(value.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (dll != INVALID_HANDLE_VALUE)
					{
						CloseHandle(dll);
						path = value;
					}
				}
			}
		}
	}
	return path;
}

SBAsioDevice SB_ReadASIODevice(HKEY hkey, const wchar_t* keyname)
{
	SBAsioDevice device = {};
	if (auto clsidKey = SBRegistryKey(hkey, SBRegistryKey::Query, keyname))
	{
		auto deviceCLSID = clsidKey.getValue(COM_CLSID);
		if (!deviceCLSID.empty())
		{
			CLSIDFromString(deviceCLSID.c_str(), reinterpret_cast<CLSID*>(&device.classID));
			device.path = SB_GetAsioDllPath(deviceCLSID.c_str());
			if (device)
			{
				auto desc = clsidKey.getValue(ASIODRV_DESC);
				if (!desc.empty())
				{
					device.name = desc;
				}
				else
				{
					device.name = keyname;
				}
			}
		}
	}
	return device;
}


std::vector<SBAsioDevice> SB_EnumerateASIODevices()
{
	std::vector<SBAsioDevice> deviceList;
	deviceList.reserve(16);
	if (auto asioDeviceKey = SBRegistryKey(HKEY_LOCAL_MACHINE, SBRegistryKey::Query | SBRegistryKey::Enumerate, ASIO_PATH))
	{
		for (std::wstring keyName : asioDeviceKey)
		{
			if (!keyName.empty())
			{
				auto device = SB_ReadASIODevice(asioDeviceKey, keyName.c_str());
				if (device)
				{
					deviceList.emplace_back(device);
				}
				else
				{
					std::wcerr << "Could not read ASIO device: " << keyName << std::endl;
				}
			}
		}
	}
	return deviceList;
}

void SB_ASIOInitialize()
{
	// initialize COM
	// TODO: once threaded, try using concurrency mode (COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	s_coInitializeCount++;
}

void SB_ASIOShutdown()
{
	if (--s_coInitializeCount == 0)
	{
		for (auto& it : s_asioDrivers)
		{
			//assert(it.second.refcount == 1);
			it.second.handle->stop();
			it.second.handle->Release();
			it.second.handle = nullptr;
			_InterlockedExchange(&it.second.refcount, 0);
		}
		s_asioDrivers.clear();
	}
	CoUninitialize();
}

SBAsioDriverResult SB_CreateAsioDriver(const SBAsioDevice& device)
{
	if (s_coInitializeCount > 0)
	{
		auto it = s_asioDrivers.find(device.classID);
		if (it != s_asioDrivers.end())
		{
			_InterlockedIncrement(&it->second.refcount);
			return SBAsioDriverResult::AlreadyExists;
		}
		else
		{
			// TODO: Support CoCreateInstanceEx for remote audio rendering?
			CLSID clsid = *reinterpret_cast<const CLSID*>(&device.classID);
			SBASIODriver driver = {};
			long result = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, clsid, reinterpret_cast<void**>(&driver.handle));
			if (S_OK == result)
			{
				_InterlockedIncrement(&driver.refcount);
				s_asioDrivers.insert({device.classID, driver});
				return SBAsioDriverResult::Success;
			}
			else
			{
				return SBAsioDriverResult::Error_Failed;
			}
		}
	}
	else
	{
		return SBAsioDriverResult::Error_Unitialized;
	}
}

SBAsioDriverResult SB_ReleaseAsioDriver(const SBAsioDevice& device)
{
	if (s_coInitializeCount > 0)
	{
		auto it = s_asioDrivers.find(device.classID);
		if (it != s_asioDrivers.end())
		{
			const bool release = _InterlockedDecrement(&it->second.refcount) == 0;
			if (release)
			{
				it->second.handle->Release();
				it->second.handle = nullptr;
				s_asioDrivers.erase(it);
			}
			return release ? SBAsioDriverResult::AlreadyExists : SBAsioDriverResult::Success;
		}
		else
		{
			return SBAsioDriverResult::Error_Failed;
		}
	}
	else
	{
		return SBAsioDriverResult::Error_Unitialized;
	}
}

IASIO* SB_QueryInterface(const SBAsioDevice& device)
{
	IASIO* handle = nullptr;
	auto it = s_asioDrivers.find(device.classID);
	if (it == s_asioDrivers.end())
	{
		SB_CreateAsioDriver(device);
		it = s_asioDrivers.find(device.classID);
	}

	if (it != s_asioDrivers.end())
	{
		handle = it->second.handle;
		handle->AddRef();
	}
	return handle;
}

