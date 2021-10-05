#include "SystemCommonPch.h"
#include "Exceptions.h"
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


std::shared_ptr<ExceptionBasicInfo> detail::ExceptionHelper::GetCurrentException() noexcept
{
    const auto cex = std::current_exception();
    if (!cex)
        return {};
    try
    {
        std::rethrow_exception(cex);
    }
    catch (const BaseException& be)
    {
        return be.Info;
    }
    catch (...)
    {
        return OtherException(cex).Info;
    }
}

[[nodiscard]] std::vector<StackTraceItem> BaseException::ExceptionStacks() const
{
    std::vector<StackTraceItem> ret;
    for (auto ex = Info.get(); ex; ex = ex->InnerException.get())
    {
        if (ex->StackTrace.size() > 0)
            ret.push_back(ex->StackTrace[0]);
        else
            ret.emplace_back(u"Undefined", u"Undefined", 0);
    }
    return ret;
}

[[nodiscard]] std::u16string_view BaseException::GetDetailMessage() const noexcept
{
    const auto ptr = Info->Resources.QueryItem("detail");
    if (ptr)
    {
        if (ptr->type() == typeid(common::SharedString<char16_t>))
            return *std::any_cast<common::SharedString<char16_t>>(ptr);
        if (ptr->type() == typeid(std::u16string))
            return *std::any_cast<std::u16string>(ptr);
        if (ptr->type() == typeid(std::u16string_view))
            return *std::any_cast<std::u16string_view>(ptr);
    }
    return {};
}


}


