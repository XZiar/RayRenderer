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
#define Broadcast2Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define Broadcast4Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
DEFINE_FASTPATH(CopyManager, Broadcast2);
DEFINE_FASTPATH(CopyManager, Broadcast4);
#define ZExtCopy12Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy14Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy24Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy28Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define ZExtCopy48Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
DEFINE_FASTPATH(CopyManager, ZExtCopy12);
DEFINE_FASTPATH(CopyManager, ZExtCopy14);
DEFINE_FASTPATH(CopyManager, ZExtCopy24);
DEFINE_FASTPATH(CopyManager, ZExtCopy28);
DEFINE_FASTPATH(CopyManager, ZExtCopy48);
#define SExtCopy12Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define SExtCopy14Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define SExtCopy24Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define SExtCopy28Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define SExtCopy48Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
DEFINE_FASTPATH(CopyManager, SExtCopy12);
DEFINE_FASTPATH(CopyManager, SExtCopy14);
DEFINE_FASTPATH(CopyManager, SExtCopy24);
DEFINE_FASTPATH(CopyManager, SExtCopy28);
DEFINE_FASTPATH(CopyManager, SExtCopy48);
#define TruncCopy21Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define TruncCopy41Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
#define TruncCopy42Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count)
DEFINE_FASTPATH(CopyManager, TruncCopy21);
DEFINE_FASTPATH(CopyManager, TruncCopy41);
DEFINE_FASTPATH(CopyManager, TruncCopy42);
#define CvtI32F32Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal)
#define CvtI16F32Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal)
#define CvtI8F32Args  BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal)
DEFINE_FASTPATH(CopyManager, CvtI32F32);
DEFINE_FASTPATH(CopyManager, CvtI16F32);
DEFINE_FASTPATH(CopyManager, CvtI8F32 );
#define CvtF32I32Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal, sat)
#define CvtF32I16Args BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal, sat)
#define CvtF32I8Args  BOOST_PP_VARIADIC_TO_SEQ(dest, src, count, mulVal, sat)
DEFINE_FASTPATH(CopyManager, CvtF32I32);
DEFINE_FASTPATH(CopyManager, CvtF32I16);
DEFINE_FASTPATH(CopyManager, CvtF32I8 );

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
struct SIMDSSE41
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse4_1"sv);
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
struct SIMDAVX2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx2"sv);
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

template<typename Src, typename Dst, bool Sat = false>
struct ScalarCast
{
    template<typename... Args>
    constexpr ScalarCast(Args&&...) noexcept {}
    Dst operator()(const Src src) const noexcept
    {
        if constexpr (Sat)
        {
            if (src >= static_cast<Src>(std::numeric_limits<Dst>::max()))
                return std::numeric_limits<Dst>::max();
            if (src <= static_cast<Src>(std::numeric_limits<Dst>::min()))
                return std::numeric_limits<Dst>::min();
        }
        return static_cast<Dst>(src);
    }
};
template<typename Src, typename Dst, bool Sat = false>
struct ScalarI2FCast
{
    Dst Muler;
    ScalarI2FCast(Dst mulVal) noexcept : Muler(mulVal) {}
    Dst operator()(const Src src) const noexcept
    {
        return static_cast<Dst>(src) * Muler;
    }
};
template<typename Src, typename Dst, bool Sat = false>
struct ScalarF2ICast
{
    Src Muler;
    ScalarF2ICast(Src mulVal) noexcept : Muler(mulVal) {}
    Dst operator()(Src src) const noexcept
    {
        src *= Muler;
        if constexpr (Sat)
        {
            if (src >= static_cast<Src>(std::numeric_limits<Dst>::max()))
                return std::numeric_limits<Dst>::max();
            if (src <= static_cast<Src>(std::numeric_limits<Dst>::min()))
                return std::numeric_limits<Dst>::min();
        }
        return static_cast<Dst>(src);
    }
};
template<template<typename, typename, bool> class Cast, bool Sat = false, typename Src, typename Dst, typename... Args>
static void CastLoop(Dst* dest, const Src* src, size_t count, Args&&... args) noexcept
{
    Cast<Src, Dst, Sat> cast(std::forward<Args>(args)...);
    for (size_t iter = count / 8; iter > 0; iter--)
    {
        dest[0] = cast(src[0]);
        dest[1] = cast(src[1]);
        dest[2] = cast(src[2]);
        dest[3] = cast(src[3]);
        dest[4] = cast(src[4]);
        dest[5] = cast(src[5]);
        dest[6] = cast(src[6]);
        dest[7] = cast(src[7]);
        dest += 8; src += 8;
    }
    switch (count % 8)
    {
    case 7: *dest++ = cast(*src++);
        [[fallthrough]];
    case 6: *dest++ = cast(*src++);
        [[fallthrough]];
    case 5: *dest++ = cast(*src++);
        [[fallthrough]];
    case 4: *dest++ = cast(*src++);
        [[fallthrough]];
    case 3: *dest++ = cast(*src++);
        [[fallthrough]];
    case 2: *dest++ = cast(*src++);
        [[fallthrough]];
    case 1: *dest++ = cast(*src++);
        break;
    case 0:
        return;
    };
}
DEFINE_FASTPATH_METHOD(ZExtCopy12, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
#define DEFINE_CVTFP2I_LOOP(func) DEFINE_FASTPATH_METHOD(func, LOOP)    \
{                                                                       \
    if (mulVal == 0)                                                    \
        CastLoop<ScalarCast>(dest, src, count);                         \
    else                                                                \
        CastLoop<ScalarI2FCast>(dest, src, count, mulVal);              \
}
DEFINE_CVTFP2I_LOOP(CvtI32F32)
DEFINE_CVTFP2I_LOOP(CvtI16F32)
DEFINE_CVTFP2I_LOOP(CvtI8F32)
#define DEFINE_CVTI2FP_LOOP(func) DEFINE_FASTPATH_METHOD(func, LOOP)    \
{                                                                       \
    if (mulVal == 0)                                                    \
    {                                                                   \
        if (sat)                                                        \
            CastLoop<ScalarCast, true>(dest, src, count);               \
        else                                                            \
            CastLoop<ScalarCast>(dest, src, count);                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if (sat)                                                        \
            CastLoop<ScalarF2ICast, true>(dest, src, count, mulVal);    \
        else                                                            \
            CastLoop<ScalarF2ICast>(dest, src, count, mulVal);          \
    }                                                                   \
}
DEFINE_CVTI2FP_LOOP(CvtF32I32)
DEFINE_CVTI2FP_LOOP(CvtF32I16)
DEFINE_CVTI2FP_LOOP(CvtF32I8)


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


template<typename TIn, typename TCast, typename T, simd::CastMode Mode>
struct LoadCastSave
{
    using Src = std::decay_t<decltype(std::declval<TIn>().Val[0])>;
    using Dst = std::decay_t<decltype(std::declval<TCast>().Val[0])>;
    static constexpr size_t N = TIn::Count, M = TCast::Count;
    static constexpr size_t K = (N >= M) ? (N / M) : (M / N - 1);
    static constexpr auto Idxes = std::make_index_sequence<K>{};
    void operator()(Dst* dst, const Src* src) const noexcept
    {
        Proc(dst, src, Idxes);
    }
    template<size_t... I>
    void Proc(Dst* dst, const Src* src, std::index_sequence<I...>) const noexcept
    {
        const T& self = *static_cast<const T*>(this);
        if constexpr (N == M)
        {
            const auto val = self.Pre(TIn(src));
            self.Post(val.template Cast<TCast, Mode>()).Save(dst);
        }
        else if constexpr (N < M)
        {
            const TIn val[] = { self.Pre(TIn(src)), self.Pre(TIn(src + N + N * I))... };
            self.Post(val[0].template Cast<TCast, Mode>(val[1 + I]...)).Save(dst);
        }
        else
        {
            const auto mid = self.Pre(TIn(src)).template Cast<TCast, Mode>();
            (..., self.Post(mid[I]).Save(dst + M * I));
        }
    }
};
template<typename TIn, typename TCast, simd::CastMode Mode = simd::detail::CstMode<TIn, TCast>()>
struct DefaultCast : public LoadCastSave<TIn, TCast, DefaultCast<TIn, TCast, Mode>, Mode>
{
    template<typename... Args>
    constexpr DefaultCast(Args&&...) noexcept {}
    TIn Pre(TIn val) const noexcept { return val; }
    TCast Post(TCast val) const noexcept { return val; }
};
template<typename TIn, typename TCast, simd::CastMode Mode = simd::detail::CstMode<TIn, TCast>()>
struct I2FCast : public LoadCastSave<TIn, TCast, I2FCast<TIn, TCast, Mode>, Mode>
{
    TCast Muler;
    template<typename... Args>
    I2FCast(TCast mulVal, Args&&...) noexcept : Muler(mulVal) {}
    TIn Pre(TIn val) const noexcept { return val; }
    TCast Post(TCast val) const noexcept { return val.Mul(Muler); }
};
template<typename TIn, typename TCast, simd::CastMode Mode = simd::detail::CstMode<TIn, TCast>()>
struct F2ICast : public LoadCastSave<TIn, TCast, F2ICast<TIn, TCast, Mode>, Mode>
{
    TIn Muler;
    template<typename... Args>
    F2ICast(TIn mulVal, Args&&...) noexcept : Muler(mulVal) {}
    TIn Pre(TIn val) const noexcept { return val.Mul(Muler); }
    TCast Post(TCast val) const noexcept { return val; }
};
template<typename Cast, auto F, typename Src, typename Dst, typename... Args>
forceinline void CastSIMD4(Dst* dest, const Src* src, size_t count, Args&&... args) noexcept
{
    Cast cast(std::forward<Args>(args)...);
    constexpr auto K = std::max(Cast::N, Cast::M);
    for (size_t iter = count / (K * 4); iter > 0; iter--)
    {
        cast(dest + K * 0, src + K * 0);
        cast(dest + K * 1, src + K * 1);
        cast(dest + K * 2, src + K * 2);
        cast(dest + K * 3, src + K * 3);
        src += K * 4; dest += K * 4;
    }
    count = count % (K * 4);
    switch (count / K)
    {
    case 3: cast(dest, src); src += K; dest += K;
          [[fallthrough]];
    case 2: cast(dest, src); src += K; dest += K;
          [[fallthrough]];
    case 1: cast(dest, src); src += K; dest += K;
          [[fallthrough]];
    default: break;
    }
    count = count % K;
    F(dest, src, count, std::forward<Args>(args)...);
}

#define DEFINE_CVTI2FP_SIMD4(func, from, to, algo, prev) DEFINE_FASTPATH_METHOD(func, algo)                         \
{                                                                                                                   \
    if (mulVal == 0)                                                                                                \
        CastSIMD4<DefaultCast<simd::from, simd::to>, &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal);   \
    else                                                                                                            \
        CastSIMD4<I2FCast<simd::from, simd::to>, &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal);       \
}
#define DEFINE_CVTFP2I_SIMD4(func, from, to, algo, prev) DEFINE_FASTPATH_METHOD(func, algo) \
{                                                                                           \
    if (mulVal == 0)                                                                        \
    {                                                                                       \
        if (sat)                                                                            \
            CastSIMD4<DefaultCast<simd::from, simd::to, simd::CastMode::RangeSaturate>,     \
                &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal, sat);           \
        else                                                                                \
            CastSIMD4<DefaultCast<simd::from, simd::to>,                                    \
                &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal, sat);           \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        if (sat)                                                                            \
            CastSIMD4<F2ICast<simd::from, simd::to, simd::CastMode::RangeSaturate>,         \
                &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal, sat);           \
        else                                                                                \
            CastSIMD4<F2ICast<simd::from, simd::to>,                                        \
                &GET_FASTPATH_METHOD(func, prev)>(dest, src, count, mulVal, sat);           \
    }                                                                                       \
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
    CastSIMD4<DefaultCast<simd::U8x16, simd::U16x8>, &GET_FASTPATH_METHOD(ZExtCopy12, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U16x8, simd::U32x4>, &GET_FASTPATH_METHOD(ZExtCopy24, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U16x8, simd::U64x2>, &GET_FASTPATH_METHOD(ZExtCopy28, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U32x4, simd::U64x2>, &GET_FASTPATH_METHOD(ZExtCopy48, LOOP)>(dest, src, count);
}
DEFINE_CVTI2FP_SIMD4(CvtI32F32, I32x4, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtI16F32, I16x8, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtI8F32,  I8x16, F32x4, SIMD128, LOOP)

DEFINE_CVTFP2I_SIMD4(CvtF32I32, F32x4, I32x4, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32I16, F32x4, I16x8, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32I8,  F32x4, I8x16, SIMD128, LOOP)

#endif


#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U8x16, simd::U32x4>, &GET_FASTPATH_METHOD(ZExtCopy14, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMD128)
{
    CastSIMD4<DefaultCast<simd::I8x16, simd::I16x8>, &GET_FASTPATH_METHOD(SExtCopy12, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMD128)
{
    CastSIMD4<DefaultCast<simd::I8x16, simd::I32x4>, &GET_FASTPATH_METHOD(SExtCopy14, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMD128)
{
    CastSIMD4<DefaultCast<simd::I16x8, simd::I32x4>, &GET_FASTPATH_METHOD(SExtCopy24, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMD128)
{
    CastSIMD4<DefaultCast<simd::I16x8, simd::I64x2>, &GET_FASTPATH_METHOD(SExtCopy28, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMD128)
{
    CastSIMD4<DefaultCast<simd::I32x4, simd::I64x2>, &GET_FASTPATH_METHOD(SExtCopy48, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U16x8, simd::U8x16>, &GET_FASTPATH_METHOD(TruncCopy21, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U32x4, simd::U8x16>, &GET_FASTPATH_METHOD(TruncCopy41, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMD128)
{
    CastSIMD4<DefaultCast<simd::U32x4, simd::U16x8>, &GET_FASTPATH_METHOD(TruncCopy42, LOOP)>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<simd::U8x16, simd::U32x4>, &GET_FASTPATH_METHOD(ZExtCopy14, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<simd::U16x8, simd::U8x16>, &GET_FASTPATH_METHOD(TruncCopy21, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<simd::U32x4, simd::U8x16>, &GET_FASTPATH_METHOD(TruncCopy41, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<simd::U32x4, simd::U16x8>, &GET_FASTPATH_METHOD(TruncCopy42, LOOP)>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMDSSE41)
{
    CastSIMD4<DefaultCast<simd::I8x16, simd::I16x8>, &GET_FASTPATH_METHOD(SExtCopy12, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMDSSE41)
{
    CastSIMD4<DefaultCast<simd::I8x16, simd::I32x4>, &GET_FASTPATH_METHOD(SExtCopy14, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMDSSE41)
{
    CastSIMD4<DefaultCast<simd::I16x8, simd::I32x4>, &GET_FASTPATH_METHOD(SExtCopy24, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMDSSE41)
{
    CastSIMD4<DefaultCast<simd::I16x8, simd::I64x2>, &GET_FASTPATH_METHOD(SExtCopy28, LOOP)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMDSSE41)
{
    CastSIMD4<DefaultCast<simd::I32x4, simd::I64x2>, &GET_FASTPATH_METHOD(SExtCopy48, LOOP)>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(Broadcast2, SIMD256)
{
    BroadcastSIMD4<simd::U16x16, &GET_FASTPATH_METHOD(Broadcast2, SIMD128)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD256)
{
    BroadcastSIMD4<simd::U32x8, &GET_FASTPATH_METHOD(Broadcast4, SIMD128)>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI32F32, I32x8, F32x8, SIMD256, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I32, F32x8, I32x8, SIMD256, SIMD128)
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
DEFINE_FASTPATH_METHOD(ZExtCopy12, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U8x32, simd::U16x16>, &GET_FASTPATH_METHOD(ZExtCopy12, SIMD128)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U8x32, simd::U32x8>, &GET_FASTPATH_METHOD(ZExtCopy14, SIMDSSSE3)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U16x16, simd::U32x8>, &GET_FASTPATH_METHOD(ZExtCopy24, SIMD128)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U32x8, simd::U64x4>, &GET_FASTPATH_METHOD(ZExtCopy48, SIMD128)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::I8x32, simd::I16x16>, &GET_FASTPATH_METHOD(SExtCopy12, SIMDSSE41)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::I8x32, simd::I32x8>, &GET_FASTPATH_METHOD(SExtCopy14, SIMDSSE41)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::I16x16, simd::I32x8>, &GET_FASTPATH_METHOD(SExtCopy24, SIMDSSE41)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::I16x16, simd::I64x4>, &GET_FASTPATH_METHOD(SExtCopy28, SIMDSSE41)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::I32x8, simd::I64x4>, &GET_FASTPATH_METHOD(SExtCopy48, SIMDSSE41)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U16x16, simd::U8x32>, &GET_FASTPATH_METHOD(TruncCopy21, SIMDSSSE3)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U32x8, simd::U8x32>, &GET_FASTPATH_METHOD(TruncCopy41, SIMDSSSE3)>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMDAVX2)
{
    CastSIMD4<DefaultCast<simd::U32x8, simd::U16x16>, &GET_FASTPATH_METHOD(TruncCopy42, SIMDSSSE3)>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI16F32, I16x16, F32x8, SIMDAVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtI8F32,  I8x32,  F32x8, SIMDAVX2, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I16, F32x8, I16x16, SIMDAVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32I8,  F32x8, I8x32,  SIMDAVX2, SIMD128)
#endif

common::span<const CopyManager::VarItem> CopyManager::GetSupportMap() noexcept
{
    static auto list = []() 
    {
        std::vector<VarItem> ret;
        RegistFuncVars(Broadcast2, SIMD256, SIMD128, LOOP);
        RegistFuncVars(Broadcast4, SIMD256, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy12, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy14, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy24, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy28, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(ZExtCopy48, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(SExtCopy12, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        RegistFuncVars(SExtCopy14, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        RegistFuncVars(SExtCopy24, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        RegistFuncVars(SExtCopy28, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        RegistFuncVars(SExtCopy48, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        RegistFuncVars(TruncCopy21, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        RegistFuncVars(TruncCopy41, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        RegistFuncVars(TruncCopy42, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        RegistFuncVars(CvtI32F32, SIMD256,  SIMD128, LOOP);
        RegistFuncVars(CvtI16F32, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(CvtI8F32,  SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(CvtF32I32, SIMD256,  SIMD128, LOOP);
        RegistFuncVars(CvtF32I16, SIMDAVX2, SIMD128, LOOP);
        RegistFuncVars(CvtF32I8,  SIMDAVX2, SIMD128, LOOP);
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
        CHECK_FUNC_VARS(func, var, ZExtCopy12, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy14, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy24, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy28, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, ZExtCopy48, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, SExtCopy12, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, SExtCopy14, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, SExtCopy24, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, SExtCopy28, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, SExtCopy48, SIMDAVX2, SIMDSSE41, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, TruncCopy21, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, TruncCopy41, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, TruncCopy42, SIMDAVX2, SIMDSSSE3, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtI32F32, SIMD256,  SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtI16F32, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtI8F32,  SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtF32I32, SIMD256, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtF32I16, SIMDAVX2, SIMD128, LOOP);
        CHECK_FUNC_VARS(func, var, CvtF32I8,  SIMDAVX2, SIMD128, LOOP);
        }
    }
}
const CopyManager CopyEx;



}