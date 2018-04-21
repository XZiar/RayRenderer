#pragma unmanaged

#include "common/TimeUtil.hpp"
#include "common/FileEx.hpp"
#include "common/ResourceHelper.inl"
#include "common/DelayLoader.inl"
#include "3rdParty/fmt/format.h"
#include "resource.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Wincrypt.h>

#pragma comment(lib, "Advapi32.lib")

using std::string;
using std::wstring;
using std::vector;
using std::map;
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

static const std::map<int32_t, wstring> DLL_MAP = 
{
    { IDR_DLL_MLOG, L"miniLogger.dll" },
    { IDR_DLL_ASYEXE, L"AsyncExecutor.dll" },
    { IDR_DLL_OGLU, L"OpenGLUtil.dll" },
    { IDR_DLL_OCL_LODER, L"OpenCL_ICD_Loader.dll" },
    { IDR_DLL_OCLU, L"OpenCLUtil.dll" },
    { IDR_DLL_FONTHELP, L"FontHelper.dll" },
    { IDR_DLL_IMGUTIL, L"ImageUtil.dll" },
    { IDR_DLL_TEXUTIL, L"TextureUtil.dll" },
    { IDR_DLL_RENDERCORE, L"RenderCore.dll" },
};

static void extractDLL()
{
    for (const auto& dllpair : DLL_MAP)
        createDLL(dllpair.second, dllpair.first);
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


std::wstring HashSelf()
{
    std::vector<uint8_t> dllsData;
    for (const auto& dllpair : DLL_MAP)
    {
        const auto dlldata = common::ResourceHelper::getData(L"DLL", dllpair.first);
        dllsData.insert(dllsData.end(), dlldata.cbegin(), dlldata.cend());
    }
    
    HCRYPTPROV hProv = NULL;
    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv,NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        DebugOutput(L"CryptAcquireContext failed, {}\n", GetLastError());
        throw std::runtime_error("cannot CryptAcquireContext, errorno:" + std::to_string(GetLastError()));
    }
    HCRYPTHASH hHash = NULL;
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        DebugOutput(L"CryptAcquireContext failed, {}\n", GetLastError());
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("cannot CryptAcquireContext, errorno:" + std::to_string(GetLastError()));
    }
    if (!CryptHashData(hHash, dllsData.data(), (DWORD)dllsData.size(), 0))
    {
        DebugOutput(L"CryptHashData failed, {}\n", GetLastError());
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        throw std::runtime_error("cannot CryptHashData, errorno:" + std::to_string(GetLastError()));
    }
    uint8_t MD5[16];
    DWORD dwHashLen = sizeof(MD5);
    constexpr wchar_t HASH_HEX[] = L"0123456789abcdef";
    const auto ret = CryptGetHashParam(hHash, HP_HASHVAL, MD5, &dwHashLen, 0);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    if (!ret)
    {
        DebugOutput(L"CryptGetHashParam failed, {}\n", GetLastError());
        throw std::runtime_error("cannot CryptGetHashParam, errorno:" + std::to_string(GetLastError()));
    }

    std::wstring hash;
    for (auto u8 : MD5)
    {
        hash.push_back(HASH_HEX[u8 / 16]);
        hash.push_back(HASH_HEX[u8 % 16]);
    }
    return hash;
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
        DLLSPath = RRPath / HashSelf();
        DelayLoader::onLoadDLL = delayloaddll;
        if (!fs::exists(DLLSPath))
        {
            fs::create_directories(DLLSPath);
            DebugOutput(L"Begin Excract DLL");
            extractDLL();
        }
        else
        {
            DebugOutput(L"already exist, skip");
        }
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
