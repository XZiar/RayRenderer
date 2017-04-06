#pragma unmanaged

#include "resource.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "../common/BasicUtil.hpp"
#include "../common/ResourceHelper.inl"
#include "../common/DelayLoader.inl"
#include <cstdio>
#include <vector>
#include <tuple>

using std::string;
using std::wstring;
using std::vector;
using std::tuple;
using std::pair;
using common::DelayLoader;
static wstring tmppath;
static vector<tuple<HMODULE, string>> hdlls;
static vector<pair<FILE*, wstring>> hfiles;

void createDLL(const wstring& dllpath, const int32_t dllid)
{
	FILE *fp = nullptr;
	errno_t errno;
	if ((errno = _wfopen_s(&fp, dllpath.c_str(), L"wb")) == 0)
	{
		vector<uint8_t> dlldata;
		common::ResourceHelper::getData(dlldata, L"DLL", dllid);
		fwrite(dlldata.data(), dlldata.size(), 1, fp);
		fclose(fp);
	}
	if ((errno = _wfopen_s(&fp, dllpath.c_str(), L"rb")) == 0)
	{
		auto err = GetLastError();
		const auto ret = DeleteFile(dllpath.c_str());
		err = GetLastError();
		hfiles.push_back({ fp,dllpath });
	}
	else
		throw std::runtime_error("cannot open DLL, errorno:" + std::to_string(errno));
	MoveFileEx(dllpath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

void extractDLL()
{
	{
	#ifdef _DEBUG
		wstring glewFile = tmppath + L"glew32d.dll";
	#else
		wstring glewFile = tmppath + L"glew32.dll";
	#endif
		createDLL(glewFile, IDR_DLL_GLEW);
	}
	{
		wstring mlogFile = tmppath + L"miniLogger.dll";
		createDLL(mlogFile, IDR_DLL_MLOG);
	}
	{
		wstring ogluFile = tmppath + L"OpenGLUtil.dll";
		createDLL(ogluFile, IDR_DLL_OGLU);
	}
	{
		wstring ocluFile = tmppath + L"OpenCLUtil.dll";
		createDLL(ocluFile, IDR_DLL_OCLU);
	}
	{
		wstring fonthelpFile = tmppath + L"FontHelper.dll";
		createDLL(fonthelpFile, IDR_DLL_FONTHELP);
	}
	{
		wstring coreFile = tmppath + L"RenderCore.dll";
		createDLL(coreFile, IDR_DLL_RENDERCORE);
	}
}

void freeDLL()
{
	for (auto it = hdlls.rbegin(); it != hdlls.rend(); it++)
	{
		auto bret = DelayLoader::unload(std::get<1>(*it));
	}
	hdlls.clear();
	for (auto fpair : hfiles)
	{
		fclose(fpair.first);
		auto err = GetLastError();
		const auto ret = DeleteFile(fpair.second.c_str());
		err = GetLastError();
		printf("%d", err);
	}
}

void* delayloaddll(const char *name)
{
	GetLastError();
	std::string tmpname(name);
	const auto dllpath = tmppath + std::wstring(tmpname.begin(), tmpname.end());
	const auto hdll = LoadLibraryEx(dllpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (hdll == NULL)
	{
		const auto err = GetLastError();
		throw std::runtime_error("cannot load DLL, err:" + std::to_string(err));
	}
	hdlls.push_back({ hdll,name });
	return hdll;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		common::ResourceHelper::init(hinstDLL);
		wchar_t tmpdir[MAX_PATH + 1] = { 0 };
		::GetTempPath(MAX_PATH, tmpdir);
		tmppath = tmpdir;
		tmppath += L"RayRenderer/";
		CreateDirectory(tmppath.c_str(), NULL);
		tmppath += std::to_wstring(common::SimpleTimer::getCurTime()) + L"/";
		CreateDirectory(tmppath.c_str(), NULL);
		DelayLoader::onLoadDLL = delayloaddll;
		//tmppath = tmpdir;
		extractDLL();
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
