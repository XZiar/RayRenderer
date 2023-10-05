
#include "FastPathCategory.h"
#include "CopyEx.h"
#include "common/simd/SIMD.hpp"

#define HALF_ENABLE_F16C_INTRINSICS 0 // only use half as fallback
#include "3rdParty/half/half.hpp"

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
template<typename Cast, auto F, typename Src, typename Dst, typename... Args>
forceinline void CastSIMD2(Dst* dest, const Src* src, size_t count, Args&&... args) noexcept
{
    Cast cast(std::forward<Args>(args)...);
    constexpr auto K = std::max(Cast::N, Cast::M);
    for (size_t iter = count / (K * 2); iter > 0; iter--)
    {
        cast(dest + K * 0, src + K * 0);
        cast(dest + K * 1, src + K * 1);
        src += K * 2; dest += K * 2;
    }
    count = count % (K * 2);
    if (count > K)
    {
        cast(dest, src); 
        src += K; dest += K; count -= K;
    }
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
DEFINE_FASTPATH_METHOD(ZExtCopy14, SSSE3)
{
    CastSIMD4<DefaultCast<U8x16, U32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, SSSE3)
{
    CastSIMD4<DefaultCast<U16x8, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, SSSE3)
{
    CastSIMD4<DefaultCast<U32x4, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, SSSE3)
{
    CastSIMD4<DefaultCast<U32x4, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy81, SSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U8x16>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, SSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, SSSE3)
{
    CastSIMD4<DefaultCast<U64x2, U32x4>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41
DEFINE_FASTPATH_METHOD(SExtCopy12, SSSE41)
{
    CastSIMD4<DefaultCast<I8x16, I16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SSSE41)
{
    CastSIMD4<DefaultCast<I8x16, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SSSE41)
{
    CastSIMD4<DefaultCast<I16x8, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SSSE41)
{
    CastSIMD4<DefaultCast<I16x8, I64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SSSE41)
{
    CastSIMD4<DefaultCast<I32x4, I64x2>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(Broadcast2, AVX)
{
    BroadcastSIMD4<U16x16, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Broadcast4, AVX)
{
    BroadcastSIMD4<U32x8, &Func<SIMD128>>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI32F32, I32x8, F32x8, AVX, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I32, F32x8, I32x8, AVX, SIMD128)

DEFINE_FASTPATH_METHOD(CvtF32F64, AVX)
{
    CastSIMD4<DefaultCast<F32x8, F64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF64F32, AVX)
{
    CastSIMD4<DefaultCast<F64x4, F32x8>, &Func<SIMD128>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
DEFINE_FASTPATH_METHOD(ZExtCopy12, AVX2)
{
    CastSIMD4<DefaultCast<U8x32, U16x16>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, AVX2)
{
    CastSIMD4<DefaultCast<U8x32, U32x8>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, AVX2)
{
    CastSIMD4<DefaultCast<U16x16, U32x8>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, AVX2)
{
    CastSIMD4<DefaultCast<U16x16, U64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, AVX2)
{
    CastSIMD4<DefaultCast<U32x8, U64x4>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy12, AVX2)
{
    CastSIMD4<DefaultCast<I8x32, I16x16>, &Func<SSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, AVX2)
{
    CastSIMD4<DefaultCast<I8x32, I32x8>, &Func<SSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, AVX2)
{
    CastSIMD4<DefaultCast<I16x16, I32x8>, &Func<SSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, AVX2)
{
    CastSIMD4<DefaultCast<I16x16, I64x4>, &Func<SSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, AVX2)
{
    CastSIMD4<DefaultCast<I32x8, I64x4>, &Func<SSSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, AVX2)
{
    CastSIMD4<DefaultCast<U16x16, U8x32>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, AVX2)
{
    CastSIMD4<DefaultCast<U32x8, U8x32>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, AVX2)
{
    CastSIMD4<DefaultCast<U32x8, U16x16>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy81, AVX2)
{
    CastSIMD4<DefaultCast<U64x4, U8x32>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, AVX2)
{
    CastSIMD4<DefaultCast<U64x4, U16x16>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, AVX2)
{
    CastSIMD4<DefaultCast<U64x4, U32x8>, &Func<SSSE3>>(dest, src, count);
}

DEFINE_CVTI2FP_SIMD4(CvtI16F32, I16x16, F32x8, AVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtI8F32,  I8x32,  F32x8, AVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU32F32, U32x8,  F32x8, AVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU16F32, U16x16, F32x8, AVX2, SIMD128)
DEFINE_CVTI2FP_SIMD4(CvtU8F32,  U8x32,  F32x8, AVX2, SIMD128)

DEFINE_CVTFP2I_SIMD4(CvtF32I16, F32x8, I16x16, AVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32I8,  F32x8, I8x32,  AVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32U16, F32x8, U16x16, AVX2, SIMD128)
DEFINE_CVTFP2I_SIMD4(CvtF32U8,  F32x8, U8x32,  AVX2, SIMD128)
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


#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41 && COMMON_SIMD_LV < 100)
// seeeeeffffffffff
// seeeeeeeefffffffffffffffffffffff
// s|oooooooo|ffffffffffxxxxxxxxxxxxx
// soooooooofffffff | fffxxxxxxxxxxxxx
struct F1632Cast_SSE41
{
    using Src = uint16_t;
    using Dst = float;
    static constexpr size_t N = 8, M = 8;
    forceinline void operator()(float* dst, const uint16_t* src) const noexcept
    {
        constexpr uint16_t SignMask = 0x8000;
        constexpr uint16_t ExpMask  = 0x7c00;
        constexpr uint16_t FracMask = 0x03ff;
        constexpr uint16_t ExpShift = (127 - 15) << (16 - 1 - 8); // e' = (e - 15) + 127 => e' = e + (127 - 15)
        constexpr uint16_t ExpMax32 = 0x7f80;
        // fexp - 127 = trailing bit count
        // shiter = 10 - trailing bit count = 10 + 127 - fexp
        constexpr uint16_t FexpMax = 10 + 127;
        // minexp = (127 - 14), curexp = (127 - 15) = minexp - 1
        // exp = minexp - shifter = (127 - 15) + 1 - (10 + 127 - fexp) = fexp - 24
        // exp -= 10 - (fexp - 127) => -= 10 + 127 - fexp
        // exp = (127 - 15), exp -= FexpMax - fexp ==> exp = (127 - 15) - (FexpMax - fexp) = fexp - 25
        constexpr uint16_t FexpAdjust = (FexpMax - (127 - 15 + 1)) << 7;

        const auto misc0 = U16x8(SignMask, SignMask, ExpMask, ExpMask, FracMask, FracMask, ExpShift, ExpShift).As<U32x4>();
        const auto misc1 = U16x8(ExpMax32, ExpMax32, FexpMax, FexpMax, FexpAdjust, FexpAdjust, 0, 0).As<U32x4>();
        
        const auto signMask   = misc0.Broadcast<0>().As<U16x8>();
        const auto expMask    = misc0.Broadcast<1>().As<U16x8>();
        const auto fracMask   = misc0.Broadcast<2>().As<U16x8>();
        const auto expShift   = misc0.Broadcast<3>().As<U16x8>();
        const auto expMax32   = misc1.Broadcast<0>().As<U16x8>();
        const auto fexpMax    = misc1.Broadcast<1>().As<U16x8>();
        const auto fexpAdjust = misc1.Broadcast<2>().As<U16x8>();

        const U16x8 dat(src);

        const auto ef = signMask.AndNot(dat);
        const auto e_ = dat.And(expMask);
        const auto f_ = dat.And(fracMask);
        const auto e_normal = e_.ShiftRightLogic<3>().Add(expShift);
        const auto isExpMax = e_.Compare<CompareType::Equal, MaskType::FullEle>(expMask);
        const auto e_nm = e_normal.SelectWith<MaskType::FullEle>(expMax32, isExpMax);
        const auto isExpMin = e_.Compare<CompareType::Equal, MaskType::FullEle>(U16x8::AllZero());
        const auto isDenorm = U16x8(_mm_sign_epi16(isExpMin.Data, ef.Data)); // ef == 0 then become zero
        const auto e_nmz = U16x8(_mm_sign_epi16(e_nm.Data, ef.Data)); // ef == 0 then become zero

        U16x8 e, f;
        IF_LIKELY(isDenorm.IsAllZero())
        { // all normal
            e = e_nmz;
            f = f_;
        }
        else
        { // calc denorm
            const auto f_fp = f_.Cast<F32x4>();
            const auto shufHiMask = _mm_setr_epi8(2, 3, 6, 7, 10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1);
            const auto f_fphi0 = _mm_shuffle_epi8(f_fp[0].As<U32x4>().Data, shufHiMask);
            const auto f_fphi1 = _mm_shuffle_epi8(f_fp[1].As<U32x4>().Data, shufHiMask);
            const auto f_fphi = U16x8(_mm_unpacklo_epi64(f_fphi0, f_fphi1));

            const auto fracShifter = fexpMax.Sub(f_fphi.ShiftRightLogic<7>());
            const auto denormFrac = f_.ShiftLeftLogic<false>(fracShifter).And(fracMask);

            const auto denormExp = f_fphi.Sub(fexpAdjust).And(expMax32);

            e = e_nmz.SelectWith<MaskType::FullEle>(denormExp, isDenorm);
            f = f_.SelectWith<MaskType::FullEle>(denormFrac, isDenorm);
        }
        const auto fhi = f.ShiftRightLogic<3>();
        const auto flo = f.ShiftLeftLogic<13>();

        const auto sign = dat.And(signMask);
        const auto sefh = sign.Or(fhi).Or(e);
        const auto out0 = flo.ZipLo(sefh).As<U32x4>(), out1 = flo.ZipHi(sefh).As<U32x4>();
        out0.Save(reinterpret_cast<uint32_t*>(dst) + 0);
        out1.Save(reinterpret_cast<uint32_t*>(dst) + 4);
    }
};

struct F3216Cast_SSE41
{
    using Src = float;
    using Dst = uint16_t;
    static constexpr size_t N = 8, M = 8;
    forceinline void operator()(uint16_t* dst, const float* src) const noexcept
    {
        constexpr uint16_t SignMask = 0x8000;
        constexpr uint16_t ExpMask  = 0x7f80;
        constexpr uint16_t FracMask = 0x03ff;
        constexpr uint16_t ExpShift = (127 - 15) << (16 - 1 - 8); // e' = (e - 15) + 127 => e' = e + (127 - 15)
        constexpr uint16_t ExpMin   = (127 - 15 - 10) << (16 - 1 - 8);
        constexpr uint16_t ExpMax   = (127 + 15 + 1) << (16 - 1 - 8);
        constexpr uint16_t FracHiBit = 0x0400;

        const auto misc0 = U16x8(SignMask, SignMask, ExpMask, ExpMask, FracMask, FracMask, ExpShift, ExpShift).As<U32x4>();
        const auto misc1 = U16x8(ExpMin, ExpMin, ExpMax, ExpMax, FracHiBit, FracHiBit, 1, 1).As<U32x4>();
        
        const auto signMask   = misc0.Broadcast<0>().As<U16x8>();
        const auto expMask    = misc0.Broadcast<1>().As<U16x8>();
        const auto fracMask   = misc0.Broadcast<2>().As<U16x8>();
        const auto expShift   = misc0.Broadcast<3>().As<U16x8>();
        const auto expMin     = misc1.Broadcast<0>().As<U16x8>();
        const auto expMax     = misc1.Broadcast<1>().As<U16x8>();
        const auto fracHiBit  = misc1.Broadcast<2>().As<U16x8>();
        const auto one        = misc1.Broadcast<3>().As<U16x8>();

        const U32x4 dat0(reinterpret_cast<const uint32_t*>(src)), dat1(reinterpret_cast<const uint32_t*>(src) + 4);

        const auto shufHiMask = _mm_setr_epi8(2, 3, 6, 7, 10, 11, 14, 15,
            0, 1, 4, 5, 8, 9, 12, 13);
        const auto datHi0 = _mm_shuffle_epi8(dat0.Data, shufHiMask);
        const auto datHi1 = _mm_shuffle_epi8(dat1.Data, shufHiMask);
        const auto datHi = U16x8(_mm_unpacklo_epi64(datHi0, datHi1));
        const auto frac0 = _mm_shuffle_epi8(dat0.ShiftLeftLogic<3>().Data, shufHiMask);
        const auto frac1 = _mm_shuffle_epi8(dat1.ShiftLeftLogic<3>().Data, shufHiMask);
        const auto fracPart = U16x8(_mm_unpacklo_epi64(frac0, frac1));
        const auto frac16 = fracPart.And(fracMask);
        const auto fracTrailing = U16x8(_mm_unpackhi_epi64(frac0, frac1));
        const auto isHalfRound = fracTrailing.Compare<CompareType::Equal, MaskType::FullEle>(signMask);
        const auto frac16LastBit = fracPart.ShiftLeftLogic<15>();
        const auto isRoundUpMSB = fracTrailing.SelectWith<MaskType::FullEle>(frac16LastBit, isHalfRound);
        const auto isRoundUpMask = isRoundUpMSB.As<I16x8>().ShiftRightArith<15>().As<U16x8>();
        const auto e_ = datHi.And(expMask); // [0000h~7f80h][denorm,inf]
        const auto e_hicut = e_.Min(expMax); // [0000h~4780h][denorm,2^16=inf]
        const auto e_shifted = e_hicut.Max(expShift).Sub(expShift); // [0000h~07c0h][0=denorm,31=inf]
        const auto e_normal = e_shifted.ShiftLeftLogic<3>(); // [0=denorm,31=inf]
        const auto mapToZero = e_.As<I16x8>().Compare<CompareType::LessThan, MaskType::FullEle>(expMin.As<I16x8>()).As<U16x8>(); // 2^-25, < 0.5*(min-denorm of fp16)
        const auto isNaNInf = e_.Compare<CompareType::Equal, MaskType::FullEle>(expMask); // NaN or Inf
        const auto mapToInf   = e_hicut.Compare<CompareType::Equal, MaskType::FullEle>(expMax); // 2^16(with high cut), become inf exp
        const auto isOverflow = isNaNInf.AndNot(mapToInf); // !isNaNInf && mapToInf
        const auto shouldFracZero = mapToZero.Or(isOverflow);
        const auto f_ = shouldFracZero.AndNot(frac16); // when shouldFracZero == ff, become 0
        const auto needRoundUpMask = shouldFracZero.AndNot(isRoundUpMask);

        const auto dontRequestDenorm = e_hicut.As<I16x8>().Compare<CompareType::GreaterThan, MaskType::FullEle>(expShift.As<I16x8>()).As<U16x8>();
        const auto notDenorm = mapToZero.Or(dontRequestDenorm); // mapToZero || dontRequestDenorm

        U16x8 e, f, r;
        e = e_normal;
        IF_LIKELY(notDenorm.Add(one).IsAllZero())
        { // all normal
            f = f_;
            r = needRoundUpMask;
        }
        else
        { // calc denorm
            const auto e_denormReq = /*shiftAdj*/expShift.Sub(e_hicut).ShiftRightLogic<16 - 9>(); // [f080h~3800h] -> [1e1h~070h], only [00h~09h] valid
            const auto f_shifted1 = f_.Or(fracHiBit).ShiftRightLogic<false>(e_denormReq);
            const auto denormRoundUpBit = f_shifted1.And(one);
            const auto denormRoundUp = U16x8(_mm_sign_epi16(denormRoundUpBit, signMask)); // bit==1 then become ffff, bit==0 then become 0
            f = f_shifted1.ShiftRightLogic<1>().SelectWith<MaskType::FullEle>(f_, notDenorm); // actually shift [0~9]+1
            r = denormRoundUp.SelectWith<MaskType::FullEle>(needRoundUpMask, notDenorm);
        }
        const auto sign = datHi.And(signMask);
        const auto sef = sign.Or(e).Or(f);
        const auto roundUpVal = _mm_abs_epi16(r); // full mask then 1, 0 then 0
        const auto out = sef.Add(roundUpVal);
        out.Save(dst);
    }
};
DEFINE_FASTPATH_METHOD(CvtF16F32, SSSE41)
{
    CastSIMD2<F1632Cast_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, SSSE41)
{
    CastSIMD2<F3216Cast_SSE41, &Func<LOOP>>(dest, src, count);
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
struct F3264CastAVX512
{
    using Src = float;
    using Dst = double;
    static constexpr size_t N = 8, M = 8;
    void operator()(double* __restrict dst, const float* __restrict src) const noexcept
    {
        _mm512_storeu_pd(dst, _mm512_cvtps_pd(_mm256_loadu_ps(src)));
    }
};
struct F6432CastAVX512
{
    using Src = double;
    using Dst = float;
    static constexpr size_t N = 8, M = 8;
    void operator()(float* __restrict dst, const double* __restrict src) const noexcept
    {
        _mm256_storeu_ps(dst, _mm512_cvtpd_ps(_mm512_loadu_pd(src)));
    }
};

struct ZExt14AVX512
{
    using Src = uint8_t;
    using Dst = uint32_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm_loadu_epi8(src +  0);
        const auto dat1 = _mm_loadu_epi8(src + 16);
        const auto dat2 = _mm_loadu_epi8(src + 32);
        const auto dat3 = _mm_loadu_epi8(src + 48);
        const auto out0 = _mm512_cvtepu8_epi32(dat0);
        const auto out1 = _mm512_cvtepu8_epi32(dat1);
        const auto out2 = _mm512_cvtepu8_epi32(dat2);
        const auto out3 = _mm512_cvtepu8_epi32(dat3);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct ZExt24AVX512
{
    using Src = uint16_t;
    using Dst = uint32_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi16(src +  0);
        const auto dat1 = _mm256_loadu_epi16(src + 16);
        const auto out0 = _mm512_cvtepu16_epi32(dat0);
        const auto out1 = _mm512_cvtepu16_epi32(dat1);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct ZExt28AVX512
{
    using Src = uint16_t;
    using Dst = uint64_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint64_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm_loadu_epi16(src +  0);
        const auto dat1 = _mm_loadu_epi16(src +  8);
        const auto dat2 = _mm_loadu_epi16(src + 16);
        const auto dat3 = _mm_loadu_epi16(src + 24);
        const auto out0 = _mm512_cvtepu16_epi64(dat0);
        const auto out1 = _mm512_cvtepu16_epi64(dat1);
        const auto out2 = _mm512_cvtepu16_epi64(dat2);
        const auto out3 = _mm512_cvtepu16_epi64(dat3);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst +  8, out1);
        _mm512_storeu_si512(dst + 16, out2);
        _mm512_storeu_si512(dst + 24, out3);
    }
};
struct ZExt48AVX512
{
    using Src = uint32_t;
    using Dst = uint64_t;
    static constexpr size_t N = 16, M = 16;
    void operator()(uint64_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi32(src + 0);
        const auto dat1 = _mm256_loadu_epi32(src + 8);
        const auto out0 = _mm512_cvtepu32_epi64(dat0);
        const auto out1 = _mm512_cvtepu32_epi64(dat1);
        _mm512_storeu_si512(dst + 0, out0);
        _mm512_storeu_si512(dst + 8, out1);
    }
};

struct SExt14AVX512
{
    using Src = int8_t;
    using Dst = int32_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(int32_t* __restrict dst, const int8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm_loadu_epi8(src +  0);
        const auto dat1 = _mm_loadu_epi8(src + 16);
        const auto dat2 = _mm_loadu_epi8(src + 32);
        const auto dat3 = _mm_loadu_epi8(src + 48);
        const auto out0 = _mm512_cvtepi8_epi32(dat0);
        const auto out1 = _mm512_cvtepi8_epi32(dat1);
        const auto out2 = _mm512_cvtepi8_epi32(dat2);
        const auto out3 = _mm512_cvtepi8_epi32(dat3);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct SExt24AVX512
{
    using Src = int16_t;
    using Dst = int32_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(int32_t* __restrict dst, const int16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi16(src +  0);
        const auto dat1 = _mm256_loadu_epi16(src + 16);
        const auto out0 = _mm512_cvtepi16_epi32(dat0);
        const auto out1 = _mm512_cvtepi16_epi32(dat1);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct SExt28AVX512
{
    using Src = int16_t;
    using Dst = int64_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(int64_t* __restrict dst, const int16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm_loadu_epi16(src +  0);
        const auto dat1 = _mm_loadu_epi16(src +  8);
        const auto dat2 = _mm_loadu_epi16(src + 16);
        const auto dat3 = _mm_loadu_epi16(src + 24);
        const auto out0 = _mm512_cvtepi16_epi64(dat0);
        const auto out1 = _mm512_cvtepi16_epi64(dat1);
        const auto out2 = _mm512_cvtepi16_epi64(dat2);
        const auto out3 = _mm512_cvtepi16_epi64(dat3);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst +  8, out1);
        _mm512_storeu_si512(dst + 16, out2);
        _mm512_storeu_si512(dst + 24, out3);
    }
};
struct SExt48AVX512
{
    using Src = int32_t;
    using Dst = int64_t;
    static constexpr size_t N = 16, M = 16;
    void operator()(int64_t* __restrict dst, const int32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi32(src + 0);
        const auto dat1 = _mm256_loadu_epi32(src + 8);
        const auto out0 = _mm512_cvtepi32_epi64(dat0);
        const auto out1 = _mm512_cvtepi32_epi64(dat1);
        _mm512_storeu_si512(dst + 0, out0);
        _mm512_storeu_si512(dst + 8, out1);
    }
};

struct Trunc41AVX512F
{
    using Src = uint32_t;
    using Dst = uint8_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src +  0);
        const auto dat1 = _mm512_loadu_epi32(src + 16);
        const auto dat2 = _mm512_loadu_epi32(src + 32);
        const auto dat3 = _mm512_loadu_epi32(src + 48);
        const U8x16 out0 = _mm512_cvtepi32_epi8(dat0);
        const U8x16 out1 = _mm512_cvtepi32_epi8(dat1);
        const U8x16 out2 = _mm512_cvtepi32_epi8(dat2);
        const U8x16 out3 = _mm512_cvtepi32_epi8(dat3);
        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
        out3.Save(dst + 48);
    }
};
struct Trunc42AVX512F
{
    using Src = uint32_t;
    using Dst = uint16_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src +  0);
        const auto dat1 = _mm512_loadu_epi32(src + 16);
        const U16x16 out0 = _mm512_cvtepi32_epi16(dat0);
        const U16x16 out1 = _mm512_cvtepi32_epi16(dat1);
        out0.Save(dst +  0);
        out1.Save(dst + 16);
    }
};
struct Trunc82AVX512F
{
    using Src = uint64_t;
    using Dst = uint16_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint16_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi64(src +  0);
        const auto dat1 = _mm512_loadu_epi64(src +  8);
        const auto dat2 = _mm512_loadu_epi64(src + 16);
        const auto dat3 = _mm512_loadu_epi64(src + 24);
        const U16x8 out0 = _mm512_cvtepi64_epi16(dat0);
        const U16x8 out1 = _mm512_cvtepi64_epi16(dat1);
        const U16x8 out2 = _mm512_cvtepi64_epi16(dat2);
        const U16x8 out3 = _mm512_cvtepi64_epi16(dat3);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct Trunc84AVX512F
{
    using Src = uint64_t;
    using Dst = uint32_t;
    static constexpr size_t N = 16, M = 16;
    void operator()(uint32_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi64(src + 0);
        const auto dat1 = _mm512_loadu_epi64(src + 8);
        const U32x8 out0 = _mm512_cvtepi64_epi32(dat0);
        const U32x8 out1 = _mm512_cvtepi64_epi32(dat1);
        out0.Save(dst + 0);
        out1.Save(dst + 8);
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
DEFINE_FASTPATH_METHOD(CvtF32F64, AVX512F)
{
    CastSIMD4<F3264CastAVX512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF64F32, AVX512F)
{
    CastSIMD4<F6432CastAVX512, &Func<AVX>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(ZExtCopy14, AVX512F)
{
    CastSIMD4<ZExt14AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, AVX512F)
{
    CastSIMD4<ZExt24AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, AVX512F)
{
    CastSIMD4<ZExt28AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, AVX512F)
{
    CastSIMD4<ZExt48AVX512, &Func<AVX2>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(SExtCopy14, AVX512F)
{
    CastSIMD4<SExt14AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, AVX512F)
{
    CastSIMD4<SExt24AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, AVX512F)
{
    CastSIMD4<SExt28AVX512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, AVX512F)
{
    CastSIMD4<SExt48AVX512, &Func<AVX2>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(TruncCopy41, AVX512F)
{
    CastSIMD4<Trunc41AVX512F, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, AVX512F)
{
    CastSIMD4<Trunc42AVX512F, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, AVX512F)
{
    CastSIMD4<Trunc82AVX512F, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, AVX512F)
{
    CastSIMD4<Trunc84AVX512F, &Func<AVX2>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320
struct ZExt12AVX512BW
{
    using Src = uint8_t;
    using Dst = uint16_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi8(src +  0);
        const auto dat1 = _mm256_loadu_epi8(src + 32);
        const auto out0 = _mm512_cvtepu8_epi16(dat0);
        const auto out1 = _mm512_cvtepu8_epi16(dat1);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 32, out1);
    }
};
struct SExt12AVX512BW
{
    using Src = int8_t;
    using Dst = int16_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(int16_t* __restrict dst, const int8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm256_loadu_epi8(src +  0);
        const auto dat1 = _mm256_loadu_epi8(src + 32);
        const auto out0 = _mm512_cvtepi8_epi16(dat0);
        const auto out1 = _mm512_cvtepi8_epi16(dat1);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 32, out1);
    }
};
struct Trunc21AVX512BW
{
    using Src = uint16_t;
    using Dst = uint8_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const U8x32 out0 = _mm512_cvtepi16_epi8(dat0);
        const U8x32 out1 = _mm512_cvtepi16_epi8(dat1);
        out0.Save(dst +  0);
        out1.Save(dst + 32);
    }
};

DEFINE_FASTPATH_METHOD(SExtCopy12, AVX512BW)
{
    CastSIMD4<SExt12AVX512BW, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy12, AVX512BW)
{
    CastSIMD4<ZExt12AVX512BW, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy21, AVX512BW)
{
    CastSIMD4<Trunc21AVX512BW, &Func<AVX2>>(dest, src, count);
}


#if defined(__AVX512VBMI__) || COMMON_COMPILER_MSVC
#   pragma message("Compiling CopyEx with AVX512-VBMI")
struct Trunc21AVX512VBMI
{
    using Src = uint16_t;
    using Dst = uint8_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0); // 00~1f
        const auto dat1 = _mm512_loadu_epi16(src + 32); // 20~3f
        const auto shufMask = _mm512_set_epi8(
            126, 124, 122, 120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96,
             94,  92,  90,  88,  86,  84,  82,  80,  78,  76,  74,  72,  70,  68, 66, 64,
             62,  60,  58,  56,  54,  52,  50,  48,  46,  44,  42,  40,  38,  36, 34, 32,
             30,  28,  26,  24,  22,  20,  18,  16,  14,  12,  10,   8,   6,   4,  2,  0);
        const auto out = _mm512_permutex2var_epi8(dat0, shufMask, dat1);
        _mm512_storeu_si512(dst, out);
    }
};
struct Trunc41AVX512VBMI
{
    using Src = uint32_t;
    using Dst = uint8_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src +  0); // 00~0f
        const auto dat1 = _mm512_loadu_epi32(src + 16); // 10~1f
        const auto dat2 = _mm512_loadu_epi32(src + 32); // 20~2f
        const auto dat3 = _mm512_loadu_epi32(src + 48); // 30~3f
        const auto shufMask = _mm512_set_epi8(
            124, 120, 116, 112, 108, 104, 100, 96, 92,  88,  84,  80,  76,  72,  68, 64,
             60,  56,  52,  48,  44,  40,  36, 32, 28,  24,  20,  16,  12,   8,   4,  0,
            124, 120, 116, 112, 108, 104, 100, 96, 92,  88,  84,  80,  76,  72,  68, 64,
             60,  56,  52,  48,  44,  40,  36, 32, 28,  24,  20,  16,  12,   8,   4,  0);
        const auto out0 = _mm512_permutex2var_epi8(dat0, shufMask, dat1);
        const auto out1 = _mm512_permutex2var_epi8(dat2, shufMask, dat3);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst +  0), _mm512_castsi512_si256(out0));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + 32), _mm512_castsi512_si256(out1));
    }
};
struct Trunc42AVX512VBMI
{
    using Src = uint32_t;
    using Dst = uint16_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src +  0); // 00~0f
        const auto dat1 = _mm512_loadu_epi32(src + 16); // 10~1f
        const auto shufMask = _mm512_set_epi16(
            62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32,
            30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10,  8,  6,  4,  2,  0);
        const auto out = _mm512_permutex2var_epi16(dat0, shufMask, dat1);
        _mm512_storeu_si512(dst, out);
    }
};
struct Trunc82AVX512VBMI
{
    using Src = uint64_t;
    using Dst = uint16_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint16_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi64(src +  0); // 00~07
        const auto dat1 = _mm512_loadu_epi64(src +  8); // 08~0f
        const auto dat2 = _mm512_loadu_epi64(src + 16); // 10~17
        const auto dat3 = _mm512_loadu_epi64(src + 24); // 18~1f
        const auto shufMask = _mm512_set_epi16(
            60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0,
            60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0);
        const auto out0 = _mm512_permutex2var_epi16(dat0, shufMask, dat1);
        const auto out1 = _mm512_permutex2var_epi16(dat2, shufMask, dat3);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst +  0), _mm512_castsi512_si256(out0));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + 16), _mm512_castsi512_si256(out1));
    }
};
struct Trunc84AVX512VBMI
{
    using Src = uint64_t;
    using Dst = uint32_t;
    static constexpr size_t N = 16, M = 16;
    void operator()(uint32_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi64(src + 0); // 00~07
        const auto dat1 = _mm512_loadu_epi64(src + 8); // 08~0f
        const auto shufMask = _mm512_set_epi32(30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0);
        const auto out = _mm512_permutex2var_epi32(dat0, shufMask, dat1);
        _mm512_storeu_si512(dst, out);
    }
};
DEFINE_FASTPATH_METHOD(TruncCopy21, AVX512VBMI)
{
    CastSIMD4<Trunc21AVX512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, AVX512VBMI)
{
    CastSIMD4<Trunc41AVX512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, AVX512VBMI)
{
    CastSIMD4<Trunc42AVX512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, AVX512VBMI)
{
    CastSIMD4<Trunc82AVX512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, AVX512VBMI)
{
    CastSIMD4<Trunc84AVX512VBMI, &Func<AVX2>>(dest, src, count);
}
#endif


#if defined(__AVX512VBMI2__) || COMMON_COMPILER_MSVC
#   pragma message("Compiling CopyEx with AVX512-VBMI2")
struct ZExt12AVX512VBMI2
{
    using Src = uint8_t;
    using Dst = uint16_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x5555555555555555u);
        const auto outLo = _mm512_maskz_expandloadu_epi8(mask, src);
        const auto outHi = _mm512_maskz_expandloadu_epi8(mask, src + 32);
        _mm512_storeu_si512(dst +  0, outLo);
        _mm512_storeu_si512(dst + 32, outHi);
    }
};
struct ZExt14AVX512VBMI2
{
    using Src = uint8_t;
    using Dst = uint32_t;
    static constexpr size_t N = 64, M = 64;
    void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x1111111111111111u);
        const auto out0 = _mm512_maskz_expandloadu_epi8(mask, src + 0);
        const auto out1 = _mm512_maskz_expandloadu_epi8(mask, src + 16);
        const auto out2 = _mm512_maskz_expandloadu_epi8(mask, src + 32);
        const auto out3 = _mm512_maskz_expandloadu_epi8(mask, src + 48);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct ZExt24AVX512VBMI2
{
    using Src = uint16_t;
    using Dst = uint32_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu32_mask32(0x55555555u);
        const auto outLo = _mm512_maskz_expandloadu_epi16(mask, src);
        const auto outHi = _mm512_maskz_expandloadu_epi16(mask, src + 16);
        _mm512_storeu_si512(dst +  0, outLo);
        _mm512_storeu_si512(dst + 16, outHi);
    }
};
struct ZExt28AVX512VBMI2
{
    using Src = uint16_t;
    using Dst = uint64_t;
    static constexpr size_t N = 32, M = 32;
    void operator()(uint64_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu32_mask32(0x11111111u);
        const auto out0 = _mm512_maskz_expandloadu_epi16(mask, src +  0);
        const auto out1 = _mm512_maskz_expandloadu_epi16(mask, src +  8);
        const auto out2 = _mm512_maskz_expandloadu_epi16(mask, src + 16);
        const auto out3 = _mm512_maskz_expandloadu_epi16(mask, src + 24);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst +  8, out1);
        _mm512_storeu_si512(dst + 16, out2);
        _mm512_storeu_si512(dst + 24, out3);
    }
};
struct ZExt48AVX512VBMI2
{
    using Src = uint32_t;
    using Dst = uint64_t;
    static constexpr size_t N = 16, M = 16;
    void operator()(uint64_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu32_mask32(0x33333333u);
        const auto outLo = _mm512_maskz_expandloadu_epi16(mask, src);
        const auto outHi = _mm512_maskz_expandloadu_epi16(mask, src + 8);
        _mm512_storeu_si512(dst + 0, outLo);
        _mm512_storeu_si512(dst + 8, outHi);
    }
};

DEFINE_FASTPATH_METHOD(ZExtCopy12, AVX512VBMI2)
{
    CastSIMD4<ZExt12AVX512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy14, AVX512VBMI2)
{
    CastSIMD4<ZExt14AVX512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy24, AVX512VBMI2)
{
    CastSIMD4<ZExt24AVX512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy28, AVX512VBMI2)
{
    CastSIMD4<ZExt28AVX512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(ZExtCopy48, AVX512VBMI2)
{
    CastSIMD4<ZExt48AVX512VBMI2, &Func<AVX2>>(dest, src, count);
}
#endif

#endif


}