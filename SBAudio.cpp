#include "SBAsioDevice.h"

#define SB_WIDEN_INTERNAL(X) 	L##X
#define SB_WIDEN(X)          	SB_WIDEN_INTERNAL(X)


#define SB_APPLICATION_NAME  	"SBAudio"
#define SB_W_APPLICATION_NAME	SB_WIDEN(SB_APPLICATION_NAME)

#define SB_USER_PATH		"User"
#define SB_DATA_PATH		"Data"
#define SB_SHARED_PATH		"Shared"
#define SB_DOCUMENTS_PATH	"Documents"

#define SB_WARNING(...)		

using SB_PLATFORM_HANDLE = void*;


#include <string>
#include <unordered_map>
#include <iostream>

struct SBApplicationContext
{
	size_t
		version = 1;
	SB_PLATFORM_HANDLE
		handle = nullptr;
	std::unordered_map<std::string, std::wstring>
		folders = {};

	operator bool() const { return version > 0 && handle != nullptr && !folders.empty(); }
};

struct SBSetup
{
	size_t
		version = 0;
	std::unordered_map<std::string, std::string>
		settings = {};

	operator bool() const { return version > 0 && !settings.empty(); }
};

#include <tuple>
#include "Shlobj.h"
bool SB_InitializeSystemPath(SBApplicationContext& context)
{
	using callback_t = bool (*)(const std::wstring&);
	callback_t createDirCallback = [](const std::wstring& path) -> bool
	{
		BOOL success = CreateDirectoryW(path.c_str(), NULL);
		return success || GetLastError() == ERROR_ALREADY_EXISTS;
	};
	callback_t verifyDirCallback = [](const std::wstring & path) -> bool
	{
		HANDLE dir = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (dir != INVALID_HANDLE_VALUE)
		{
			CloseHandle(dir);
			return true;
		}
		else
		{
			return false;
		}
	};

	std::tuple<std::string, GUID, callback_t> knownFolders[] = {
		{ SB_USER_PATH, FOLDERID_LocalAppData, createDirCallback },  	// %LOCALAPPDATA% (%USERPROFILE%\AppData\Local)
		{ SB_DATA_PATH, FOLDERID_ProgramData, verifyDirCallback },   	// %ALLUSERSPROFILE% (%ProgramData%, %SystemDrive%\ProgramData)
		{ SB_SHARED_PATH, FOLDERID_Public, verifyDirCallback },        	// %PUBLIC% (%SystemDrive%\Users\Public)
		{ SB_DOCUMENTS_PATH, FOLDERID_Documents, verifyDirCallback },	// %USERPROFILE%\Documents
	};

	for (const auto& knownFolder : knownFolders)
	{
		const auto folderName = std::get<0>(knownFolder);
		const auto folderGUID = std::get<1>(knownFolder);
		const auto callback   = std::get<2>(knownFolder);

		PWSTR	path = nullptr;
		if (SHGetKnownFolderPath(folderGUID, 0, NULL, &path) == S_OK)
		{
			std::wstring applicationPath = std::wstring(path) + std::wstring(L"\\" SB_W_APPLICATION_NAME);
			CoTaskMemFree(path);
			path = nullptr;

			if (callback && callback(applicationPath))
			{
				context.folders.insert({ folderName, applicationPath });
			}
		}
	}
	return context;
}

bool SB_InitializeSystemAudio(SBApplicationContext& context)
{
	SB_ASIOInitialize();
	std::wcout << "Initializing audio";
	auto list = SB_EnumerateASIODevices();
	for (const auto& it : list)
	{
		SB_CreateAsioDriver(it);
		auto handle = SB_QueryInterface(it);
		handle->init(GetCurrentProcess());
		ASIOError started = handle->start();
		std::wcout << "\n\t" << it.name << ": " << SB_GetASIOErrorString(started);
		handle->Release();
	}
	std::wcout << std::endl;
	return context;
}

void SB_ShutdownApplicationContext(const SBApplicationContext& context)
{
	std::wcout << "Shutdown audio";
	auto list = SB_EnumerateASIODevices();
	for (const auto& it : list)
	{
		auto handle = SB_QueryInterface(it);
		ASIOError stopped = handle->stop();
		std::wcout << "\n\t" << it.name << ": " << SB_GetASIOErrorString(stopped);
		handle->Release();
	}
	std::wcout << std::endl;
	SB_ASIOShutdown();
}


SBApplicationContext SB_InitializeApplicationContext()
{
	SBApplicationContext context = {};
	context.handle = GetCurrentProcess();
	SB_InitializeSystemPath(context);
	// Network
	// Display
	// Input
	// Audio
	SB_InitializeSystemAudio(context);
	return context;
}

/*
//#include "Windows.h"
#include <fstream>
SBSetup SB_ReadConfiguration(const SBApplicationContext& context)
{
	SBSetup setup;
	std::ifstream configFile(config.configFilePath.c_str());
	while (configFile)
	{
		std::string key;
		std::getline(configFile, key);
		std::string value;
		std::getline(configFile, value);
		if (!key.empty())
		{
			config.settings.insert({ key, value });
		}
		else
		{
			SB_WARNING("Invalid key found in config file.");
		}
	}
	return setup;
}
*/

int main(int argc, char* argv[])
{
	// 1. Initialize system / get configuration
	//	-	Get system configuration file;
	//	-	Get user configuration file;
	//	-	Initialize worker threads;
	const SBApplicationContext context = SB_InitializeApplicationContext();

	SB_ShutdownApplicationContext(context);
}
