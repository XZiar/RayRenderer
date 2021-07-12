#include "SystemCommonPch.h"
#include "CopyEx.h"
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#include <boost/predef/other/endian.h>
#if COMMON_ARCH_X86 && !BOOST_ENDIAN_LITTLE_BYTE
#   error("unsupported std::byte order (non little endian) on x86")
#endif

namespace common
{
using namespace std::string_view_literals;
namespace fastpath
{
DEFINE_FASTPATH(CopyManager, Broadcast2);
DEFINE_FASTPATH(CopyManager, Broadcast4);
DEFINE_FASTPATH(CopyManager, ZExtCopy12);
DEFINE_FASTPATH(CopyManager, ZExtCopy14);
DEFINE_FASTPATH(CopyManager, ZExtCopy24);
DEFINE_FASTPATH(CopyManager, NarrowCopy21);
DEFINE_FASTPATH(CopyManager, NarrowCopy41);
#define Broadcast2Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define Broadcast4Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy12Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy14Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy24Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define NarrowCopy21Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define NarrowCopy41Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)

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
struct SIMDSSSE3
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("ssse3"sv);
#else
        return false;
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

DEFINE_FASTPATH_METHOD(Broadcast2, LOOP)
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
DEFINE_FASTPATH_METHOD(Broadcast4, LOOP)
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
template<typename Src, typename Dst>
static void CastCopyLoop(Dst* dest, const Src* src, size_t count) noexcept
{
    for (size_t iter = count / 8; iter > 0; iter--)
    {
        dest[0] = static_cast<Dst>(src[0]);
        dest[1] = static_cast<Dst>(src[1]);
        dest[2] = static_cast<Dst>(src[2]);
        dest[3] = static_cast<Dst>(src[3]);
        dest[4] = static_cast<Dst>(src[4]);
        dest[5] = static_cast<Dst>(src[5]);
        dest[6] = static_cast<Dst>(src[6]);
        dest[7] = static_cast<Dst>(src[7]);
        dest += 8; src += 8;
    }
    switch (count % 8)
    {
    case 7: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 6: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 5: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 4: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 3: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 2: *dest++ = static_cast<Dst>(*src++);
        [[fallthrough]];
    case 1: *dest++ = static_cast<Dst>(*src++);
        break;
    case 0:
        return;
    };
}
DEFINE_FASTPATH_METHOD(ZExtCopy12, LOOP)
{
    CastCopyLoop(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, LOOP)
{
    CastCopyLoop(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, LOOP)
{
    CastCopyLoop(dest, src, count);
}
DEFINE_FASTPATH_METHOD(NarrowCopy21, LOOP)
{
    CastCopyLoop(dest, src, count);
}
DEFINE_FASTPATH_METHOD(NarrowCopy41, LOOP)
{
    CastCopyLoop(dest, src, count);
}


template<typename T, auto F, typename Src, typename Dst>
forceinline void BroadcastSIMD4(Dst* dest, const Src src, size_t count) noexcept
{
    constexpr auto N = T::Count;
    T data(src);
    for (auto iter = count / (N * 4); iter > 0; --iter)
    {
        data.Save(dest + N * 0);
        data.Save(dest + N * 1);
        data.Save(dest + N * 2);
        data.Save(dest + N * 3);
        dest += N * 4;
    }
    count = count % (N * 4);
    switch (count / N)
    {
    case 3: data.Save(dest); dest += N;
        [[fallthrough]];
    case 2: data.Save(dest); dest += N;
        [[fallthrough]];
    case 1: data.Save(dest); dest += N;
        [[fallthrough]];
    default: break;
    }
    count = count % N;
    F(dest, src, count);
}


template<typename TIn, typename TCast, auto F, typename Src, typename Dst, size_t... I>
forceinline void ZExtCopySIMD4(Dst* dest, const Src* src, size_t count, std::index_sequence<I...>) noexcept
{
    constexpr auto N = TIn::Count, M = TCast::Count;
    static_assert((sizeof...(I) == N / M) && (N % M == 0));
    while (count >= N * 4)
    {
        const auto data0 = TIn(src + N * 0).template Cast<TCast>();
        const auto data1 = TIn(src + N * 1).template Cast<TCast>();
        const auto data2 = TIn(src + N * 2).template Cast<TCast>();
        const auto data3 = TIn(src + N * 3).template Cast<TCast>();
        (..., data0[I].Save(dest + N * 0 + M * I));
        (..., data1[I].Save(dest + N * 1 + M * I));
        (..., data2[I].Save(dest + N * 2 + M * I));
        (..., data3[I].Save(dest + N * 3 + M * I));
        src += N * 4; dest += N * 4; count -= N * 4;
    }
    switch (count / N)
    {
    case 3: { const auto data = TIn(src).template Cast<TCast>(); (..., data[I].Save(dest + M * I)); src += N; dest += N; }
        [[fallthrough]];
    case 2: { const auto data = TIn(src).template Cast<TCast>(); (..., data[I].Save(dest + M * I)); src += N; dest += N; }
        [[fallthrough]];
    case 1: { const auto data = TIn(src).template Cast<TCast>(); (..., data[I].Save(dest + M * I)); src += N; dest += N; }
        [[fallthrough]];
    default: break;
    }
    count = count % N;
    F(dest, src, count);
}

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 20) || (!COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100)

DEFINE_FASTPATH_METHOD(Broadcast2, SIMD128)
{
    BroadcastSIMD4<simd::U16x8, &GET_FASTPATH_METHOD(Broadcast2, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD128)
{
    BroadcastSIMD4<simd::U32x4, &GET_FASTPATH_METHOD(Broadcast4, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy12, SIMD128)
{
    ZExtCopySIMD4<simd::U8x16, simd::U16x8, &GET_FASTPATH_METHOD(ZExtCopy12, LOOP)>(dest, src, count, std::make_index_sequence<2>{});
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, SIMD128)
{
    ZExtCopySIMD4<simd::U16x8, simd::U32x4, &GET_FASTPATH_METHOD(ZExtCopy24, LOOP)>(dest, src, count, std::make_index_sequence<2>{});
}

#endif


#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMD128)
{
    ZExtCopySIMD4<simd::U8x16, simd::U32x4, &GET_FASTPATH_METHOD(ZExtCopy14, LOOP)>(dest, src, count, std::make_index_sequence<4>{});
}
#endif
#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 42
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMDSSSE3)
{
    ZExtCopySIMD4<simd::U8x16, simd::U32x4, &GET_FASTPATH_METHOD(ZExtCopy14, LOOP)>(dest, src, count, std::make_index_sequence<4>{});
}
#endif


#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100)

DEFINE_FASTPATH_METHOD(Broadcast2, SIMD256)
{
    BroadcastSIMD4<simd::U16x16, &GET_FASTPATH_METHOD(Broadcast2, SIMD128)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD256)
{
    BroadcastSIMD4<simd::U32x8, &GET_FASTPATH_METHOD(Broadcast4, SIMD128)>(dest, src, count);
}

#endif

common::span<const CopyManager::VarItem> CopyManager::GetSupportMap() noexcept
{
    static auto list = []() 
    {
        std::vector<VarItem> ret;
        RegistFuncVars(Broadcast2, SIMD256, SIMD128, LOOP);
        RegistFuncVars(Broadcast4, SIMD256, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy12, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy14, SIMDSSSE3, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy24, SIMD128, LOOP);
        RegistFuncVars(NarrowCopy21, LOOP);
        RegistFuncVars(NarrowCopy41, LOOP);
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
        CHECK_FUNC_VARS(func, var, ZExtCopy12, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy14, SIMDSSSE3, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy24, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, NarrowCopy21, LOOP);
        CHECK_FUNC_VARS(func, var, NarrowCopy41, LOOP);
        }
    }
}
const CopyManager CopyEx;



}