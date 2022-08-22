#include "SystemCommonPch.h"
#include "Exceptions.h"
#include "SpinLock.h"
#include "common/FrozenDenseSet.hpp"
#include "common/simd/SIMD.hpp"
#if COMMON_ARCH_X86
#   include "3rdParty/libcpuid/libcpuid/libcpuid.h"
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

struct CPUFeature
{
    std::vector<std::string_view> FeatureText;
    container::FrozenDenseStringSet<char, false> FeatureLookup;
    void TryCPUID() noexcept
    {
        if (!FeatureText.empty()) return;
#if COMMON_ARCH_X86
        if (cpuid_present())
        {
            struct cpu_raw_data_t raw;
            if (cpuid_get_raw_data(&raw) >= 0)
            {
                if (raw.basic_cpuid[0][EAX] >= 7) 
                {
                    if (const auto reg = raw.basic_cpuid[7][ECX]; reg & (1u << 5))
                        FeatureText.push_back("waitpkg"sv);
                }
                cpu_id_t data;
                if (cpu_identify(&raw, &data) >= 0)
                {
# define CHECK_FEATURE(en, name) if (data.flags[CPU_FEATURE_##en]) FeatureText.push_back(#name""sv)
                    CHECK_FEATURE(SSE,          sse);
                    CHECK_FEATURE(SSE2,         sse2);
                    CHECK_FEATURE(PNI,          sse3);
                    CHECK_FEATURE(SSSE3,        ssse3);
                    CHECK_FEATURE(SSE4_1,       sse4_1);
                    CHECK_FEATURE(SSE4_2,       sse4_2);
                    CHECK_FEATURE(AVX,          avx);
                    CHECK_FEATURE(FMA3,         fma);
                    CHECK_FEATURE(AVX2,         avx2);
                    CHECK_FEATURE(AVX512F,      avx512f);
                    CHECK_FEATURE(AVX512DQ,     avx512dq);
                    CHECK_FEATURE(AVX512PF,     avx512pf);
                    CHECK_FEATURE(AVX512ER,     avx512er);
                    CHECK_FEATURE(AVX512CD,     avx512cd);
                    CHECK_FEATURE(AVX512BW,     avx512bw);
                    CHECK_FEATURE(AVX512VL,     avx512vl);
                    CHECK_FEATURE(AVX512VNNI,   avx512vnni);
                    CHECK_FEATURE(AVX512VBMI,   avx512vbmi);
                    CHECK_FEATURE(AVX512VBMI2,  avx512vbmi2);
                    CHECK_FEATURE(PCLMUL,       pclmul);
                    CHECK_FEATURE(POPCNT,       popcnt);
                    CHECK_FEATURE(AES,          aes);
                    CHECK_FEATURE(ABM,          lzcnt);
                    CHECK_FEATURE(F16C,         f16c);
                    CHECK_FEATURE(BMI1,         bmi1);
                    CHECK_FEATURE(BMI2,         bmi2);
                    CHECK_FEATURE(SHA_NI,       sha);
                    CHECK_FEATURE(ADX,          adx);
# undef CHECK_FEATURE
                }
            }
        }
#endif
    }
    void TryAUXVal() noexcept
    {
        if (!FeatureText.empty()) return;
#if COMMON_OS_LINUX && COMMON_ARCH_ARM && (!COMMON_OS_ANDROID || __NDK_MAJOR__ >= 18)
        [[maybe_unused]] const auto cap1 = getauxval(AT_HWCAP), cap2 = getauxval(AT_HWCAP2);
        //printf("cap1&2: [%lx][%lx]\n", cap1, cap2);
# define PFX1(en) PPCAT(HWCAP_,  en)
# define PFX2(en) PPCAT(HWCAP2_, en)
# define CHECK_FEATURE(n, en, name) if (PPCAT(cap, n) & PPCAT(PFX, n)(en)) FeatureText.emplace_back(#name""sv)
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
        if (!FeatureText.empty()) return;
#if COMMON_OS_DARWIN && COMMON_ARCH_ARM
        constexpr auto SysCtl = [](const std::string_view name) -> bool 
        {
            char buf[100] = { 0 };
            size_t len = 100;
            if (sysctlbyname(name.data(), &buf, &len, nullptr, 0)) return false;
            return len >= 1 && buf[0] == 1;
        };
# define CHECK_FEATURE(name, feat) if (SysCtl("hw.optional."#name""sv)) FeatureText.emplace_back(#feat""sv)
        CHECK_FEATURE(floatingpoint, fp);
#   if COMMON_OSBIT == 64
        FeatureText.emplace_back("asimd"sv);
        FeatureText.emplace_back("aes"sv);
        FeatureText.emplace_back("pmull"sv);
        FeatureText.emplace_back("sha1"sv);
        FeatureText.emplace_back("sha2"sv);
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
        TryCPUID();
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


