#include "SystemCommonPch.h"
#include "CopyEx.h"
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

namespace common
{
using namespace std::string_view_literals;
namespace fastpath
{
struct Broadcast2 { using RetType = void; };
struct Broadcast4 { using RetType = void; };

struct LOOP : FuncVarBase {};
struct SIMD128 
{ 
    static bool RuntimeCheck() noexcept 
    { 
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2"sv);
#else
        return CheckCPUFeature("asimd"sv);
#endif
    }
};
struct SIMD256
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx"sv);
#else
        return false;
#endif
    }
};
}

DEFINE_FASTPATH_METHOD(Broadcast2, LOOP, uint16_t* dest, const uint16_t src, size_t count)
{
    const uint64_t data = src * 0x0001000100010001u;
    while (count >= 4)
    {
        *reinterpret_cast<uint64_t*>(dest) = data;
        dest += 4; count -= 4;
    }
    switch (count)
    {
    case 3: *dest++ = src;
        [[fallthrough]];
    case 2: *dest++ = src;
        [[fallthrough]];
    case 1: *dest++ = src;
        [[fallthrough]];
    default: break;
    }
}
DEFINE_FASTPATH_METHOD(Broadcast4, LOOP, uint32_t* dest, const uint32_t src, size_t count)
{
    const uint64_t dat = src * 0x0000000100000001u;
    while (count >= 2)
    {
        *reinterpret_cast<uint64_t*>(dest) = dat;
        dest += 2; count -= 2;
    }
    if (count)
    {
        *dest++ = src;
    }
}

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 42) || (!COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100)

DEFINE_FASTPATH_METHOD(Broadcast2, SIMD128, uint16_t* dest, const uint16_t src, size_t count)
{
    simd::U16x8 data(src);
    while (count >= 32)
    {
        data.Save(dest);
        data.Save(dest + 8);
        data.Save(dest + 16);
        data.Save(dest + 24);
        dest += 32; count -= 32;
    }
    switch (count / 8)
    {
    case 3: data.Save(dest); dest += 8;
        [[fallthrough]];
    case 2: data.Save(dest); dest += 8;
        [[fallthrough]];
    case 1: data.Save(dest); dest += 8;
        [[fallthrough]];
    default: break;
    }
    count = count % 8;
    CALL_FASTPATH_METHOD(Broadcast2, LOOP, dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD128, uint32_t* dest, const uint32_t src, size_t count)
{
    simd::U32x4 data(src);
    while (count >= 16)
    {
        data.Save(dest);
        data.Save(dest + 4);
        data.Save(dest + 8);
        data.Save(dest + 12);
        dest += 16; count -= 16;
    }
    switch (count / 4)
    {
    case 3: data.Save(dest); dest += 4;
        [[fallthrough]];
    case 2: data.Save(dest); dest += 4;
        [[fallthrough]];
    case 1: data.Save(dest); dest += 4;
        [[fallthrough]];
    default: break;
    }
    count = count % 4;
    CALL_FASTPATH_METHOD(Broadcast4, LOOP, dest, src, count);
}

#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100)

DEFINE_FASTPATH_METHOD(Broadcast2, SIMD256, uint16_t* dest, const uint16_t src, size_t count)
{
    simd::U16x16 data(src);
    while (count >= 64)
    {
        data.Save(dest);
        data.Save(dest + 16);
        data.Save(dest + 32);
        data.Save(dest + 48);
        dest += 64; count -= 64;
    }
    switch (count / 16)
    {
    case 3: data.Save(dest); dest += 16;
        [[fallthrough]];
    case 2: data.Save(dest); dest += 16;
        [[fallthrough]];
    case 1: data.Save(dest); dest += 16;
        [[fallthrough]];
    default: break;
    }
    count = count % 16;
    CALL_FASTPATH_METHOD(Broadcast2, SIMD128, dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD256, uint32_t* dest, const uint32_t src, size_t count)
{
    simd::U32x8 data(src);
    while (count >= 32)
    {
        data.Save(dest);
        data.Save(dest + 8);
        data.Save(dest + 16);
        data.Save(dest + 24);
        dest += 32; count -= 32;
    }
    switch (count / 8)
    {
    case 3: data.Save(dest); dest += 8;
        [[fallthrough]];
    case 2: data.Save(dest); dest += 8;
        [[fallthrough]];
    case 1: data.Save(dest); dest += 8;
        [[fallthrough]];
    default: break;
    }
    count = count % 8;
    CALL_FASTPATH_METHOD(Broadcast4, SIMD128, dest, src, count);
}

#endif

common::span<const CopyManager::VarItem> CopyManager::GetSupportMap() noexcept
{
    static auto list = []() 
    {
        std::vector<VarItem> ret;
        RegistFuncVars(Broadcast2, SIMD256, SIMD128, LOOP);
        RegistFuncVars(Broadcast4, SIMD256, SIMD128, LOOP);
        return ret;
    }();
    return list;
}
CopyManager::CopyManager(common::span<const CopyManager::VarItem> requests) noexcept
{
    for (const auto& [func, var] : requests)
    {
        switch (DJBHash::HashC(func))
        {
        CHECK_FUNC_VARS(func, var, Broadcast2, SIMD256, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, Broadcast4, SIMD256, SIMD128, LOOP);
        }
    }
}
const CopyManager CopyEx;



#undef TestFeature
#undef SetFunc
#undef UseIfExist
#undef FallbackTo

}