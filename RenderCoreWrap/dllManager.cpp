#pragma unmanaged

#include "../common/TimeUtil.hpp"
#include "resource.h"
#include <cstdio>
#include <vector>
#include <tuple>
#include <filesystem>
#include "../common/ResourceHelper.inl"
#include "../common/DelayLoader.inl"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

using std::string;
using std::wstring;
using std::vector;
using std::pair;
namespace fs = std::experimental::filesystem;
using common::DelayLoader;
static fs::path RRPath, DLLSPath;
static vector<pair<HMODULE, string>> hdlls;
static vector<pair<FILE*, wstring>> hfiles;

void createDLL(const wstring& dllname, const int32_t dllid)
{
	const wstring dllpath = DLLSPath / dllname;
	FILE *fp = nullptr;
	errno_t errno;
	if ((errno = _wfopen_s(&fp, dllpath.c_str(), L"wb")) == 0)
	{
		auto dlldata = common::ResourceHelper::getData(L"DLL", dllid);
		fwrite(dlldata.data(), dlldata.size(), 1, fp);
		fclose(fp);
	}
	if ((errno = _wfopen_s(&fp, dllpath.c_str(), L"rb")) == 0)
	{
		DeleteFile(dllpath.c_str());
		MoveFileEx(dllpath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		hfiles.push_back({ fp,dllpath });
	}
	else
		throw std::runtime_error("cannot open DLL, errorno:" + std::to_string(errno));
}

void extractDLL()
{
	createDLL(L"miniLogger.dll", IDR_DLL_MLOG);
	createDLL(L"OpenGLUtil.dll", IDR_DLL_OGLU);
	createDLL(L"OpenCLUtil.dll", IDR_DLL_OCLU);
	createDLL(L"FontHelper.dll", IDR_DLL_FONTHELP);
	createDLL(L"RenderCore.dll", IDR_DLL_RENDERCORE);
}

void freeDLL()
{
	for (const auto& dll : hdlls)
	{
		auto bret = DelayLoader::unload(dll.second);
	}
	hdlls.clear();
	for (const auto& fpair : hfiles)
	{
		fclose(fpair.first);
		DeleteFile(fpair.second.c_str());
	}
	std::error_code errcd;

	for (auto& subdir : fs::directory_iterator(RRPath))
		fs::remove_all(subdir, errcd);
}

void* delayloaddll(const char *name)
{
	GetLastError();
	const std::string tmpname(name);
	const wstring dllpath = DLLSPath / std::wstring(tmpname.begin(), tmpname.end());
	const auto hdll = LoadLibraryEx(dllpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (hdll == NULL)
	{
		const auto err = GetLastError();
		throw std::runtime_error("cannot load DLL, err:" + std::to_string(err));
	}
	hdlls.push_back({ hdll,tmpname });
	return hdll;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		common::ResourceHelper::init(hinstDLL);
		RRPath = fs::temp_directory_path() / L"RayRenderer";
		DLLSPath = RRPath / std::to_wstring(common::SimpleTimer::getCurTime());
		fs::create_directories(DLLSPath);
		DelayLoader::onLoadDLL = delayloaddll;
		extractDLL();
		SetDllDirectory(DLLSPath.c_str());
	}
	break;
	case DLL_PROCESS_DETACH:
		freeDLL();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
