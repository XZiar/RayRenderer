//
// pch.cpp
// Include the standard header and generate the precompiled header.
//

#include "pch.h"
#include "3rdParty/cpuinfo/include/cpuinfo.h"


template<size_t N>
std::vector<std::array<uint8_t, N>> GenerateAllPoses() noexcept
{
    std::vector<std::array<uint8_t, N>> all;
    constexpr auto Count = Pow<N, N>();
    all.resize(Count);
    for (size_t j = 0; j < Count; ++j)
    {
        size_t val = j;
        for (size_t i = 0; i < N; ++i)
        {
            all[j][i] = static_cast<uint8_t>(val % N);
            val /= N;
        }
    }
    return all;
}


template<> 
std::vector<std::array<uint8_t, 2>> PosesHolder<2>::Poses = GenerateAllPoses<2>();
template<>
std::vector<std::array<uint8_t, 4>> PosesHolder<4>::Poses = GenerateAllPoses<4>();
template<>
std::vector<std::array<uint8_t, 8>> PosesHolder<8>::Poses = GenerateAllPoses<8>();


std::mt19937& GetRanEng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
uint32_t GetARand()
{
    return GetRanEng()();
}


void MemCopy(void* dest, size_t destsz, const void* src, size_t count)
{
    memcpy_s(dest, destsz, src, count);
}


alignas(32) const std::array<uint8_t, RandValBytes / 1> RandVals = []()
{
    std::array<uint8_t, RandValBytes / 1> vals = {};
    for (auto& val : vals)
        val = static_cast<uint8_t>(GetARand());
    return vals;
}();
alignas(32) const std::array<float, RandValBytes / 4> RandValsF32 = []()
{
    std::array<float, RandValBytes / 4> vals = {};
    for (auto& val : vals)
        val = static_cast<float>(static_cast<int32_t>(GetARand()));
    return vals;
}();
alignas(32) const std::array<double, RandValBytes / 8> RandValsF64 = []()
{
    std::array<double, RandValBytes / 8> vals = {};
    for (auto& val : vals)
        val = static_cast<double>(static_cast<int32_t>(GetARand()));
    return vals;
}();


static uint32_t GetSIMDLevel_()
{
    cpuinfo_initialize();

#if COMMON_ARCH_X86
    if (cpuinfo_has_x86_avx512bw() && cpuinfo_has_x86_avx512dq() && cpuinfo_has_x86_avx512vl())
        return 320;
    if (cpuinfo_has_x86_avx512f() && cpuinfo_has_x86_avx512cd())
        return 310;
    if (cpuinfo_has_x86_avx2())
        return 200;
    if (cpuinfo_has_x86_fma3())
        return 150;
    if (cpuinfo_has_x86_avx())
        return 100;
    if (cpuinfo_has_x86_sse4_2())
        return 42;
    if (cpuinfo_has_x86_sse4_1())
        return 41;
    if (cpuinfo_has_x86_ssse3())
        return 31;
    if (cpuinfo_has_x86_sse3())
        return 30;
    if (cpuinfo_has_x86_sse2())
        return 20;
    if (cpuinfo_has_x86_sse())
        return 10;
#else
# if COMMON_OSBIT == 64
    if (cpuinfo_has_arm_neon())
        return 200;
# endif
    if (cpuinfo_has_arm_neon_v8())
        return 100;
    if (cpuinfo_has_arm_neon())
    {
        if (cpuinfo_has_arm_vfpv4())
            return 40;
        if (cpuinfo_has_arm_vfpv3())
            return 30;
        if (cpuinfo_has_arm_vfpv2())
            return 20;
        return 10;
    }
#endif
    return 0;
}

uint32_t SIMDFixture::GetSIMDLevel()
{
    static const uint32_t LEVEL = GetSIMDLevel_();
    return LEVEL;
}

std::string_view SIMDFixture::GetSIMDLevelName(const uint32_t level)
{
#if COMMON_ARCH_X86
    if (level >= 320)
        return "AVX512";
    if (level >= 310)
        return "AVX512";
    if (level >= 200)
        return "AVX2";
    if (level >= 150)
        return "FMA";
    if (level >= 100)
        return "AVX";
    if (level >= 42)
        return "SSE4_2";
    if (level >= 41)
        return "SSE4_1";
    if (level >= 31)
        return "SSSE3";
    if (level >= 30)
        return "SSE3";
    if (level >= 20)
        return "SSE2";
    if (level >= 10)
        return "SSE";
#else
    if (level >= 200)
        return "A64";
    if (level >= 100)
        return "A32";
    if (level >= 50)
        return "NEON_VFPv5";
    if (level >= 40)
        return "NEON_VFPv4";
    if (level >= 30)
        return "NEON_VFPv3";
    if (level >= 20)
        return "NEON_VFPv2";
    if (level >= 10)
        return "NEON";
#endif
    else
        return "NONE";
}


GTEST_DEFAULT_MAIN
