
#include "FastPathCategory.h"
#include "CopyEx.h"
#include "common/simd/SIMD.hpp"

#define HALF_ENABLE_F16C_INTRINSICS 0 // only use half as fallback
#include "3rdParty/half/half.hpp"
#include <bit>
#if !defined(__cpp_lib_bitops) || __cpp_lib_bitops < 201907L
#   error "need bitops support"
#endif

static_assert(::common::detail::is_little_endian, "unsupported std::byte order (non little endian)");


#define Broadcast2Info  (void)(uint16_t* dest, const uint16_t src, size_t count)
#define Broadcast4Info  (void)(uint32_t* dest, const uint32_t src, size_t count)
#define ZExtCopy12Info  (void)(uint16_t* dest, const uint8_t* src, size_t count)
#define ZExtCopy14Info  (void)(uint32_t* dest, const uint8_t* src, size_t count)
#define ZExtCopy24Info  (void)(uint32_t* dest, const uint16_t* src, size_t count)
#define ZExtCopy28Info  (void)(uint64_t* dest, const uint16_t* src, size_t count)
#define ZExtCopy48Info  (void)(uint64_t* dest, const uint32_t* src, size_t count)
#define SExtCopy12Info  (void)(int16_t* dest, const int8_t* src, size_t count)
#define SExtCopy14Info  (void)(int32_t* dest, const int8_t* src, size_t count)
#define SExtCopy24Info  (void)(int32_t* dest, const int16_t* src, size_t count)
#define SExtCopy28Info  (void)(int64_t* dest, const int16_t* src, size_t count)
#define SExtCopy48Info  (void)(int64_t* dest, const int32_t* src, size_t count)
#define TruncCopy21Info (void)(uint8_t * dest, const uint16_t* src, size_t count)
#define TruncCopy41Info (void)(uint8_t * dest, const uint32_t* src, size_t count)
#define TruncCopy42Info (void)(uint16_t* dest, const uint32_t* src, size_t count)
#define TruncCopy81Info (void)(uint8_t * dest, const uint64_t* src, size_t count)
#define TruncCopy82Info (void)(uint16_t* dest, const uint64_t* src, size_t count)
#define TruncCopy84Info (void)(uint32_t* dest, const uint64_t* src, size_t count)
#define CvtI32F32Info   (void)(float* dest, const int32_t* src, size_t count, float mulVal)
#define CvtI16F32Info   (void)(float* dest, const int16_t* src, size_t count, float mulVal)
#define CvtI8F32Info    (void)(float* dest, const int8_t * src, size_t count, float mulVal)
#define CvtU32F32Info   (void)(float* dest, const uint32_t* src, size_t count, float mulVal)
#define CvtU16F32Info   (void)(float* dest, const uint16_t* src, size_t count, float mulVal)
#define CvtU8F32Info    (void)(float* dest, const uint8_t * src, size_t count, float mulVal)
#define CvtF32I32Info   (void)(int32_t* dest, const float* src, size_t count, float mulVal, bool saturate)
#define CvtF32I16Info   (void)(int16_t* dest, const float* src, size_t count, float mulVal, bool saturate)
#define CvtF32I8Info    (void)(int8_t * dest, const float* src, size_t count, float mulVal, bool saturate)
#define CvtF32U16Info   (void)(uint16_t* dest, const float* src, size_t count, float mulVal, bool saturate)
#define CvtF32U8Info    (void)(uint8_t * dest, const float* src, size_t count, float mulVal, bool saturate)
#define CvtF16F32Info   (void)(float   * dest, const uint16_t* src, size_t count)
#define CvtF32F16Info   (void)(uint16_t* dest, const float   * src, size_t count)
#define CvtF32F64Info   (void)(double* dest, const float * src, size_t count)
#define CvtF64F32Info   (void)(float * dest, const double* src, size_t count)


namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::common::CopyManager;


#if COMMON_ARCH_ARM && COMMON_COMPILER_MSVC
using float16_t = uint16_t;
#endif


DEFINE_FASTPATHS(CopyManager,
    Broadcast2, Broadcast4,
    ZExtCopy12, ZExtCopy14, ZExtCopy24, ZExtCopy28, ZExtCopy48,
    SExtCopy12, SExtCopy14, SExtCopy24, SExtCopy28, SExtCopy48,
    TruncCopy21, TruncCopy41, TruncCopy42, TruncCopy81, TruncCopy82, TruncCopy84,
    CvtI32F32, CvtI16F32, CvtI8F32, CvtU32F32, CvtU16F32, CvtU8F32,
    CvtF32I32, CvtF32I16, CvtF32I8, CvtF32U16, CvtF32U8,
    CvtF16F32, CvtF32F16, CvtF32F64, CvtF64F32)


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
DEFINE_FASTPATH_METHOD(TruncCopy81, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, LOOP)
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
DEFINE_CVTFP2I_LOOP(CvtU32F32)
DEFINE_CVTFP2I_LOOP(CvtU16F32)
DEFINE_CVTFP2I_LOOP(CvtU8F32)
#define DEFINE_CVTI2FP_LOOP(func) DEFINE_FASTPATH_METHOD(func, LOOP)    \
{                                                                       \
    if (mulVal == 0)                                                    \
    {                                                                   \
        if (saturate)                                                   \
            CastLoop<ScalarCast, true>(dest, src, count);               \
        else                                                            \
            CastLoop<ScalarCast>(dest, src, count);                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if (saturate)                                                   \
            CastLoop<ScalarF2ICast, true>(dest, src, count, mulVal);    \
        else                                                            \
            CastLoop<ScalarF2ICast>(dest, src, count, mulVal);          \
    }                                                                   \
}
DEFINE_CVTI2FP_LOOP(CvtF32I32)
DEFINE_CVTI2FP_LOOP(CvtF32I16)
DEFINE_CVTI2FP_LOOP(CvtF32I8)
DEFINE_CVTI2FP_LOOP(CvtF32U16)
DEFINE_CVTI2FP_LOOP(CvtF32U8)
DEFINE_FASTPATH_METHOD(CvtF16F32, LOOP)
{
    const auto src_ = reinterpret_cast<const half_float::half*>(src);
    CastLoop<ScalarCast>(dest, src_, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, LOOP)
{
    const auto dst = reinterpret_cast<half_float::half*>(dest);
    CastLoop<ScalarCast>(dst, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F64, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF64F32, LOOP)
{
    CastLoop<ScalarCast>(dest, src, count);
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


template<typename TIn, typename TCast, typename T, CastMode Mode>
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
template<typename TIn, typename TCast, CastMode Mode = ::common::simd::detail::CstMode<TIn, TCast>()>
struct DefaultCast : public LoadCastSave<TIn, TCast, DefaultCast<TIn, TCast, Mode>, Mode>
{
    template<typename... Args>
    constexpr DefaultCast(Args&&...) noexcept {}
    TIn Pre(TIn val) const noexcept { return val; }
    TCast Post(TCast val) const noexcept { return val; }
};
template<typename TIn, typename TCast, CastMode Mode = ::common::simd::detail::CstMode<TIn, TCast>()>
struct I2FCast : public LoadCastSave<TIn, TCast, I2FCast<TIn, TCast, Mode>, Mode>
{
    TCast Muler;
    template<typename... Args>
    I2FCast(TCast mulVal, Args&&...) noexcept : Muler(mulVal) {}
    TIn Pre(TIn val) const noexcept { return val; }
    TCast Post(TCast val) const noexcept { return val.Mul(Muler); }
};
template<typename TIn, typename TCast, CastMode Mode = ::common::simd::detail::CstMode<TIn, TCast>()>
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

#define DEFINE_CVTI2FP_SIMD4(func, from, to, algo, prev)                        \
DEFINE_FASTPATH_METHOD(func, algo)                                              \
{                                                                               \
    if (mulVal == 0)                                                            \
        CastSIMD4<DefaultCast<from, to>, &Func<prev>>(dest, src, count, mulVal);\
    else                                                                        \
        CastSIMD4<I2FCast<from, to>, &Func<prev>>(dest, src, count, mulVal);    \
}
#define DEFINE_CVTFP2I_SIMD4(func, from, to, algo, prev)                \
DEFINE_FASTPATH_METHOD(func, algo)                                      \
{                                                                       \
    if (mulVal == 0)                                                    \
    {                                                                   \
        if (saturate)                                                   \
            CastSIMD4<DefaultCast<from, to, CastMode::RangeSaturate>,   \
                &Func<prev>>(dest, src, count, mulVal, saturate);       \
        else                                                            \
            CastSIMD4<DefaultCast<from, to>,                            \
                &Func<prev>>(dest, src, count, mulVal, saturate);       \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if (saturate)                                                   \
            CastSIMD4<F2ICast<from, to, CastMode::RangeSaturate>,       \
                &Func<prev>>(dest, src, count, mulVal, saturate);       \
        else                                                            \
            CastSIMD4<F2ICast<from, to>,                                \
                &Func<prev>>(dest, src, count, mulVal, saturate);       \
    }                                                                   \
}

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 20) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 10)

DEFINE_FASTPATH_METHOD(Broadcast2, SIMD128)
{
    BroadcastSIMD4<U16x8, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD128)
{
    BroadcastSIMD4<U32x4, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy12, SIMD128)
{
    CastSIMD4<DefaultCast<U8x16, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, SIMD128)
{
    CastSIMD4<DefaultCast<U16x8, U32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, SIMD128)
{
    CastSIMD4<DefaultCast<U16x8, U64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, SIMD128)
{
    CastSIMD4<DefaultCast<U32x4, U64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_CVTI2FP_SIMD4(CvtI32F32, I32x4, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtI16F32, I16x8, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtI8F32,  I8x16, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtU32F32, U32x4, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtU16F32, U16x8, F32x4, SIMD128, LOOP)
DEFINE_CVTI2FP_SIMD4(CvtU8F32,  U8x16, F32x4, SIMD128, LOOP)

DEFINE_CVTFP2I_SIMD4(CvtF32I32, F32x4, I32x4, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32I16, F32x4, I16x8, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32I8,  F32x4, I8x16, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32U16, F32x4, U16x8, SIMD128, LOOP)
DEFINE_CVTFP2I_SIMD4(CvtF32U8,  F32x4, U8x16, SIMD128, LOOP)

#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 20) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 200)
DEFINE_FASTPATH_METHOD(CvtF32F64, SIMD128)
{
    CastSIMD4<DefaultCast<F32x4, F64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF64F32, SIMD128)
{
    CastSIMD4<DefaultCast<F64x2, F32x4>, &Func<LOOP>>(dest, src, count);
}
#endif


#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 10
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMD128)
{
    CastSIMD4<DefaultCast<U8x16, U32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMD128)
{
    CastSIMD4<DefaultCast<I8x16, I16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMD128)
{
    CastSIMD4<DefaultCast<I8x16, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMD128)
{
    CastSIMD4<DefaultCast<I16x8, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMD128)
{
    CastSIMD4<DefaultCast<I16x8, I64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMD128)
{
    CastSIMD4<DefaultCast<I32x4, I64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMD128)
{
    CastSIMD4<DefaultCast<U16x8, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMD128)
{
    CastSIMD4<DefaultCast<U32x4, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMD128)
{
    CastSIMD4<DefaultCast<U32x4, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy81, SIMD128)
{
    CastSIMD4<DefaultCast<U64x2, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, SIMD128)
{
    CastSIMD4<DefaultCast<U64x2, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, SIMD128)
{
    CastSIMD4<DefaultCast<U64x2, U32x4>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U8x16, U32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U16x8, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U32x4, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U32x4, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy81, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, SIMDSSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U32x4>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMDSSE41)
{
    CastSIMD4<DefaultCast<I8x16, I16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMDSSE41)
{
    CastSIMD4<DefaultCast<I8x16, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMDSSE41)
{
    CastSIMD4<DefaultCast<I16x8, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMDSSE41)
{
    CastSIMD4<DefaultCast<I16x8, I64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMDSSE41)
{
    CastSIMD4<DefaultCast<I32x4, I64x2>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(Broadcast2, SIMD256)
{
    BroadcastSIMD4<U16x16, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, SIMD256)
{
    BroadcastSIMD4<U32x8, &Func<SIMD128>>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI32F32, I32x8, F32x8, SIMD256, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I32, F32x8, I32x8, SIMD256, SIMD128)

DEFINE_FASTPATH_METHOD(CvtF32F64, SIMD256)
{
    CastSIMD4<DefaultCast<F32x8, F64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF64F32, SIMD256)
{
    CastSIMD4<DefaultCast<F64x4, F32x8>, &Func<SIMD128>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
DEFINE_FASTPATH_METHOD(ZExtCopy12, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U8x32, U16x16>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U8x32, U32x8>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U16x16, U32x8>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U16x16, U64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U32x8, U64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, SIMDAVX2)
{
    CastSIMD4<DefaultCast<I8x32, I16x16>, &Func<SIMDSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SIMDAVX2)
{
    CastSIMD4<DefaultCast<I8x32, I32x8>, &Func<SIMDSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SIMDAVX2)
{
    CastSIMD4<DefaultCast<I16x16, I32x8>, &Func<SIMDSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SIMDAVX2)
{
    CastSIMD4<DefaultCast<I16x16, I64x4>, &Func<SIMDSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SIMDAVX2)
{
    CastSIMD4<DefaultCast<I32x8, I64x4>, &Func<SIMDSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U16x16, U8x32>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U32x8, U8x32>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U32x8, U16x16>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy81, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U64x4, U8x32>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U64x4, U16x16>, &Func<SIMDSSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, SIMDAVX2)
{
    CastSIMD4<DefaultCast<U64x4, U32x8>, &Func<SIMDSSSE3>>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI16F32, I16x16, F32x8, SIMDAVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtI8F32,  I8x32,  F32x8, SIMDAVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU32F32, U32x8,  F32x8, SIMDAVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU16F32, U16x16, F32x8, SIMDAVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU8F32,  U8x32,  F32x8, SIMDAVX2, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I16, F32x8, I16x16, SIMDAVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32I8,  F32x8, I8x32,  SIMDAVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32U16, F32x8, U16x16, SIMDAVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32U8,  F32x8, U8x32,  SIMDAVX2, SIMD128)
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100 && (defined(__F16C__) || COMMON_COMPILER_MSVC)
struct F1632Cast
{
    using Src = uint16_t;
    using Dst = float;
    static constexpr size_t N = 8, M = 8;
    void operator()(float* dst, const uint16_t* src) const noexcept
    {
        F32x8(_mm256_cvtph_ps(U16x8(src))).Save(dst);
    }
};
struct F3216Cast
{
    static constexpr size_t N = 8, M = 8;
    void operator()(uint16_t* dst, const float* src) const noexcept
    {
        U16x8(_mm256_cvtps_ph(F32x8(src), _MM_FROUND_TO_NEAREST_INT)).Save(dst);
    }
};
DEFINE_FASTPATH_METHOD(CvtF16F32, F16C)
{
    CastSIMD4<F1632Cast, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, F16C)
{
    CastSIMD4<F3216Cast, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 30
struct F1632Cast1
{
    using Src = uint16_t;
    using Dst = float;
    static constexpr size_t N = 8, M = 8;
    void operator()(float* dst, const uint16_t* src) const noexcept
    {
# if COMMON_SIMD_LV >= 200
#   if COMMON_COMPILER_MSVC
        const auto in = vld1q_u16(src); // msvc only typedef __n128 & __n64
#   else
        const auto in = vld1q_f16(reinterpret_cast<const float16_t*>(src));
#   endif
        F32x4(vcvt_f32_f16(vget_low_f16(in))).Save(dst + 0);
        F32x4(vcvt_high_f32_f16(in))         .Save(dst + 4);
# else
#   if COMMON_COMPILER_MSVC
        const auto in = vld1_u16_x2(src); // msvc only typedef __n128 & __n64
#   else
        const auto in = vld1_f16_x2(reinterpret_cast<const float16_t*>(src));
#   endif
        F32x4(vcvt_f32_f16(in.val[0])).Save(dst + 0);
        F32x4(vcvt_f32_f16(in.val[1])).Save(dst + 4);
# endif
    }
};
struct F3216Cast1
{
    static constexpr size_t N = 8, M = 8;
    void operator()(uint16_t* dst, const float* src) const noexcept
    {
        const auto dst_ = reinterpret_cast<float16_t*>(dst);
        const auto in = vld1q_f32_x2(src);
# if COMMON_SIMD_LV >= 200
        const auto out = vcvt_high_f16_f32(vcvt_f16_f32(in.val[0]), in.val[1]);
#   if COMMON_COMPILER_MSVC
        vst1q_u16(dst_, out); // msvc only typedef __n128 & __n64
#   else
        vst1q_f16(dst_, out);
#   endif
# else
        vst1_f16(dst_ + 0, vcvt_f16_f32(in.val[0]));
        vst1_f16(dst_ + 4, vcvt_f16_f32(in.val[1]));
# endif
    }
};
DEFINE_FASTPATH_METHOD(CvtF16F32, SIMD128)
{
    CastSIMD4<F1632Cast1, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, SIMD128)
{
    CastSIMD4<F3216Cast1, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 310 && (!COMMON_COMPILER_MSVC || COMMON_MSVC_VER >= 191000)
struct F1632CastAVX512
{
    using Src = uint16_t;
    using Dst = float;
    static constexpr size_t N = 16, M = 16;
    void operator()(float* dst, const uint16_t* src) const noexcept
    {
        _mm512_storeu_ps(dst, _mm512_cvtph_ps(U16x16(src)));
    }
};
struct F3216CastAVX512
{
    static constexpr size_t N = 16, M = 16;
    void operator()(uint16_t* dst, const float* src) const noexcept
    {
        U16x16(_mm512_cvtps_ph(_mm512_loadu_ps(src), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)).Save(dst);
    }
};
DEFINE_FASTPATH_METHOD(CvtF16F32, AVX512F)
{
    CastSIMD4<F1632CastAVX512, &Func<F16C>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, AVX512F)
{
    CastSIMD4<F3216CastAVX512, &Func<F16C>>(dest, src, count);
}
#endif

}