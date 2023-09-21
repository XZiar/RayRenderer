#include "SystemCommonPch.h"
#include "Exceptions.h"
#include "SpinLock.h"
#include "common/FrozenDenseSet.hpp"
#include "common/simd/SIMD.hpp"
#include "common/StringLinq.hpp"
#include "3rdParty/cpuinfo/include/cpuinfo.h"
#if COMMON_ARCH_X86
#   include "3rdParty/cpuinfo/src/x86/cpuid.h"
#endif
#if COMMON_OS_LINUX && COMMON_ARCH_ARM
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
#endif
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
#endif
#if COMMON_OS_DARWIN
#   include "ObjCHelper.h"
#   if COMMON_OS_IOS
#       pragma message("Compiling SystemCommon with IOS SDK[" STRINGIZE(__IPHONE_OS_VERSION_MAX_ALLOWED) "]" )
#   endif
#endif
#if COMMON_OS_UNIX
#   include <sys/utsname.h>
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
#endif


template<uint32_t Minor = 100, uint32_t Patch = 100>
forceinline std::array<uint32_t, 3> SplitVer3(uint32_t ver) noexcept
{
    return { ver / (Minor * Patch), (ver % (Minor * Patch)) / Patch, ver % Patch };
}
void PrintSystemVersion() noexcept
{
#if COMMON_OS_WIN
    printf("Running on Windows build [%u]\n", GetWinBuildNumber());
#elif COMMON_OS_ANDROID
    const auto kerVer = SplitVer3<100, 1000>(GetLinuxKernelVersion());
    printf("Running on Android API [%d], Linux [%u.%u.%u]\n", GetAndroidAPIVersion(), kerVer[0], kerVer[1], kerVer[2]);
#elif COMMON_OS_DARWIN
    const auto ver = SplitVer3(GetDarwinOSVersion());
    const auto kerVer = SplitVer3<100, 1000>(GetUnixKernelVersion());
    printf("Running on %s [%u.%u.%u], Darwin [%u.%u.%u]\n",
# if COMMON_OS_MACOS
        "macOS",
# elif COMMON_OS_IOS
        "iOS",
# else
        "Unknown",
# endif
        ver[0], ver[1], ver[2], kerVer[0], kerVer[1], kerVer[2]);
#elif COMMON_OS_LINUX
    const auto kerVer = SplitVer3<100, 1000>(GetLinuxKernelVersion());
    printf("Running on Linux [%u.%u.%u]\n", kerVer[0], kerVer[1], kerVer[2]);
#elif COMMON_OS_UNIX
    const auto kerVer = SplitVer3<100, 1000>(GetUnixKernelVersion());
    printf("Running on Unix [%u.%u.%u]\n", kerVer[0], kerVer[1], kerVer[2]);
#endif
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
        std::string_view str;
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


struct CPUFeature
{
    std::vector<std::string_view> FeatureText;
    container::FrozenDenseStringSetSimple<char> FeatureLookup;
    void AppendFeature(std::string_view txt) noexcept
    {
        for (const auto& feat : FeatureText)
        {
            if (feat == txt) return;
        }
        FeatureText.push_back(txt);
    }
    void TryCPUInfo() noexcept
    {
        cpuinfo_initialize();
        detail::InitMessage::Enqueue(mlog::LogLevel::Verbose, "cpuinfo", std::string("detected cpu: ") + cpuinfo_get_package(0)->name + "\n");
#if COMMON_ARCH_X86
        const uint32_t max_base_index = cpuid(0).eax;
        if (max_base_index >= 7)
        {
            const auto reg70 = cpuidex(7, 0);
            if (reg70.ecx & (1u << 5))
                AppendFeature("waitpkg"sv);
        }
# define CHECK_FEATURE(en, name) if (cpuinfo_has_x86_##en()) AppendFeature(#name""sv)
        CHECK_FEATURE(sse,          sse);
        CHECK_FEATURE(sse2,         sse2);
        CHECK_FEATURE(sse3,         sse3);
        CHECK_FEATURE(ssse3,        ssse3);
        CHECK_FEATURE(sse4_1,       sse4_1);
        CHECK_FEATURE(sse4_2,       sse4_2);
        CHECK_FEATURE(avx,          avx);
        CHECK_FEATURE(fma3,         fma);
        CHECK_FEATURE(avx2,         avx2);
        CHECK_FEATURE(avx512f,      avx512f);
        CHECK_FEATURE(avx512dq,     avx512dq);
        CHECK_FEATURE(avx512pf,     avx512pf);
        CHECK_FEATURE(avx512er,     avx512er);
        CHECK_FEATURE(avx512cd,     avx512cd);
        CHECK_FEATURE(avx512bw,     avx512bw);
        CHECK_FEATURE(avx512vl,     avx512vl);
        CHECK_FEATURE(avx512vnni,   avx512vnni);
        CHECK_FEATURE(avx512vbmi,   avx512vbmi);
        CHECK_FEATURE(avx512vbmi2,  avx512vbmi2);
        CHECK_FEATURE(pclmulqdq,    pclmul);
        CHECK_FEATURE(popcnt,       popcnt);
        CHECK_FEATURE(aes,          aes);
        CHECK_FEATURE(lzcnt,        lzcnt);
        CHECK_FEATURE(f16c,         f16c);
        CHECK_FEATURE(bmi,          bmi1);
        CHECK_FEATURE(bmi2,         bmi2);
        CHECK_FEATURE(sha,          sha);
        CHECK_FEATURE(adx,          adx);
# undef CHECK_FEATURE
#elif COMMON_ARCH_ARM
        cpuinfo_has_arm_sha2();
# define CHECK_FEATURE(en, name) if (cpuinfo_has_arm_##en()) AppendFeature(#name""sv)
        CHECK_FEATURE(neon,         asimd);
        CHECK_FEATURE(aes,          aes);
        CHECK_FEATURE(pmull,        pmull);
        CHECK_FEATURE(sha1,         sha1);
        CHECK_FEATURE(sha2,         sha2);
        CHECK_FEATURE(crc32,        crc32);
# undef CHECK_FEATURE
#endif
        return;
    }
    void TryAUXVal() noexcept
    {
#if COMMON_OS_LINUX && COMMON_ARCH_ARM && (!COMMON_OS_ANDROID || __NDK_MAJOR__ >= 18)
        [[maybe_unused]] const auto cap1 = getauxval(AT_HWCAP), cap2 = getauxval(AT_HWCAP2);
        //printf("cap1&2: [%lx][%lx]\n", cap1, cap2);
# define PFX1(en) PPCAT(HWCAP_,  en)
# define PFX2(en) PPCAT(HWCAP2_, en)
# define CHECK_FEATURE(n, en, name) if (PPCAT(cap, n) & PPCAT(PFX, n)(en)) AppendFeature(#name""sv)
# if COMMON_ARCH_ARM
#   if defined(__aarch64__)
#     define CAPNUM 1
        CHECK_FEATURE(1, ASIMD, asimd);
        CHECK_FEATURE(CAPNUM, FP,       fp);
#   else
#     define CAPNUM 2
        CHECK_FEATURE(1, NEON, asimd);
#   endif
        CHECK_FEATURE(CAPNUM, AES,      aes);
        CHECK_FEATURE(CAPNUM, PMULL,    pmull);
        CHECK_FEATURE(CAPNUM, SHA1,     sha1);
        CHECK_FEATURE(CAPNUM, SHA2,     sha2);
        CHECK_FEATURE(CAPNUM, CRC32,    crc32);
# undef CAPNUM
# endif
# undef CHECK_FEATURE
# undef PFX1
# undef PFX2
#endif
    }
    void TrySysCtl() noexcept
    {
#if COMMON_OS_DARWIN && COMMON_ARCH_ARM
        constexpr auto SysCtl = [](const std::string_view name) -> bool 
        {
            char buf[100] = { 0 };
            size_t len = 100;
            if (sysctlbyname(name.data(), &buf, &len, nullptr, 0)) return false;
            return len >= 1 && buf[0] == 1;
        };
# define CHECK_FEATURE(name, feat) if (SysCtl("hw.optional."#name""sv)) AppendFeature(#feat""sv)
        CHECK_FEATURE(floatingpoint, fp);
#   if COMMON_OSBIT == 64
        AppendFeature("asimd"sv);
        AppendFeature("aes"sv);
        AppendFeature("pmull"sv);
        AppendFeature("sha1"sv);
        AppendFeature("sha2"sv);
#   else
        CHECK_FEATURE(neon, asimd);
#   endif
        CHECK_FEATURE(armv8_crc32, crc32);
        CHECK_FEATURE(armv8_2_sha512, sha512);
        CHECK_FEATURE(armv8_2_sha3, sha3);
# undef CHECK_FEATURE
#endif
    }

    CPUFeature() noexcept
    {
        TryCPUInfo();
        TryAUXVal();
        TrySysCtl();
        FeatureLookup = FeatureText;
    }
};

[[nodiscard]] static const CPUFeature& GetCPUFeatureHost() noexcept
{
    static const CPUFeature Host;
    return Host;
}


bool CheckCPUFeature(str::HashedStrView<char> feature) noexcept
{
    static const auto& features = GetCPUFeatureHost();
    return features.FeatureLookup.Has(feature);
}
span<const std::string_view> GetCPUFeatures() noexcept
{
    static const auto& features = GetCPUFeatureHost();
    return features.FeatureText;
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
