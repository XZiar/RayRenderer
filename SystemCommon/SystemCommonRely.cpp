#include "SystemCommonPch.h"
#include "Exceptions.h"
#include "Format.h"
#include "ConsoleEx.h"
#include "SpinLock.h"
#include "common/simd/SIMD.hpp"
#include "common/StringLinq.hpp"
#include <boost/version.hpp>

#pragma message("Compiling SystemCommon with [" STRINGIZE(COMMON_SIMD_INTRIN) "]" )
#pragma message("Compiling SystemCommon with boost[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#if COMMON_OS_WIN
#   include <sdkddkver.h>
#   pragma message("Compiling SystemCommon with WinSDK[" STRINGIZE(NTDDI_VERSION) "]" )
#endif
#if COMMON_OS_ANDROID
#   include <android/ndk-version.h>
#   include <android/api-level.h>
#   include <android/log.h>
#   pragma message("Compiling SystemCommon with Android NDK[" STRINGIZE(__NDK_MAJOR__) "]" )
#   pragma message("Compiling SystemCommon with Android Target API[" STRINGIZE(__ANDROID_API__) "]" )
#endif
#if COMMON_OS_DARWIN
#   include "ObjCHelper.h"
#   if COMMON_OS_IOS
#       pragma message("Compiling SystemCommon with IOS SDK[" STRINGIZE(__IPHONE_OS_VERSION_MAX_ALLOWED) "]" )
#   endif
#endif
#if COMMON_OS_UNIX
#   include <sys/utsname.h>
#   ifdef __GLIBC__
#       pragma message("Compiling SystemCommon with GLIBC[" STRINGIZE(__GLIBC__) "." STRINGIZE(__GLIBC_MINOR__) "]" )
#       include <gnu/libc-version.h>
#   endif
#endif

#include <cstdarg>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <forward_list>

namespace common
{
using namespace std::string_view_literals;

#if COMMON_OS_WIN
typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
static uint32_t GetWinBuildImpl()
{
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr ptrGetVersion = reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hMod, "RtlGetVersion"));
        if (ptrGetVersion != nullptr)
        {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (0 == ptrGetVersion(&rovi))
                return rovi.dwBuildNumber;
        }
    }
    return 0;
}
uint32_t GetWinBuildNumber() noexcept
{
    static uint32_t verNum = GetWinBuildImpl();
    return verNum;
}
#endif


#if COMMON_OS_ANDROID
int32_t GetAndroidAPIVersion() noexcept
{
    static int32_t verNum = android_get_device_api_level();
    return verNum;
}
#endif


#if COMMON_OS_DARWIN
struct NSOperatingSystemVersion
{
    NSInteger majorVersion;
    NSInteger minorVersion;
    NSInteger patchVersion;
};
static uint32_t GetDarwinOSVersion_()
{
    objc::Clz NSProcessInfo("NSProcessInfo");
    objc::Instance procInfo = NSProcessInfo.Call<id>("processInfo");
    const auto osVer = procInfo.Call<NSOperatingSystemVersion>("operatingSystemVersion");
    const auto ret = static_cast<uint32_t>(osVer.majorVersion) * 10000 + static_cast<uint32_t>(osVer.minorVersion) * 100 + static_cast<uint32_t>(osVer.patchVersion);
    procInfo.Call<id>("release");
    return ret;
}
uint32_t GetDarwinOSVersion() noexcept
{
    static uint32_t verNum = GetDarwinOSVersion_();
    return verNum;
}
#endif


#if COMMON_OS_UNIX
static const utsname& GetUTSName() noexcept
{
    static const auto name = []()
    {
        struct utsname buffer = {};
        uname(&buffer);
        return buffer;
    }();
    return name;
}
uint32_t GetUnixKernelVersion() noexcept
{
    static const auto ver = []()
    {
        const auto& name = GetUTSName();
        uint32_t ver = 0;
        uint32_t partIdx = 0;
        for (const std::string_view part : str::SplitStream(name.release, '.', false).Take(3))
        {
            uint32_t partver = 0;
            for (const auto ch : part)
            {
                if (ch >= '0' && ch <= '9')
                    partver = partver * 10 + (ch - '0');
                else
                    break;
            }
            if (partIdx++ < 2)
                ver = ver * 100 + (partver % 100);
            else
                ver = ver * 1000 + (partver % 1000);
        }
        return ver;
    }();
    return ver;
}
std::optional<uint32_t> GetGlibcVersion() noexcept
{
#   ifdef __GLIBC__
    static const auto ver = []()
    {
        const auto txt = gnu_get_libc_version();
        uint32_t ver = 0;
        for (const std::string_view part : str::SplitStream(txt, '.', false).Take(2))
        {
            uint32_t partver = 0;
            for (const auto ch : part)
            {
                if (ch >= '0' && ch <= '9')
                    partver = partver * 10 + (ch - '0');
                else
                    break;
            }
            ver = ver * 100 + (partver % 100);
        }
        return ver;
    }();
    return ver;
#   endif
    return {};
}
#endif


template<uint32_t Minor = 100, uint32_t Patch = 100>
forceinline std::array<uint32_t, 3> SplitVer3(uint32_t ver) noexcept
{
    return { ver / (Minor * Patch), (ver % (Minor * Patch)) / Patch, ver % Patch };
}
void PrintSystemVersion() noexcept
{
    str::Formatter<char16_t> fmter;
    std::u16string txt;
#if COMMON_OS_WIN
    fmter.FormatToStatic(txt, FmtString(u"Running on Windows build [{}]\n"), GetWinBuildNumber());
#elif COMMON_OS_ANDROID
    const auto kerVer = SplitVer3<100, 1000>(GetLinuxKernelVersion());
    fmter.FormatToStatic(txt, FmtString(u"Running on Android API [{}], Linux [{}.{}.{}]\n"), GetAndroidAPIVersion(), kerVer[0], kerVer[1], kerVer[2]);
#elif COMMON_OS_DARWIN
    const auto ver = SplitVer3(GetDarwinOSVersion());
    const auto kerVer = SplitVer3<100, 1000>(GetUnixKernelVersion());
    printf(u"Running on %s [%u.%u.%u], Darwin [%u.%u.%u]\n",
# if COMMON_OS_MACOS
        "macOS",
# elif COMMON_OS_IOS
        "iOS",
# else
        "Unknown",
# endif
        ver[0], ver[1], ver[2], kerVer[0], kerVer[1], kerVer[2]);
#elif COMMON_OS_UNIX
#   if COMMON_OS_LINUX
    std::string_view target = "Linux";
#   else
    std::string_view target = "Unix";
#   endif
    const auto kerVer = SplitVer3<100, 1000>(GetUnixKernelVersion());
    fmter.FormatToStatic(txt, FmtString(u"Running on {} [{}.{}.{}]"), target, kerVer[0], kerVer[1], kerVer[2]);
    const auto glibcver = GetGlibcVersion();
    if (glibcver.has_value())
        fmter.FormatToStatic(txt, FmtString(u" glibc [{}.{}]"), (*glibcver) / 100, (*glibcver) % 100);
    txt.push_back(u'\n');
#endif
    console::ConsoleEx::Get().Print(txt);
}


namespace detail
{
#if COMMON_OS_ANDROID
void DebugErrorOutput(std::string_view str) noexcept
{
    __android_log_write(ANDROID_LOG_FATAL, "SystemCommon", str.data());
}
#elif COMMON_OS_WIN
void DebugErrorOutput(std::string_view str) noexcept
{
    OutputDebugStringA(str.data());
}
#else
void DebugErrorOutput(std::string_view str) noexcept
{
    fprintf(stderr, "%s", str.data());
}
#endif


struct InitMessageHost
{
    common::SpinLocker Lock;
    std::unique_ptr<InitMessage::Handler> Handler;
    std::deque<InitMessage> Messages;
    static std::pair<InitMessageHost*, bool*> Get() noexcept
    {
        static thread_local bool beInLock = false;
        static InitMessageHost host;
        return { &host, &beInLock };
    }
    // expects beInLock = true
    void ConsumeAll()
    {
        while (!Messages.empty())
        {
            const auto& tmp = Messages.front();
            Handler->Handle(tmp.Level, tmp.Host, tmp.Message);
            Messages.pop_front();
        }
    }
};

InitMessage::Handler::~Handler() {}

void InitMessage::Enqueue(mlog::LogLevel level, std::string_view host, std::string_view msg) noexcept
{
    const auto [man, beInLock] = InitMessageHost::Get();
    if (!*beInLock)
    {
        const auto locker = man->Lock.LockScope();
        *beInLock = true;
        if (man->Handler)
        {
            if (!man->Messages.empty())
                man->ConsumeAll();
            man->Handler->Handle(level, host, msg);
            man->ConsumeAll();
        }
        else
            man->Messages.push_back(InitMessage{ std::string(host), std::string(msg), level });
        *beInLock = false;
    }
    else
    {
        Ensures(!man->Lock.TryLock());
        man->Messages.push_back(InitMessage{ std::string(host), std::string(msg), level });
    }
}
void InitMessage::Consume(std::unique_ptr<Handler> handler) noexcept
{
    const auto [man, beInLock] = InitMessageHost::Get();
    Ensures(!*beInLock);
    const auto locker = man->Lock.LockScope();
    *beInLock = true;
    Ensures(!man->Handler);
    man->Handler = std::move(handler);
    man->ConsumeAll();
    *beInLock = false;
}

}


#if COMMON_OS_WIN || defined(__STDC_LIB_EXT1__)

std::string GetEnvVar(const char* name) noexcept
{
    std::string ret;
    // use local var to try approach thread-safe by avoiding 2-step query
    constexpr size_t TmpLen = 128;
    char tmp[TmpLen] = { 0 };
    size_t len = 0;
    const auto err = getenv_s(&len, tmp, TmpLen, name);
    switch (err)
    {
    case 0:
        if (len > 1) // only copy when key exists and has content
            ret.assign(tmp, len - 1);
        break;
    case ERANGE:
        ret.resize(len);
        if (getenv_s(&len, ret.data(), ret.size(), name) == 0 && len == 0)
            ret.resize(len - 1); // remove trailing zero
        else
            ret.clear();
        break;
    default:
        break;
    }
    return ret;
}

#else

std::string GetEnvVar(const char* name) noexcept
{
    std::string ret;
    const auto str = getenv(name);
    if (str != nullptr)
        ret.assign(str);
    return ret;
}

#endif


std::u16string GetSystemName() noexcept
{
#if COMMON_OS_WIN
    std::u16string str(MAX_COMPUTERNAME_LENGTH + 1, L'\0');
    while (true)
    {
        DWORD size = static_cast<uint32_t>(str.size());
        const auto ret = GetComputerNameW(reinterpret_cast<wchar_t*>(str.data()), &size);
        if (ret)
        {
            str.resize(size);
            break;
        }
        if (GetLastError() == ERROR_BUFFER_OVERFLOW)
        {
            str.resize(size + 1);
            continue;
        }
        str.clear();
        break;
    }
    return str;
#elif COMMON_OS_LINUX || COMMON_OS_MACOS
    std::string tmp(HOST_NAME_MAX, '\0');
    while (true)
    {
        if (0 == gethostname(tmp.data(), tmp.size()))
        {
            return str::to_u16string(tmp.data(), std::char_traits<char>::length(tmp.data()), str::Encoding::UTF8);
        }
        switch (errno)
        {
        case EINVAL:
        case ENAMETOOLONG:
            tmp.resize(tmp.size() * 2);
            continue;
        default:
            break;
        }
        break;
    }
    return {};
#else
    return {};
#endif
}


static void LogCpuinfoMsg(mlog::LogLevel level, const char* format, va_list args) noexcept
{
    constexpr auto Log = [](mlog::LogLevel level, const char* str, size_t len) 
        {
            if (level > mlog::LogLevel::Error)
            {
                std::string msg = "[cpuinfo]";
                msg.append(str, len);
                detail::DebugErrorOutput(msg);
            }
            detail::InitMessage::Enqueue(level, "cpuinfo", { str, len });
        };
    constexpr uint32_t Len = 1020;
    char tmp[Len + 1] = { '\0' };
    va_list args_copy;
    va_copy(args_copy, args);
    const auto len = vsnprintf(tmp, Len, format, args);
    if (len > 0)
    {
        if (static_cast<uint32_t>(len) > Len)
        {
            std::string tmp2;
            tmp2.resize(len + 1);
            const auto len2 = vsnprintf(tmp2.data(), len, format, args_copy);
            if (len2 > 0)
            {
                tmp2.resize(std::min(len, len2));
                tmp2.push_back('\n');
                Log(level, tmp2.data(), tmp2.size());
            }
        }
        else
        {
            tmp[len] = '\n';
            Log(level, tmp, len + 1);
        }
    }
    va_end(args_copy);
}


struct CleanerData
{
    std::shared_mutex UseLock;
    spinlock::SpinLocker ModifyLock;
    //std::mutex ModifyLock;
    std::forward_list<std::function<void(void)>> Callbacks;
    bool Called = false;
    static CleanerData& Get() noexcept
    {
        static CleanerData data;
        return data;
    }
};

uintptr_t ExitCleaner::RegisterCleaner_(std::function<void(void)> callback) noexcept
{
    auto& data = CleanerData::Get();
    std::unique_lock lock(data.UseLock);
    if (data.Called)
        return 0;
    try
    {
        auto ptr = &data.Callbacks.emplace_front(std::move(callback));
        return reinterpret_cast<uintptr_t>(ptr);
    }
    catch (const std::bad_alloc& err)
    {
        detail::DebugErrorOutput(err.what());
        return 0;
    }
    return true;
}

bool ExitCleaner::UnRegisterCleaner(uintptr_t id) noexcept
{
    auto& data = CleanerData::Get();
    std::shared_lock lock(data.UseLock);
    if (data.Called)
        return false;
    auto lock2 = data.ModifyLock.LockScope();
    //std::unique_lock lock2(data.ModifyLock);
    return data.Callbacks.remove_if([&](const auto& callback) { return reinterpret_cast<uintptr_t>(&callback) == id; }) > 0;
}

ExitCleaner::~ExitCleaner()
{
    auto& data = CleanerData::Get();
    std::shared_lock lock(data.UseLock);
    while (!data.Callbacks.empty())
    {
        data.ModifyLock.Lock();
        //std::unique_lock lock2(data.ModifyLock);
        const auto callback = std::move(data.Callbacks.front());
        data.Callbacks.pop_front();
        data.ModifyLock.Unlock();
        callback();
    }
    data.Called = true;
    return;
}


std::string_view GetColorName(CommonColor color) noexcept
{
    constexpr std::string_view names[16] =
    {
        "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
        "BrightBlack", "BrightRed", "BrightGreen", "BrightYellow", "BrightBlue", "BrightMagenta", "BrightCyan", "BrightWhite"
    };
    if (const auto idx = enum_cast(color); idx < 16) return names[idx];
    return "Unknown"sv;
}

constexpr auto Color256Map = []()
{
    constexpr uint8_t list6[6] = { 0, 95, 135, 175, 215, 255 };
    std::array<std::array<uint8_t, 4>, 256> ret = { {} };
    ret[ 0] = {   0,  0,  0,0 };
    ret[ 1] = { 128,  0,  0,0 };
    ret[ 2] = {   0,128,  0,0 };
    ret[ 3] = { 128,128,  0,0 };
    ret[ 4] = {   0,  0,128,0 };
    ret[ 5] = { 128,  0,128,0 };
    ret[ 6] = {   0,128,128,0 };
    ret[ 7] = { 192,192,192,0 };
    ret[ 8] = { 128,128,128,0 };
    ret[ 9] = { 255,  0,  0,0 };
    ret[10] = {   0,255,  0,0 };
    ret[11] = { 255,255,  0,0 };
    ret[12] = {   0,  0,255,0 };
    ret[13] = { 255,  0,255,0 };
    ret[14] = {   0,255,255,0 };
    ret[15] = { 255,255,255,0 };
    for (uint32_t r = 0, idx = 16; r < 6; ++r)
    {
        for (uint32_t g = 0; g < 6; ++g)
        {
            for (uint32_t b = 0; b < 6; ++b)
            {
                ret[idx++] = { list6[r], list6[g], list6[b], 0 };
            }
        }
    }
    for (uint8_t idx = 232, val = 8; idx < 255; ++idx, val += 10)
    {
        ret[idx] = { val, val, val, 0 };
    }
    return ret;
}();
ScreenColor Expend256ColorToRGB(uint8_t color) noexcept
{
    const auto& newColor = Color256Map[color];
    return { false, newColor[0], newColor[1], newColor[2] };
}


void FastPathBase::Init(common::span<const PathInfo> info, common::span<const VarItem> requests) noexcept
{
    if (requests.empty())
    {
        for (const auto& path : info)
        {
            if (!path.Variants.empty())
            {
                const auto& var = path.Variants.front();
                path.Access(*this) = var.FuncPtr;
                VariantMap.emplace_back(path.FuncName, var.MethodName);
            }
        }
    }
    else
    {
        for (const auto& req : requests)
        {
            for (const auto& path : info)
            {
                if (path.FuncName != req.first) 
                    continue;
                if (auto& ptr = path.Access(*this); ptr == nullptr)
                {
                    for (const auto& var : path.Variants)
                    {
                        if (var.MethodName == req.second)
                        {
                            ptr = var.FuncPtr;
                            VariantMap.emplace_back(path.FuncName, var.MethodName);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
}

void FastPathBase::MergeInto(std::vector<PathInfo>& dst, common::span<const PathInfo> src) noexcept
{
    for (const auto& info : src)
    {
        for (auto& path : dst)
        {
            if (info.FuncName == path.FuncName)
            {
                for (const auto& method : info.Variants)
                {
                    bool found = false;
                    for (const auto& existing : path.Variants)
                    {
                        if (existing.MethodName == method.MethodName)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        path.Variants.emplace_back(method);
                    }
                }
                break;
            }
        }
    }
}

}


extern "C"
{
void cpuinfo_vlog_fatal(const char* format, va_list args)
{
    common::LogCpuinfoMsg(common::mlog::LogLevel::None, format, args);
}
void cpuinfo_vlog_error(const char* format, va_list args)
{
    common::LogCpuinfoMsg(common::mlog::LogLevel::Error, format, args);
}
void cpuinfo_vlog_warning(const char* format, va_list args)
{
    common::LogCpuinfoMsg(common::mlog::LogLevel::Warning, format, args);
}
}
