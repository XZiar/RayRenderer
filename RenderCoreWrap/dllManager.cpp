#pragma unmanaged

#include "common/TimeUtil.hpp"
#include "common/ResourceHelper.inl"
#include "common/DelayLoader.inl"
#include "3rdParty/fmt/format.h"
#include "resource.h"
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

using std::string;
using std::wstring;
using std::vector;
using std::pair;
namespace fs = std::experimental::filesystem;
using common::DelayLoader;
using common::SimpleTimer;
static fs::path RRPath, DLLSPath;
static vector<pair<HMODULE, string>> hdlls; //dlls need to be unload manually
static vector<pair<FILE*, wstring>> hfiles;
static SimpleTimer timer;


template<class... Args>
static void DebugOutput(const std::wstring formater, Args&&... args)
{
    timer.Stop();
    const std::wstring logdat = fmt::format(L"###[{:>7.2f}]" + formater, timer.ElapseMs() / 1000.f, std::forward<Args>(args)...) + L"\n";
    OutputDebugString(logdat.c_str());
}

static void createDLL(const wstring& dllname, const int32_t dllid)
{
    const wstring dllpath = DLLSPath / dllname;
    FILE *fp = nullptr;
    errno_t errno;
    if ((errno = _wfopen_s(&fp, dllpath.c_str(), L"wb")) == 0)
    {
        const auto dlldata = common::ResourceHelper::getData(L"DLL", dllid);
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
    DebugOutput(L"Excracted DLL [{}]", dllname);

}

static void extractDLL()
{
    createDLL(L"miniLogger.dll", IDR_DLL_MLOG);
    createDLL(L"AsyncExecutor.dll", IDR_DLL_ASYEXE);
    createDLL(L"OpenGLUtil.dll", IDR_DLL_OGLU);
    createDLL(L"OpenCL_ICD_Loader.dll", IDR_DLL_OCL_LODER);
    createDLL(L"OpenCLUtil.dll", IDR_DLL_OCLU);
    createDLL(L"FontHelper.dll", IDR_DLL_FONTHELP);
    createDLL(L"ImageUtil.dll", IDR_DLL_IMGUTIL);
    createDLL(L"RenderCore.dll", IDR_DLL_RENDERCORE);
}

static void freeDLL()
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

static void* delayloaddll(const char *name)
{
    const auto lasterr = GetLastError();
    const string tmpname(name);
    const wstring wname(tmpname.begin(), tmpname.end());
    const auto preHandle = GetModuleHandle(wname.c_str());
    if (preHandle != NULL)
    {
        DebugOutput(L"DLL had been loaded before delay [{}]", wname);
        return preHandle;
    }
    DebugOutput(L"Begin delay load DLL [{}]", wname);
    const wstring dllpath = DLLSPath / wname;
    const auto hdll = LoadLibraryEx(dllpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (hdll == NULL)
    {
        const auto err = GetLastError();
        DebugOutput(L"Fail to load DLL [{}] with error [{:d}]", wname, err);
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
        DebugOutput(L"DLL_PROCESS_ATTACH");
        common::ResourceHelper::init(hinstDLL);
        DebugOutput(L"Res Inited");
        RRPath = fs::temp_directory_path() / L"RayRenderer";
        DLLSPath = RRPath / std::to_wstring(common::SimpleTimer::getCurTime());
        fs::create_directories(DLLSPath);
        DelayLoader::onLoadDLL = delayloaddll;
        DebugOutput(L"Begin Excract DLL");
        extractDLL();
        SetDllDirectory(DLLSPath.c_str());
        DebugOutput(L"Initial Finished");
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
