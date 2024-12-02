
#include "FastPathCategory.h"
#include "CopyEx.h"
#include "common/simd/SIMD.hpp"

#define HALF_ENABLE_F16C_INTRINSICS 0 // only use half as fallback
#include "3rdParty/half/half.hpp"

static_assert(::common::detail::is_little_endian, "unsupported std::byte order (non little endian)");


#define SwapRegionInfo  (void)(uint8_t* srcA, uint8_t* srcB, size_t count)
#define Reverse1Info    (void)(uint8_t * dat, size_t count)
#define Reverse2Info    (void)(uint16_t* dat, size_t count)
#define Reverse3Info    (void)(uint8_t * dat, size_t count)
#define Reverse4Info    (void)(uint32_t* dat, size_t count)
#define Reverse8Info    (void)(uint64_t* dat, size_t count)
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
#define CvtF16F32Info   (void)(float * dest, const ::common::fp16_t* src, size_t count)
#define CvtF32F16Info   (void)(::common::fp16_t* dest, const float * src, size_t count)
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
    SwapRegion, Reverse1, Reverse2, Reverse3, Reverse4, Reverse8,
    Broadcast2, Broadcast4,
    ZExtCopy12, ZExtCopy14, ZExtCopy24, ZExtCopy28, ZExtCopy48,
    SExtCopy12, SExtCopy14, SExtCopy24, SExtCopy28, SExtCopy48,
    TruncCopy21, TruncCopy41, TruncCopy42, TruncCopy81, TruncCopy82, TruncCopy84,
    CvtI32F32, CvtI16F32, CvtI8F32, CvtU32F32, CvtU16F32, CvtU8F32,
    CvtF32I32, CvtF32I16, CvtF32I8, CvtF32U16, CvtF32U8,
    CvtF16F32, CvtF32F16, CvtF32F64, CvtF64F32)


DEFINE_FASTPATH_METHOD(SwapRegion, LOOP)
{
    while (count >= 8)
    {
        const auto ptrA = reinterpret_cast<uint64_t*>(srcA), ptrB = reinterpret_cast<uint64_t*>(srcB);
        const auto datA = *ptrA, datB = *ptrB;
        *ptrA = datB; *ptrB = datA;
        srcA += 8; srcB += 8; count -= 8;
    }
    CM_ASSUME(count < 8);
    if (count)
    {
        uint8_t tmp[8] = {};
        memcpy_s(tmp, 8, srcA, count);
        memcpy_s(srcA, count, srcB, count);
        memcpy_s(srcB, count, tmp, count);
    }
}
template<typename P, size_t M, typename T>
static void ReverseBatch(T* dat, size_t cnt)
{
    constexpr auto Step = sizeof(decltype(P::Load(dat))) / sizeof(T);
    constexpr auto Batch = Step / M;
    auto head = dat, tail = dat + cnt * M;
    size_t count = cnt / Batch;
    while (count >= 4)
    {
        tail -= 2 * Step;
        const auto dat0 = P::Load(head), dat1 = P::Load(head + Step), dat2 = P::Load(tail), dat3 = P::Load(tail + Step);
        const auto mid0 = P::Rev(dat0), mid1 = P::Rev(dat1), mid2 = P::Rev(dat2), mid3 = P::Rev(dat3);
        P::Save(mid3, head);
        P::Save(mid2, head + Step);
        P::Save(mid1, tail);
        P::Save(mid0, tail + Step);
        head += 2 * Step; count -= 4;
    }
    if (count >= 2)
    {
        tail -= Step;
        const auto mid = P::Rev(P::Load(tail));
        P::Save(P::Rev(P::Load(head)), tail);
        P::Save(mid, head);
        head += Step; count -= 2;
    }
    if (const auto left = cnt % (2 * Batch); left)
    {
        const auto processed = (cnt - left) / 2;
        P::Next(dat + processed * M, left);
    }
}
template<typename T>
struct RevBy64
{
    static forceinline uint64_t Load(const T* ptr) noexcept { return *reinterpret_cast<const uint64_t*>(ptr); }
    static forceinline void Save(uint64_t val, T* ptr) noexcept { *reinterpret_cast<uint64_t*>(ptr) = val; }
};
template<typename T>
struct RevByVec
{
    using E = typename T::EleType;
    static forceinline T Load(const E* ptr) noexcept { return T(ptr); }
    static forceinline void Save(const T& val, E* ptr) noexcept { val.Save(ptr); }
};
DEFINE_FASTPATH_METHOD(Reverse1, LOOP)
{
    struct Rev1 : public RevBy64<uint8_t>
    {
        static forceinline uint64_t Rev(uint64_t val) noexcept
        {
#if COMMON_COMPILER_MSVC
            return _byteswap_uint64(val);
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
            return __builtin_bswap64(val);
#endif
        }
        static forceinline void Next(uint8_t* dat, size_t count) noexcept
        {
            for (size_t i = 0, j = count - 1; i + 1 <= j; ++i, --j)
                std::swap(dat[i], dat[j]);
        }
    };
    ReverseBatch<Rev1, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse2, LOOP)
{
    struct Rev2 : public RevBy64<uint16_t>
    {
        static forceinline uint64_t Rev(uint64_t val) noexcept
        {
            const auto new0 = val >> 48;
            const auto new1 = (val >> 16) & 0xffff0000u;
            const auto new2 = (val << 16) & 0xffff00000000u;
            const auto new3 = val << 48;
            return new0 | new1 | new2 | new3;
        }
        static forceinline void Next(uint16_t* dat, size_t count) noexcept
        {
            for (size_t i = 0, j = count - 1; i + 1 <= j; ++i, --j)
                std::swap(dat[i], dat[j]);
        }
    };
    ReverseBatch<Rev2, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse3, LOOP)
{
    for (size_t i = 0, j = 3 * (count - 1); i + 3 <= j; i += 3, j -= 3)
    {
        std::swap(dat[i + 0], dat[j + 0]);
        std::swap(dat[i + 1], dat[j + 1]);
        std::swap(dat[i + 2], dat[j + 2]);
    }
}
DEFINE_FASTPATH_METHOD(Reverse4, LOOP)
{
    for (size_t i = 0, j = count - 1; i + 1 <= j; ++i, --j)
        std::swap(dat[i], dat[j]);
}
DEFINE_FASTPATH_METHOD(Reverse8, LOOP)
{
    for (size_t i = 0, j = count - 1; i + 1 <= j; ++i, --j)
        std::swap(dat[i], dat[j]);
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


template<typename T, auto F>
forceinline void SwapX8(uint8_t* __restrict srcA, uint8_t* __restrict srcB, size_t count) noexcept // assume 16 SIMD register
{
    constexpr auto N = T::Count;
#define SWAP_VEC do { const T datA(srcA), datB(srcB); datA.Save(srcB); datB.Save(srcA); srcA += N, srcB += N; } while(0)
    while (count >= N * 8)
    {
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        SWAP_VEC;
        count -= N * 8;
    }
    switch (count / (N * 8))
    {
    case 7: SWAP_VEC;
        [[fallthrough]];
    case 6: SWAP_VEC;
        [[fallthrough]];
    case 5: SWAP_VEC;
        [[fallthrough]];
    case 4: SWAP_VEC;
        [[fallthrough]];
    case 3: SWAP_VEC;
        [[fallthrough]];
    case 2: SWAP_VEC;
        [[fallthrough]];
    case 1: SWAP_VEC;
        [[fallthrough]];
    default: break;
    }
#undef SWAP_VEC
    count = count % (N * 8);
    CM_ASSUME(count < (N * 8));
    if (count)
        F(srcA, srcB, count);
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

DEFINE_FASTPATH_METHOD(SwapRegion, SIMD128)
{
    SwapX8<U8x16, &Func<LOOP>>(srcA, srcB, count);
}
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
struct Trunc16_8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u8(reinterpret_cast<const uint8_t*>(src));
        U8x16 out(dat.val[0]);
        out.Save(dst);
    }
};
struct Trunc32_8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        U8x16 out(dat.val[0]);
        out.Save(dst);
    }
};
struct Trunc32_16_NEON
{
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u16(reinterpret_cast<const uint16_t*>(src));
        U16x8 out(dat.val[0]);
        out.Save(dst);
    }
};
struct Trunc64_16_NEON
{
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u16(reinterpret_cast<const uint16_t*>(src));
        U16x8 out(dat.val[0]);
        out.Save(dst);
    }
};
struct Trunc64_32_NEON
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint32_t* __restrict dst, const uint64_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u32(reinterpret_cast<const uint32_t*>(src));
        U32x4 out(dat.val[0]);
        out.Save(dst);
    }
};
DEFINE_FASTPATH_METHOD(TruncCopy21, NEON)
{
    CastSIMD4<Trunc16_8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy41, NEON)
{
    CastSIMD4<Trunc32_8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy42, NEON)
{
    CastSIMD4<Trunc32_16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy82, NEON)
{
    CastSIMD4<Trunc64_16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(TruncCopy84, NEON)
{
    CastSIMD4<Trunc64_32_NEON, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 200
DEFINE_FASTPATH_METHOD(Reverse3, SIMD128)
{
    struct Rev3
    {
        static forceinline Pack<U8x16, 3> Load(const uint8_t* ptr) noexcept { return { U8x16(ptr), U8x16(ptr + 16), U8x16(ptr + 32) }; }
        static forceinline void Save(const Pack<U8x16, 3>& val, uint8_t* ptr) noexcept { val[0].Save(ptr); val[1].Save(ptr + 16); val[2].Save(ptr + 32); }
        static forceinline Pack<U8x16, 3> Rev(const Pack<U8x16, 3>& val) noexcept
        {
            // 000 111 222 333 444 5
            // 55 666 777 888 999 aa
            // a bbb ccc ddd eee fff

            alignas(16) constexpr uint8_t idxes0[] = { 17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2 };
            const auto shuffle0 = vld1q_u8(idxes0);
            alignas(16) constexpr uint8_t idxes1[] = { 31, 32, 27, 28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16 };
            const auto shuffle1 = vld1q_u8(idxes1);
            alignas(16) constexpr uint8_t idxes2[] = { 29, 30, 31, 26, 27, 28, 23, 24, 25, 20, 21, 22, 17, 18, 19, 14 };
            const auto shuffle2 = vld1q_u8(idxes2);

            uint8x16x2_t mid01, mid12;
            mid01.val[0] = val[0]; mid01.val[1] = val[1];
            mid12.val[0] = val[1]; mid12.val[1] = val[2];
            const auto out0 = vqtbl2q_u8(mid12, shuffle2);
            const auto out2 = vqtbl2q_u8(mid01, shuffle0);
            uint8x16x3_t mid012;
            mid012.val[0] = val[0]; mid012.val[1] = val[1]; mid012.val[2] = val[2];
            const auto out1 = vqtbl3q_u8(mid012, shuffle1);

            return { out0, out1, out2 };
        }
        static forceinline void Next(uint8_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev3, 3>(dat, count);
}
#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 10)
# if COMMON_ARCH_X86
#   define ALGO SSSE3
# else
#   define ALGO SIMD128
# endif
DEFINE_FASTPATH_METHOD(Reverse1, ALGO)
{
    struct Rev1 : public RevByVec<U8x16>
    {
        static forceinline U8x16 Rev(const U8x16& val) noexcept
        {
            return val.Shuffle<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>();
        }
        static forceinline void Next(uint8_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev1, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse2, ALGO)
{
    struct Rev2 : public RevByVec<U16x8>
    {
        static forceinline U16x8 Rev(const U16x8& val) noexcept
        {
            return val.Shuffle<7, 6, 5, 4, 3, 2, 1, 0>();
        }
        static forceinline void Next(uint16_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev2, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse4, ALGO)
{
    struct Rev4 : public RevByVec<U32x4>
    {
        static forceinline U32x4 Rev(const U32x4& val) noexcept
        {
            return val.Shuffle<3, 2, 1, 0>();
        }
        static forceinline void Next(uint32_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev4, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse8, ALGO)
{
    struct Rev8 : public RevByVec<U64x2>
    {
        static forceinline U64x2 Rev(const U64x2& val) noexcept
        {
            return val.Shuffle<1, 0>();
        }
        static forceinline void Next(uint64_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev8, 1>(dat, count);
}
# undef ALGO
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31
DEFINE_FASTPATH_METHOD(Reverse3, SSSE3)
{
    struct Rev3
    {
        static forceinline Pack<U8x16, 3> Load(const uint8_t* ptr) noexcept { return { U8x16(ptr), U8x16(ptr + 16), U8x16(ptr + 32) }; }
        static forceinline void Save(const Pack<U8x16, 3>& val, uint8_t* ptr) noexcept { val[0].Save(ptr); val[1].Save(ptr + 16); val[2].Save(ptr + 32); }
        static forceinline Pack<U8x16, 3> Rev(const Pack<U8x16, 3>& val) noexcept
        {
            // 000 111 222 333 444 5
            // 55 666 777 888 999 aa
            // a bbb ccc ddd eee fff
            const auto mid2 = val[0].Shuffle<12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2, 0>(); // 444 333 222 111 000 x
            const auto mid0 = val[2].Shuffle<15, 13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3>(); // x fff eee ddd ccc bbb

            const auto merge01 = val[0].MoveToLoWith<15>(val[1]); // 555 666 777 888 999 a
            const auto merge12 = val[1].MoveToLoWith< 1>(val[2]); // 5 666 777 888 999 aaa

            const U8x16 shuffle01 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2);
            const auto mid12 = merge01.Shuffle(shuffle01); // - --- --- --- --- 555
            const U8x16 shuffle12 = _mm_setr_epi8(13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3, -1);
            const auto mid01 = merge12.Shuffle(shuffle12); // aaa 999 888 777 666 -

            const auto out1 = mid01.MoveToLo<1>().Or(mid12.MoveToHi<1>()); // aa 999 888 777 666 55
            // x 999 888 777 666 555 444 333 222 111 000 x
            const auto out2 = mid12.MoveToLoWith<15>(mid2); // 5 444 333 222 111 000
            // x fff eee ddd ccc bbb aaa 999 888 777 666 x
            const auto out0 = mid0.MoveToLoWith< 1>(mid01); // fff eee ddd ccc bbb a

            return { out0, out1, out2 };
        }
        static forceinline void Next(uint8_t* dat, size_t count) noexcept
        {
            Func<LOOP>(dat, count);
        }
    };
    ReverseBatch<Rev3, 3>(dat, count);
}
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
DEFINE_FASTPATH_METHOD(SExtCopy12, SSE41)
{
    CastSIMD4<DefaultCast<I8x16, I16x8>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, SSE41)
{
    CastSIMD4<DefaultCast<I8x16, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, SSE41)
{
    CastSIMD4<DefaultCast<I16x8, I32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, SSE41)
{
    CastSIMD4<DefaultCast<I16x8, I64x2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, SSE41)
{
    CastSIMD4<DefaultCast<I32x4, I64x2>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100
DEFINE_FASTPATH_METHOD(SwapRegion, AVX)
{
    SwapX8<U8x32, &Func<SIMD128>>(srcA, srcB, count);
}
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

DEFINE_FASTPATH_METHOD(Reverse1, AVX2)
{
    struct Rev1 : public RevByVec<U8x32>
    {
        static forceinline U8x32 Rev(const U8x32& val) noexcept
        {
            return val.ShuffleLane<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>().PermuteLane<1, 0>();
        }
        static forceinline void Next(uint8_t* dat, size_t count) noexcept
        {
            Func<SSSE3>(dat, count);
        }
    };
    ReverseBatch<Rev1, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse2, AVX2)
{
    struct Rev2 : public RevByVec<U16x16>
    {
        static forceinline U16x16 Rev(const U16x16& val) noexcept
        {
            return val.ShuffleLane<7, 6, 5, 4, 3, 2, 1, 0>().PermuteLane<1, 0>();
        }
        static forceinline void Next(uint16_t* dat, size_t count) noexcept
        {
            Func<SSSE3>(dat, count);
        }
    };
    ReverseBatch<Rev2, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse4, AVX2)
{
    struct Rev4 : public RevByVec<U32x8>
    {
        static forceinline U32x8 Rev(const U32x8& val) noexcept
        {
            return val.Shuffle<7, 6, 5, 4, 3, 2, 1, 0>();
        }
        static forceinline void Next(uint32_t* dat, size_t count) noexcept
        {
            Func<SSSE3>(dat, count);
        }
    };
    ReverseBatch<Rev4, 1>(dat, count);
}
DEFINE_FASTPATH_METHOD(Reverse8, AVX2)
{
    struct Rev8 : public RevByVec<U64x4>
    {
        static forceinline U64x4 Rev(const U64x4& val) noexcept
        {
            return val.Shuffle<3, 2, 1, 0>();
        }
        static forceinline void Next(uint64_t* dat, size_t count) noexcept
        {
            Func<SSSE3>(dat, count);
        }
    };
    ReverseBatch<Rev8, 1>(dat, count);
}

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
    CastSIMD4<DefaultCast<I8x32, I16x16>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy14, AVX2)
{
    CastSIMD4<DefaultCast<I8x32, I32x8>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy24, AVX2)
{
    CastSIMD4<DefaultCast<I16x16, I32x8>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy28, AVX2)
{
    CastSIMD4<DefaultCast<I16x16, I64x4>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(SExtCopy48, AVX2)
{
    CastSIMD4<DefaultCast<I32x8, I64x4>, &Func<SSE41>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(CvtF16F32, F16C)
{
    CastSIMD4<DefaultCast<F16x8, F32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, F16C)
{
    CastSIMD4<DefaultCast<F32x4, F16x8>, &Func<LOOP>>(dest, src, count);
}
#endif


#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41 && COMMON_SIMD_LV < 100)
// seeeeeffffffffff
// seeeeeeeefffffffffffffffffffffff
// s|oooooooo|ffffffffffxxxxxxxxxxxxx
// soooooooofffffff | fffxxxxxxxxxxxxx
DEFINE_FASTPATH_METHOD(CvtF16F32, SSE41)
{
    CastSIMD2<DefaultCast<F16x8, F32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, SSE41)
{
    CastSIMD2<DefaultCast<F32x4, F16x8>, &Func<LOOP>>(dest, src, count);
}
#endif


#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 30
DEFINE_FASTPATH_METHOD(CvtF16F32, SIMD128)
{
    CastSIMD4<DefaultCast<F16x8, F32x4>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(CvtF32F16, SIMD128)
{
    CastSIMD4<DefaultCast<F32x4, F16x8>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 310 && (!COMMON_COMPILER_MSVC || COMMON_MSVC_VER >= 191000)
struct F1632CastAVX512
{
    static constexpr size_t N = 16, M = 16;
    void operator()(float* dst, const ::common::fp16_t* src) const noexcept
    {
        _mm512_storeu_ps(dst, _mm512_cvtph_ps(_mm256_loadu_epi16(src)));
    }
};
struct F3216CastAVX512
{
    static constexpr size_t N = 16, M = 16;
    void operator()(::common::fp16_t* dst, const float* src) const noexcept
    {
        _mm256_storeu_epi16(dst, _mm512_cvtps_ph(_mm512_loadu_ps(src), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    }
};
struct F3264CastAVX512
{
    static constexpr size_t N = 8, M = 8;
    void operator()(double* __restrict dst, const float* __restrict src) const noexcept
    {
        _mm512_storeu_pd(dst, _mm512_cvtps_pd(_mm256_loadu_ps(src)));
    }
};
struct F6432CastAVX512
{
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
        const auto dat0 = U8x16(src +  0);
        const auto dat1 = U8x16(src + 16);
        const auto dat2 = U8x16(src + 32);
        const auto dat3 = U8x16(src + 48);
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
        const auto dat0 = U16x16(src +  0);
        const auto dat1 = U16x16(src + 16);
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
        const auto dat0 = U16x8(src +  0);
        const auto dat1 = U16x8(src +  8);
        const auto dat2 = U16x8(src + 16);
        const auto dat3 = U16x8(src + 24);
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
        const auto dat0 = U32x8(src + 0);
        const auto dat1 = U32x8(src + 8);
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
        const auto dat0 = I8x16(src +  0);
        const auto dat1 = I8x16(src + 16);
        const auto dat2 = I8x16(src + 32);
        const auto dat3 = I8x16(src + 48);
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
        const auto dat0 = I16x16(src +  0);
        const auto dat1 = I16x16(src + 16);
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
        const auto dat0 = I16x8(src +  0);
        const auto dat1 = I16x8(src +  8);
        const auto dat2 = I16x8(src + 16);
        const auto dat3 = I16x8(src + 24);
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
        const auto dat0 = I32x8(src + 0);
        const auto dat1 = I32x8(src + 8);
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
#if COMMON_COMPILER_MSVC // WA for https://developercommunity.visualstudio.com/t/Incorrect-AVX512-assembly-generated-with/10772485
        std::atomic_signal_fence(std::memory_order_acquire);
#endif
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
#if COMMON_COMPILER_MSVC // WA for https://developercommunity.visualstudio.com/t/Incorrect-AVX512-assembly-generated-with/10772485
        std::atomic_signal_fence(std::memory_order_acquire);
#endif
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
#if COMMON_COMPILER_MSVC // WA for https://developercommunity.visualstudio.com/t/Incorrect-AVX512-assembly-generated-with/10772485
        std::atomic_signal_fence(std::memory_order_acquire);
#endif
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