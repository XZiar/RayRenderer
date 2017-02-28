#pragma unmanaged
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "../common/ResourceHelper.inl"
#include "../common/DelayLoader.inl"
#include <cstdio>
#include "resource.h"

void extractDLL()
{
	using std::wstring;
	using std::vector;
	wchar_t tmppath[MAX_PATH + 1] = { 0 };
	::GetTempPath(MAX_PATH, tmppath);
	wstring tmpdir(tmppath);
	vector<uint8_t> dlldata;
	FILE *fp = nullptr;
	{
	#ifdef _DEBUG
		wstring glewFile = tmpdir + L"glew32d.dll";
	#else
		wstring glewFile = tmpdir + L"glew32.dll";
	#endif
		common::ResourceHelper::getData(dlldata, L"DLL", IDR_DLL_GLEW);
		_wfopen_s(&fp, glewFile.c_str(), L"wb");
		if (fp != nullptr)
		{
			fwrite(dlldata.data(), dlldata.size(), 1, fp);
			fclose(fp);
			LoadLibrary(glewFile.c_str());
		}
	}
	{
		wstring ogluFile = tmpdir + L"OpenGLUtil.dll";
		common::ResourceHelper::getData(dlldata, L"DLL", IDR_DLL_OGLU);
		_wfopen_s(&fp, ogluFile.c_str(), L"wb");
		if (fp != nullptr)
		{
			fwrite(dlldata.data(), dlldata.size(), 1, fp);
			fclose(fp);
			LoadLibrary(ogluFile.c_str());
		}
	}
	{
		wstring coreFile = tmpdir + L"RenderCore.dll";
		common::ResourceHelper::getData(dlldata, L"DLL", IDR_DLL_RENDERCORE);
		_wfopen_s(&fp, coreFile.c_str(), L"wb");
		if (fp != nullptr)
		{
			fwrite(dlldata.data(), dlldata.size(), 1, fp);
			fclose(fp);
			LoadLibrary(coreFile.c_str());
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		common::ResourceHelper::init(hinstDLL);
		extractDLL();
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
