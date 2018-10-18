#pragma unmanaged

#include "common/TimeUtil.hpp"
#include "common/FileEx.hpp"
#include "common/ResourceHelper.inl"
#include "common/DelayLoader.inl"
#include "3rdParty/fmt/format.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "3rdParty/cryptopp/md5.h"
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
    { IDR_DLL_RESPACKER, L"ResourcePackager.dll" },
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
    const string dllname(name);
    const wstring wname(dllname.cbegin(), dllname.cend());
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
    hdlls.push_back({ hdll,dllname });
    return hdll;
}


std::string HashSelf()
{
    std::vector<uint8_t> dllsData;
    for (const auto& dllpair : DLL_MAP)
    {
        const auto dlldata = common::ResourceHelper::getData(L"DLL", dllpair.first);
        dllsData.insert(dllsData.end(), dlldata.cbegin(), dlldata.cend());
    }
  
    static_assert(CryptoPP::Weak::MD5::DIGESTSIZE == 16, "MD5 should generate 16 bytes digest");
    std::array<uint8_t, 16> MD5;
    CryptoPP::Weak::MD5 md5er;
    md5er.CalculateDigest(reinterpret_cast<CryptoPP::byte*>(MD5.data()), reinterpret_cast<const CryptoPP::byte*>(dllsData.data()), dllsData.size());

    constexpr char HASH_HEX[] = "0123456789abcdef";
    std::string hash;
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
        RRPath = fs::temp_directory_path() / L"RayRenderer" / L"DLLs";
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
