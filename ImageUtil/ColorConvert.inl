
#include "FastPathCategory.h"
#include "ColorConvert.h"
#include "common/simd/SIMD.hpp"


#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-function"
#   pragma clang diagnostic ignored "-Wc99-extensions"
#elif COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4505)
#endif
#ifdef IMGU_FASTPATH_STB
#   define STB_IMAGE_RESIZE2_IMPLEMENTATION
#   define STB_IMAGE_RESIZE_STATIC
#   include "stb/stb_image_resize2.h"
#endif
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

#include <bit>
#include <cassert>
#include <atomic>
#include <cmath>

#define DoResizeInfo (void)(const ::xziar::img::STBResize::ResizeInfo& info)

#define G8ToGA8Info         (void)(uint16_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define G8ToRGB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define G8ToRGBA8Info       (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define GA8ToRGB8Info       (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define GA8ToRGBA8Info      (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define G16ToGA16Info       (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count, uint16_t alpha)
#define G16ToRGB16Info      (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define G16ToRGBA16Info     (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count, uint16_t alpha)
#define GA16ToRGB16Info     (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define GA16ToRGBA16Info    (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define GfToGAfInfo         (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define GfToRGBfInfo        (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define GfToRGBAfInfo       (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define GAfToRGBfInfo       (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define GAfToRGBAfInfo      (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGB8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define BGR8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define RGBA8ToRGB8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToBGR8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB16ToRGBA16Info   (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count, uint16_t alpha)
#define BGR16ToRGBA16Info   (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count, uint16_t alpha)
#define RGBA16ToRGB16Info   (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGBA16ToBGR16Info   (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGBfToRGBAfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define BGRfToRGBAfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define RGBAfToRGBfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBGRfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGB8ToBGR8Info      (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGBA8ToBGRA8Info    (void)(uint32_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB16ToBGR16Info    (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGBA16ToBGRA16Info  (void)(uint16_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGBfToBGRfInfo      (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBGRAfInfo    (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGB8ToR8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToG8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB16ToR16Info      (void)(uint16_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB16ToG16Info      (void)(uint16_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB16ToB16Info      (void)(uint16_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGBAfToRfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToGfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToAfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToRfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToGfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToBfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define Extract8x2Info      (void)(uint8_t* const * __restrict dest, const uint16_t* __restrict src, size_t count)
#define Extract8x3Info      (void)(uint8_t* const * __restrict dest, const uint8_t*  __restrict src, size_t count)
#define Extract8x4Info      (void)(uint8_t* const * __restrict dest, const uint32_t* __restrict src, size_t count)
#define Extract16x2Info     (void)(uint16_t* const * __restrict dest, const uint16_t* __restrict src, size_t count)
#define Extract16x3Info     (void)(uint16_t* const * __restrict dest, const uint16_t* __restrict src, size_t count)
#define Extract16x4Info     (void)(uint16_t* const * __restrict dest, const uint16_t* __restrict src, size_t count)
#define Extract32x2Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Extract32x3Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Extract32x4Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Combine8x2Info      (void)(uint16_t* __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine8x3Info      (void)(uint8_t*  __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine8x4Info      (void)(uint32_t* __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine16x2Info     (void)(uint16_t* __restrict dest, const uint16_t* const * __restrict src, size_t count)
#define Combine16x3Info     (void)(uint16_t* __restrict dest, const uint16_t* const * __restrict src, size_t count)
#define Combine16x4Info     (void)(uint16_t* __restrict dest, const uint16_t* const * __restrict src, size_t count)
#define Combine32x2Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
#define Combine32x3Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
#define Combine32x4Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
#define G8ToG16Info         (void)(uint16_t* __restrict dest, const uint8_t * __restrict src, size_t count)
#define G16ToG8Info         (void)(uint8_t * __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB555ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR555ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB555ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR555ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB5551ToRGBA8Info  (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR5551ToRGBA8Info  (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB565ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR565ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB565ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR565ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB10ToRGBfInfo     (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)
#define BGR10ToRGBfInfo     (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)
#define RGB10ToRGBAfInfo    (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)
#define BGR10ToRGBAfInfo    (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)
#define RGB10A2ToRGBAfInfo  (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)
#define BGR10A2ToRGBAfInfo  (void)(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal)


#define RGB8ToYCbCr8FastInfo    (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count, uint8_t mval)
#define RGB8ToYCbCr8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count, uint8_t mval)
#define YCbCr8ToRGB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count, uint8_t mval)


namespace xziar::img
{
extern std::array<std::array<  float,  9>, 16> RGB8ToYCC8LUTF32;
extern std::array<std::array<int16_t,  9>, 16> RGB8ToYCC8LUTI16;
//extern std::array<std::array<int16_t, 9>, 16> RGB8ToYCC8LUTI16_15;
extern std::array<std::array< int8_t, 16>, 16> RGB8ToYCC8LUTI8x4;
extern std::array<std::array<uint8_t, 16>, 16> RGB8ToYCC8LUTU8x4;
extern std::array<std::array<  float,  9>, 16> YCC8ToRGB8LUTF32;
extern std::array<std::array<int16_t,  6>, 16> YCC8ToRGB8LUTI16; // Y,0,Bcb,Rcr,Gcb,Gcr
}

template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGBOrder
{
    static constexpr uint8_t R = IdxR, G = IdxG, B = IdxB;
};

namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::xziar::img::ColorConvertor;
using ::xziar::img::YCCConvertor;
using ::xziar::img::STBResize;
using ::xziar::img::YCCMatrix;

using ::xziar::img::RGB8ToYCC8LUTF32;
using ::xziar::img::RGB8ToYCC8LUTI16;
using ::xziar::img::RGB8ToYCC8LUTI8x4;
using ::xziar::img::RGB8ToYCC8LUTU8x4;
using ::xziar::img::YCC8ToRGB8LUTF32;
using ::xziar::img::YCC8ToRGB8LUTI16;


DEFINE_FASTPATHS(STBResize, DoResize)

DEFINE_FASTPATHS(ColorConvertor, 
    G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGB8, GA8ToRGBA8, G16ToGA16, G16ToRGB16, G16ToRGBA16, GA16ToRGB16, GA16ToRGBA16, GfToGAf, GfToRGBf, GfToRGBAf, GAfToRGBf, GAfToRGBAf,
    RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, RGB16ToRGBA16, BGR16ToRGBA16, RGBA16ToRGB16, RGBA16ToBGR16, RGBfToRGBAf, BGRfToRGBAf, RGBAfToRGBf, RGBAfToBGRf,
    RGB8ToBGR8, RGBA8ToBGRA8, RGB16ToBGR16, RGBA16ToBGRA16, RGBfToBGRf, RGBAfToBGRAf,
    RGB8ToR8, RGB8ToG8, RGB8ToB8, RGB16ToR16, RGB16ToG16, RGB16ToB16, RGBAfToRf, RGBAfToGf, RGBAfToBf, RGBAfToAf, RGBfToRf, RGBfToGf, RGBfToBf,
    Extract8x2, Extract8x3, Extract8x4, Extract16x2, Extract16x3, Extract16x4, Extract32x2, Extract32x3, Extract32x4,
    Combine8x2, Combine8x3, Combine8x4, Combine16x2, Combine16x3, Combine16x4, Combine32x2, Combine32x3, Combine32x4,
    G8ToG16, G16ToG8,
    RGB555ToRGB8, BGR555ToRGB8, RGB555ToRGBA8, BGR555ToRGBA8, RGB5551ToRGBA8, BGR5551ToRGBA8,
    RGB565ToRGB8, BGR565ToRGB8, RGB565ToRGBA8, BGR565ToRGBA8,
    RGB10ToRGBf, BGR10ToRGBf, RGB10ToRGBAf, BGR10ToRGBAf, RGB10A2ToRGBAf, BGR10A2ToRGBAf)

DEFINE_FASTPATHS(YCCConvertor, RGB8ToYCbCr8Fast, RGB8ToYCbCr8, YCbCr8ToRGB8)


#ifdef IMGU_FASTPATH_STB
forceinline void CallStbResize(const ::xziar::img::STBResize::ResizeInfo& info)
{
    ::stbir_resize(info.Input, static_cast<int32_t>(info.InputSizes.first), static_cast<int32_t>(info.InputSizes.second), static_cast<int>(info.InputRowStride),
        info.Output, static_cast<int32_t>(info.OutputSizes.first), static_cast<int32_t>(info.OutputSizes.second), static_cast<int>(info.OutputRowStride),
        static_cast<stbir_pixel_layout>(info.Layout), static_cast<stbir_datatype>(info.Datatype), static_cast<stbir_edge>(info.Edge), static_cast<stbir_filter>(info.Filter));
}
# if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
#   pragma message("Compiling stb_image_resize2 with AVX2")
DEFINE_FASTPATH_METHOD(DoResize, AVX2)
{
    CallStbResize(info);
}
# elif (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 20) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 10)
#   if COMMON_ARCH_X86
#     define ALGO SSE2
#     pragma message("Compiling stb_image_resize2 with SSE2")
#   else
#     define ALGO NEON
#     pragma message("Compiling stb_image_resize2 with NEON")
#   endif
DEFINE_FASTPATH_METHOD(DoResize, ALGO)
{
    CallStbResize(info);
}
#   undef ALGO
# endif
#endif


template<typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void ProcessLOOP4(Dst* __restrict dest, const Src* __restrict src, size_t count, Args&&... args) noexcept
{
    Proc proc(std::forward<Args>(args)...);
    for (size_t iter = count / (Proc::K * 4); iter > 0; iter--)
    {
        proc(dest + Proc::N * 0, src + Proc::M * 0);
        proc(dest + Proc::N * 1, src + Proc::M * 1);
        proc(dest + Proc::N * 2, src + Proc::M * 2);
        proc(dest + Proc::N * 3, src + Proc::M * 3);
        src += Proc::M * 4; dest += Proc::N * 4;
    }
    count = count % (Proc::K * 4);
    switch (count / Proc::K)
    {
    case 3: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    case 2: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    case 1: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::K;
    if (count)
        F(dest, src, count, std::forward<Args>(args)...);
}

template<size_t N, typename T, size_t... Ns>
forceinline void IncPtrs(T* __restrict * ptrs, std::index_sequence<Ns...>) noexcept
{
    (..., void(ptrs[Ns] += N));
}

template<size_t C, typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void ExtractLOOP4(Dst* const __restrict * dest, const Src* __restrict src, size_t count, Args&&... args) noexcept
{
    static constexpr auto Idxes = std::make_index_sequence<C>();
    Proc proc(std::forward<Args>(args)...);
    Dst* __restrict dsts[C] = {};
    for (size_t i = 0; i < C; ++i)
        dsts[i] = dest[i];
    for (size_t iter = count / (Proc::K * 4); iter > 0; iter--)
    {
        proc(dsts, src + Proc::M * 0); 
        IncPtrs<Proc::N>(dsts, Idxes);
        proc(dsts, src + Proc::M * 1);
        IncPtrs<Proc::N>(dsts, Idxes);
        proc(dsts, src + Proc::M * 2);
        IncPtrs<Proc::N>(dsts, Idxes);
        proc(dsts, src + Proc::M * 3);
        IncPtrs<Proc::N>(dsts, Idxes);
        src += Proc::M * 4;
    }
    count = count % (Proc::K * 4);
    switch (count / Proc::K)
    {
    case 3: proc(dsts, src); src += Proc::M; IncPtrs<Proc::N>(dsts, Idxes);
        [[fallthrough]];
    case 2: proc(dsts, src); src += Proc::M; IncPtrs<Proc::N>(dsts, Idxes);
        [[fallthrough]];
    case 1: proc(dsts, src); src += Proc::M; IncPtrs<Proc::N>(dsts, Idxes);
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::K;
    if (count)
        F(const_cast<Dst* const*>(dsts), src, count, std::forward<Args>(args)...);
}
template<size_t C, typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void CombineLOOP4(Dst* __restrict dest, const Src* const __restrict * src, size_t count, Args&&... args) noexcept
{
    static constexpr auto Idxes = std::make_index_sequence<C>();
    Proc proc(std::forward<Args>(args)...);
    const Src* __restrict srcs[C] = {};
    for (size_t i = 0; i < C; ++i)
        srcs[i] = src[i];
    for (size_t iter = count / (Proc::K * 4); iter > 0; iter--)
    {
        proc(dest + Proc::N * 0, srcs);
        IncPtrs<Proc::M>(srcs, Idxes);
        proc(dest + Proc::N * 1, srcs);
        IncPtrs<Proc::M>(srcs, Idxes);
        proc(dest + Proc::N * 2, srcs);
        IncPtrs<Proc::M>(srcs, Idxes);
        proc(dest + Proc::N * 3, srcs);
        IncPtrs<Proc::M>(srcs, Idxes);
        dest += Proc::N * 4;
    }
    count = count % (Proc::K * 4);
    switch (count / Proc::K)
    {
    case 3: proc(dest, srcs); dest += Proc::N; IncPtrs<Proc::M>(srcs, Idxes);
        [[fallthrough]];
    case 2: proc(dest, srcs); dest += Proc::N; IncPtrs<Proc::M>(srcs, Idxes);
        [[fallthrough]];
    case 1: proc(dest, srcs); dest += Proc::N; IncPtrs<Proc::M>(srcs, Idxes);
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::K;
    if (count)
        F(dest, const_cast<Src* const*>(srcs), count, std::forward<Args>(args)...);
}

#define LOOP8(func)             \
while (count >= 8)              \
{                               \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    count -= 8;                 \
}                               \
switch (count)                  \
{                               \
case 7: func; [[fallthrough]];  \
case 6: func; [[fallthrough]];  \
case 5: func; [[fallthrough]];  \
case 4: func; [[fallthrough]];  \
case 3: func; [[fallthrough]];  \
case 2: func; [[fallthrough]];  \
case 1: func; [[fallthrough]];  \
default: break;                 \
}

#define LOOP4(func)             \
while (count >= 4)              \
{                               \
    func;                       \
    func;                       \
    func;                       \
    func;                       \
    count -= 4;                 \
}                               \
switch (count)                  \
{                               \
case 3: func; [[fallthrough]];  \
case 2: func; [[fallthrough]];  \
case 1: func; [[fallthrough]];  \
default: break;                 \
}

DEFINE_FASTPATH_METHOD(G8ToGA8, LOOP)
{
    const auto mask = static_cast<uint16_t>(static_cast<uint8_t>(alpha) << 8);
#define LOOP_GRAY_GRAYA *dest++ = uint16_t(*src++) | mask;
    LOOP8(LOOP_GRAY_GRAYA)
#undef LOOP_GRAY_GRAYA
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, LOOP)
{
#define LOOP_GRAY_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src; src++
    LOOP8(LOOP_GRAY_RGB)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, LOOP)
{
    const auto mask = static_cast<uint32_t>(static_cast<uint8_t>(alpha) << 24);
#define LOOP_GRAY_RGBA *dest++ = uint32_t(*src++) * 0x010101u | mask
    LOOP8(LOOP_GRAY_RGBA)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, LOOP)
{
#define LOOP_GRAYA_RGB do { const auto tmp = static_cast<uint8_t>(*src++); *dest++ = tmp; *dest++ = tmp; *dest++ = tmp; } while(0)
    LOOP8(LOOP_GRAYA_RGB)
#undef LOOP_GRAYA_RGB
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, LOOP)
{
#define LOOP_GRAYA_RGBA do { const uint32_t tmp = *src++; *dest++ = (tmp & 0xffu) * 0x0101u | (tmp << 16u); } while(0)
    LOOP8(LOOP_GRAYA_RGBA)
#undef LOOP_GRAYA_RGBA
}
DEFINE_FASTPATH_METHOD(G16ToGA16, LOOP)
{
    const auto mask = static_cast<uint32_t>(alpha) << 16;
    uint32_t* __restrict dst = reinterpret_cast<uint32_t*>(dest);
#define LOOP_GRAY_GRAYA *dst++ = mask | *src++
    LOOP8(LOOP_GRAY_GRAYA)
#undef LOOP_GRAY_GRAYA
}
DEFINE_FASTPATH_METHOD(G16ToRGB16, LOOP)
{
#define LOOP_GRAY_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src; src++
    LOOP8(LOOP_GRAY_RGB)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(G16ToRGBA16, LOOP)
{
    const auto mask = static_cast<uint64_t>(alpha) << 48;
    constexpr uint64_t muler = 0x000100010001u;
    uint64_t* __restrict dst = reinterpret_cast<uint64_t*>(dest);
#define LOOP_GRAY_RGBA *dst++ = mask | ((*src++) * muler)
    LOOP8(LOOP_GRAY_RGBA)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(GA16ToRGB16, LOOP)
{
#define LOOP_GRAYA_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src++; src++
    LOOP8(LOOP_GRAYA_RGB)
#undef LOOP_GRAYA_RGB
}
DEFINE_FASTPATH_METHOD(GA16ToRGBA16, LOOP)
{
    uint64_t* __restrict pdst = reinterpret_cast<uint64_t*>(dest);
    const uint32_t* __restrict psrc = reinterpret_cast<const uint32_t*>(src);
#define LOOP_GRAYA_RGBA do { const auto val = *psrc++; const auto lo = (val << 16) | static_cast<uint16_t>(val); const auto hi = static_cast<uint64_t>(val) << 32; *pdst++ = hi | lo; } while(0)
    LOOP8(LOOP_GRAYA_RGBA)
#undef LOOP_GRAYA_RGBA
}
DEFINE_FASTPATH_METHOD(GfToGAf, LOOP)
{
    const auto mask = static_cast<uint64_t>(::common::bit_cast<uint32_t>(alpha)) << 32;
    uint64_t* __restrict pdst = reinterpret_cast<uint64_t*>(dest);
    const uint32_t* __restrict psrc = reinterpret_cast<const uint32_t*>(src);
#define LOOP_GRAY_GRAYA *pdst++ = mask | *psrc++
    LOOP8(LOOP_GRAY_GRAYA)
#undef LOOP_GRAY_GRAYA
}
DEFINE_FASTPATH_METHOD(GfToRGBf, LOOP)
{
#define LOOP_GRAY_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src; src++
    LOOP8(LOOP_GRAY_RGB)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, LOOP)
{
    const auto mask = static_cast<uint64_t>(::common::bit_cast<uint32_t>(alpha)) << 32;
    uint64_t* __restrict pdst = reinterpret_cast<uint64_t*>(dest);
    const uint32_t* __restrict psrc = reinterpret_cast<const uint32_t*>(src);
#define LOOP_GRAY_RGBA do { const uint64_t val = *psrc++; *pdst++ = (val << 32) | val; *pdst++ = mask | val; } while(0)
    LOOP8(LOOP_GRAY_RGBA)
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, LOOP)
{
#define LOOP_GRAYA_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src++; src++
    LOOP8(LOOP_GRAYA_RGB)
#undef LOOP_GRAYA_RGB
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, LOOP)
{
    uint64_t* __restrict pdst = reinterpret_cast<uint64_t*>(dest);
    const uint64_t* __restrict psrc = reinterpret_cast<const uint64_t*>(src);
#define LOOP_GRAYA_RGBA do { const auto val = *psrc++; const auto lo = (val << 32) | static_cast<uint32_t>(val); *pdst++ = lo; *pdst++ = val; } while(0)
    LOOP8(LOOP_GRAYA_RGBA)
#undef LOOP_GRAYA_RGBA
}

template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB3To4(uint32_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha) noexcept
{
    const uint32_t a = static_cast<uint32_t>(alpha) << 24;
#define LOOP_RGB_RGBA do { const uint32_t r = src[IdxR]; const uint32_t g = src[IdxG]; const uint32_t b = src[IdxB]; src += 3; *dest++ = r | (g << 8) | (b << 16) | a; } while(0)
    LOOP8(LOOP_RGB_RGBA)
#undef LOOP_RGB_RGBA
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, LOOP)
{
    RGB3To4<0, 1, 2>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, LOOP)
{
    RGB3To4<2, 1, 0>(dest, src, count, alpha);
}
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB4To3(uint8_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_RGB do { const auto val = *src++; *dest++ = static_cast<uint8_t>(val >> (IdxR * 8)); *dest++ = static_cast<uint8_t>(val >> (IdxG * 8)); *dest++ = static_cast<uint8_t>(val >> (IdxB * 8)); } while(0)
    LOOP8(LOOP_RGBA_RGB)
#undef LOOP_RGBA_RGB
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, LOOP)
{
    RGB4To3<0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, LOOP)
{
    RGB4To3<2, 1, 0>(dest, src, count);
}
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, typename T>
forceinline void RGBT3To4(T* __restrict dest, const T* __restrict src, size_t count, T alpha) noexcept
{
#define LOOP_RGB_RGBA do { const auto r = src[IdxR]; const auto g = src[IdxG]; const auto b = src[IdxB]; src += 3; *dest++ = r; *dest++ = g; *dest++ = b; * dest++ = alpha; } while(0)
    LOOP8(LOOP_RGB_RGBA)
#undef LOOP_RGB_RGBA
}
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, typename T>
forceinline void RGBT4To3(T* __restrict dest, const T* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_RGB *dest++ = src[IdxR]; *dest++ = src[IdxG]; *dest++ = src[IdxB]; src += 4
    LOOP8(LOOP_RGBA_RGB)
#undef LOOP_RGBA_RGB
}
DEFINE_FASTPATH_METHOD(RGB16ToRGBA16, LOOP)
{
    RGBT3To4<0, 1, 2>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR16ToRGBA16, LOOP)
{
    RGBT3To4<2, 1, 0>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA16ToRGB16, LOOP)
{
    RGBT4To3<0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGR16, LOOP)
{
    RGBT4To3<2, 1, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, LOOP)
{
    RGBT3To4<0, 1, 2>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, LOOP)
{
    RGBT3To4<2, 1, 0>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, LOOP)
{
    RGBT4To3<0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, LOOP)
{
    RGBT4To3<2, 1, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, LOOP)
{
#define LOOP_RGB_BGR do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; } while(0)
    LOOP8(LOOP_RGB_BGR)
#undef LOOP_RGB_BGR
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, LOOP)
{
#define LOOP_RGBA_BGRA do { const uint32_t tmp = *src++; const uint32_t r = (tmp & 0xffu); const uint32_t b = ((tmp >> 16) & 0xffu); *dest++ = (tmp & 0xff00ff00u) | (r << 16u) | b; } while(0)
    LOOP8(LOOP_RGBA_BGRA)
#undef LOOP_RGBA_BGRA
}
DEFINE_FASTPATH_METHOD(RGB16ToBGR16, LOOP)
{
#define LOOP_RGB_BGR do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; } while(0)
    LOOP8(LOOP_RGB_BGR)
#undef LOOP_RGB_BGR
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGRA16, LOOP)
{
#define LOOP_RGBA_BGRA do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; *dest++ = *src++; } while(0)
    LOOP8(LOOP_RGBA_BGRA)
#undef LOOP_RGBA_BGRA
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, LOOP)
{
#define LOOP_RGB_BGR do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; } while(0)
    LOOP8(LOOP_RGB_BGR)
#undef LOOP_RGB_BGR
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, LOOP)
{
#define LOOP_RGBA_BGRA do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; *dest++ = *src++; } while(0)
    LOOP8(LOOP_RGBA_BGRA)
#undef LOOP_RGBA_BGRA
}
template<uint8_t N, uint8_t Idx, typename T>
forceinline void KeepChannelLoopN_1(T* __restrict dest, const T* __restrict src, size_t count) noexcept
{
#define LOOP_N_CH *dest++ = src[Idx]; src += N
    LOOP8(LOOP_N_CH)
#undef LOOP_N_CH
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, LOOP)
{
    KeepChannelLoopN_1<3, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, LOOP)
{
    KeepChannelLoopN_1<3, 1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, LOOP)
{
    KeepChannelLoopN_1<3, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToR16, LOOP)
{
    KeepChannelLoopN_1<3, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToG16, LOOP)
{
    KeepChannelLoopN_1<3, 1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToB16, LOOP)
{
    KeepChannelLoopN_1<3, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, LOOP)
{
    KeepChannelLoopN_1<4, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, LOOP)
{
    KeepChannelLoopN_1<4, 1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, LOOP)
{
    KeepChannelLoopN_1<4, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, LOOP)
{
    KeepChannelLoopN_1<4, 3>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, LOOP)
{
    KeepChannelLoopN_1<3, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, LOOP)
{
    KeepChannelLoopN_1<3, 1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, LOOP)
{
    KeepChannelLoopN_1<3, 2>(dest, src, count);
}
template<typename T, uint8_t N>
forceinline void ExtractX(T* const __restrict * dest, const T* __restrict src, size_t count) noexcept
{
    size_t idx = 0;
#define LOOP_EXT_X do { for (uint8_t i = 0; i < N; ++i) { dest[i][idx] = *src++; } idx++; } while(0)
    LOOP8(LOOP_EXT_X)
#undef LOOP_EXT_X
}
DEFINE_FASTPATH_METHOD(Extract8x2, LOOP)
{
    ExtractX<uint8_t, 2>(dest, reinterpret_cast<const uint8_t*>(src), count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, LOOP)
{
    ExtractX<uint8_t, 3>(dest, reinterpret_cast<const uint8_t*>(src), count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, LOOP)
{
    ExtractX<uint8_t, 4>(dest, reinterpret_cast<const uint8_t*>(src), count);
}
DEFINE_FASTPATH_METHOD(Extract16x2, LOOP)
{
    ExtractX<uint16_t, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x3, LOOP)
{
    ExtractX<uint16_t, 3>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x4, LOOP)
{
    ExtractX<uint16_t, 4>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x2, LOOP)
{
    ExtractX<float, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x3, LOOP)
{
    ExtractX<float, 3>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x4, LOOP)
{
    ExtractX<float, 4>(dest, src, count);
}
template<typename T, uint8_t N>
forceinline void CombineX(T* __restrict dest, const T* const __restrict* src, size_t count) noexcept
{
    size_t idx = 0;
#define LOOP_CMB_X do { for (uint8_t i = 0; i < N; ++i) { *dest++ = src[i][idx]; } idx++; } while(0)
    LOOP8(LOOP_CMB_X)
#undef LOOP_CMB_X
}
DEFINE_FASTPATH_METHOD(Combine8x2, LOOP)
{
    CombineX<uint8_t, 2>(reinterpret_cast<uint8_t*>(dest), src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, LOOP)
{
    CombineX<uint8_t, 3>(reinterpret_cast<uint8_t*>(dest), src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, LOOP)
{
    CombineX<uint8_t, 4>(reinterpret_cast<uint8_t*>(dest), src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x2, LOOP)
{
    CombineX<uint16_t, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x3, LOOP)
{
    CombineX<uint16_t, 3>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x4, LOOP)
{
    CombineX<uint16_t, 4>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, LOOP)
{
    CombineX<float, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, LOOP)
{
    CombineX<float, 3>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, LOOP)
{
    CombineX<float, 4>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(G8ToG16, LOOP)
{
#define LOOP_8_16 do { const uint32_t val = *src++; *dest++ = static_cast<uint16_t>((val << 8) | val); } while(0)
    LOOP8(LOOP_8_16)
#undef LOOP_8_16
}
DEFINE_FASTPATH_METHOD(G16ToG8, LOOP)
{
#define LOOP_16_8 do { const uint32_t val = *src++; *dest++ = static_cast<uint8_t>(val / (UINT16_MAX / UINT8_MAX)); } while(0)
    LOOP8(LOOP_16_8)
#undef LOOP_16_8
}

template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, uint8_t BitR, uint8_t BitG, uint8_t BitB>
forceinline void RGBxxxToRGB8(uint8_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    static_assert(BitR < 8 && BitG < 8 && BitB < 8 && BitR > 0 && BitG > 0 && BitR > 0);
    constexpr auto MaskR = static_cast<uint16_t>((1u << BitR) - 1u);
    constexpr auto MaskG = static_cast<uint16_t>((1u << BitG) - 1u);
    constexpr auto MaskB = static_cast<uint16_t>((1u << BitB) - 1u);
    constexpr uint8_t ShiftR = 0;
    constexpr uint8_t ShiftG = BitR;
    constexpr uint8_t ShiftB = BitR + BitG;
    constexpr uint32_t MulR = BitR == 5 ? 527 : 259;
    constexpr uint32_t MulG = BitG == 5 ? 527 : 259;
    constexpr uint32_t MulB = BitB == 5 ? 527 : 259;
    constexpr uint32_t AddR = BitR == 5 ? 23 : 33;
    constexpr uint32_t AddG = BitG == 5 ? 23 : 33;
    constexpr uint32_t AddB = BitB == 5 ? 23 : 33;
#define LOOP_XXX_RGB do { const auto val = *src++;                                  \
const auto r = static_cast<uint8_t>((((val >> ShiftR) & MaskR) * MulR + AddR) >> 6);\
const auto g = static_cast<uint8_t>((((val >> ShiftG) & MaskG) * MulG + AddG) >> 6);\
const auto b = static_cast<uint8_t>((((val >> ShiftB) & MaskB) * MulB + AddB) >> 6);\
dest[IdxR] = r; dest[IdxG] = g; dest[IdxB] = b; dest += 3; } while (0)
    LOOP4(LOOP_XXX_RGB)
#undef LOOP_XXX_RGB
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, LOOP)
{
    RGBxxxToRGB8<0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, LOOP)
{
    RGBxxxToRGB8<2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, LOOP)
{
    RGBxxxToRGB8<0, 1, 2, 5, 6, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, LOOP)
{
    RGBxxxToRGB8<2, 1, 0, 5, 6, 5>(dest, src, count);
}

constexpr auto LUT5Bit = []() 
{
    std::array<uint8_t, 32> ret = { 0 };
    for (uint32_t i = 0; i < 32; ++i)
        ret[i] = static_cast<uint8_t>((i * 527 + 23) >> 6);
    return ret;
}();
constexpr auto LUT6Bit = []()
{
    std::array<uint8_t, 64> ret = { 0 };
    for (uint32_t i = 0; i < 64; ++i)
        ret[i] = static_cast<uint8_t>((i * 259 + 33) >> 6);
    return ret;
}();
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, uint8_t BitR, uint8_t BitG, uint8_t BitB>
forceinline void RGBxxxToRGB8LUT(uint8_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    static_assert(BitR == 5 && BitB == 5 && (BitG == 5 || BitG == 6));
    constexpr auto MaskR = static_cast<uint16_t>((1u << BitR) - 1u);
    constexpr auto MaskG = static_cast<uint16_t>((1u << BitG) - 1u);
    constexpr auto MaskB = static_cast<uint16_t>((1u << BitB) - 1u);
    constexpr uint8_t ShiftR = 0;
    constexpr uint8_t ShiftG = BitR;
    constexpr uint8_t ShiftB = BitR + BitG;
    const uint8_t* __restrict lut5 = LUT5Bit.data();
    const uint8_t* __restrict lut6 = LUT6Bit.data();
#define LOOP_XXX_RGB do { const auto val = *src++;                  \
const auto r = lut5[(val >> ShiftR) & MaskR];                       \
const auto g = (BitG == 5 ? lut5 : lut6)[(val >> ShiftG) & MaskG];  \
const auto b = lut5[(val >> ShiftB) & MaskB];                       \
dest[IdxR] = r; dest[IdxG] = g; dest[IdxB] = b; dest += 3; } while (0)
    LOOP8(LOOP_XXX_RGB)
#undef LOOP_XXX_RGB
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, LOOP2)
{
    RGBxxxToRGB8LUT<0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, LOOP2)
{
    RGBxxxToRGB8LUT<2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, LOOP2)
{
    RGBxxxToRGB8LUT<0, 1, 2, 5, 6, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, LOOP2)
{
    RGBxxxToRGB8LUT<2, 1, 0, 5, 6, 5>(dest, src, count);
}

template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, uint8_t BitR, uint8_t BitG, uint8_t BitB>
[[deprecated("not compatible with math mapping")]]
forceinline void RGBxxxToRGB8LUT2(uint8_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    static_assert(BitR == 5 && BitB == 5 && (BitG == 5 || BitG == 6));
    static_assert(IdxG == 1 && IdxR + IdxB == 2 && (IdxR == 2 || IdxB == 2));
    constexpr bool IsRev = IdxR != 0;
    constexpr uint32_t MulR = BitR == 5 ? 527 : 259;
    constexpr uint32_t MulG = BitG == 5 ? 527 : 259;
    constexpr uint32_t MulB = BitB == 5 ? 527 : 259;
    constexpr uint32_t AddR = BitR == 5 ? 23 : 33;
    constexpr uint32_t AddG = BitG == 5 ? 23 : 33;
    constexpr uint32_t AddB = BitB == 5 ? 23 : 33;
    constexpr auto MaskR = (1u << BitR) - 1u;
    constexpr auto MaskG = (1u << BitG) - 1u;
    constexpr auto MaskB = (1u << BitB) - 1u;
    constexpr uint8_t ShiftB = BitR + BitG - 8;
    constexpr auto LUTLo = []()
    {
        std::array<uint16_t, 256> ret = { 0 };
        for (uint32_t i = 0; i < 256; ++i)
        {
            const auto r = static_cast<uint8_t>(((i & MaskR) * MulR + AddR) >> 6);
            const auto g = static_cast<uint8_t>(((i >> 5) * MulG + AddG) >> 6);
            if constexpr (IsRev) // Rg->_gR
                ret[i] = static_cast<uint16_t>((r << 8) + g);
            else // Rg->Rg_
                ret[i] = static_cast<uint16_t>((g << 8) + r);
        }
        return ret;
    }();
    constexpr auto LUTHi = []()
    {
        std::array<uint16_t, 256> ret = { 0 };
        for (uint32_t i = 0; i < 256; ++i)
        {
            const auto g = static_cast<uint8_t>((((i << 3) & MaskG) * MulG) >> 6); // AddG is in Lo
            const auto b = static_cast<uint8_t>((((i >> ShiftB) & MaskB) * MulB + AddB) >> 6);
            if constexpr (IsRev) // gB->Bg_
                ret[i] = static_cast<uint16_t>((g << 8) + b);
            else // gB->_gB
                ret[i] = static_cast<uint16_t>((b << 8) + g);
        }
        return ret;
    }();
    while (count >= 4)
    {
        const auto val4 = *reinterpret_cast<const uint64_t*>(src);
        src += 4;
        const uint64_t val0l = LUTLo[static_cast<uint8_t>(val4 >>  0)];
        const uint64_t val0h = LUTHi[static_cast<uint8_t>(val4 >>  8)];
        const uint64_t val1l = LUTLo[static_cast<uint8_t>(val4 >> 16)];
        const uint64_t val1h = LUTHi[static_cast<uint8_t>(val4 >> 24)];
        const uint64_t val2l = LUTLo[static_cast<uint8_t>(val4 >> 32)];
        const uint64_t val2h = LUTHi[static_cast<uint8_t>(val4 >> 40)];
        const uint64_t val3l = LUTLo[static_cast<uint8_t>(val4 >> 48)];
        const uint64_t val3h = LUTHi[static_cast<uint8_t>(val4 >> 56)];
        uint64_t out0 = 0;
        uint32_t out1 = 0;
        if constexpr (IsRev) // RGB->_gR|Bg_
        {
            out0 = (val0l << 8) + (val0h << 0) + (val1l << 32) + (val1h << 24) + (val2l << 56) + (val2h << 48);
            out1 = static_cast<uint32_t>((val2l >> 8) + (val3l << 16) + (val3h << 8));
        }
        else // RGB->Rg_|_gB
        {
            out0 = (val0l << 0) + (val0h << 8) + (val1l << 24) + (val1h << 32) + (val2l << 48) + (val2h << 56);
            out1 = static_cast<uint32_t>((val2h >> 8) + (val3l << 8) + (val3h << 16));
        }
        reinterpret_cast<uint64_t*>(dest)[0] = out0;
        dest += 8;
        reinterpret_cast<uint32_t*>(dest)[0] = out1;
        dest += 4;
        count -= 4;
    }
    if (count > 0)
    {
        RGBxxxToRGB8<IdxR, IdxG, IdxB, BitR, BitG, BitB>(dest, src, count);
    }
}

template<bool HasAlpha, uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, uint8_t BitR, uint8_t BitG, uint8_t BitB>
forceinline void RGBxxxToRGBA8(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    static_assert(BitR < 8 && BitG < 8 && BitB < 8 && BitR > 0 && BitG > 0 && BitR > 0);
    constexpr auto MaskR = static_cast<uint16_t>((1u << BitR) - 1u);
    constexpr auto MaskG = static_cast<uint16_t>((1u << BitG) - 1u);
    constexpr auto MaskB = static_cast<uint16_t>((1u << BitB) - 1u);
    constexpr uint8_t ShiftR = 0;
    constexpr uint8_t ShiftG = BitR;
    constexpr uint8_t ShiftB = BitR + BitG;
    constexpr uint32_t MulR = BitR == 5 ? 527 : 259;
    constexpr uint32_t MulG = BitG == 5 ? 527 : 259;
    constexpr uint32_t MulB = BitB == 5 ? 527 : 259;
    constexpr uint32_t AddR = BitR == 5 ? 23 : 33;
    constexpr uint32_t AddG = BitG == 5 ? 23 : 33;
    constexpr uint32_t AddB = BitB == 5 ? 23 : 33;

#define LOOP_XXX_RGBA do { const auto val = *src++; uint32_t tmp = (!HasAlpha || (val & 0x8000u)) ? 0xff000000u : 0x0u; \
const auto r = static_cast<uint8_t>((((val >> ShiftR) & MaskR) * MulR + AddR) >> 6); tmp |= r << (IdxR * 8);            \
const auto g = static_cast<uint8_t>((((val >> ShiftG) & MaskG) * MulG + AddG) >> 6); tmp |= g << (IdxG * 8);            \
const auto b = static_cast<uint8_t>((((val >> ShiftB) & MaskB) * MulB + AddB) >> 6); tmp |= b << (IdxB * 8);            \
*dest++ = tmp; } while(0)
    LOOP4(LOOP_XXX_RGBA)
#undef LOOP_XXX_RGBA
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<false, 0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<false, 2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<true, 0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<true, 2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<false, 0, 1, 2, 5, 6, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, LOOP)
{
    RGBxxxToRGBA8<false, 2, 1, 0, 5, 6, 5>(dest, src, count);
}

template<bool HasAlpha, uint8_t IdxR, uint8_t IdxG, uint8_t IdxB, uint8_t BitR, uint8_t BitG, uint8_t BitB>
forceinline void RGBxxxToRGBA8LUT(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    static_assert(BitR == 5 && BitB == 5 && (BitG == 5 || BitG == 6));
    constexpr auto MaskR = static_cast<uint16_t>((1u << BitR) - 1u);
    constexpr auto MaskG = static_cast<uint16_t>((1u << BitG) - 1u);
    constexpr auto MaskB = static_cast<uint16_t>((1u << BitB) - 1u);
    constexpr uint8_t ShiftR = 0;
    constexpr uint8_t ShiftG = BitR;
    constexpr uint8_t ShiftB = BitR + BitG;
    const uint8_t* __restrict lut5 = LUT5Bit.data();
    const uint8_t* __restrict lut6 = LUT6Bit.data();
#define LOOP_XXX_RGB do { const auto val = *src++;                                          \
uint32_t tmp = (!HasAlpha || (val & 0x8000u)) ? 0xff000000u : 0x0u;                         \
const auto r = lut5[(val >> ShiftR) & MaskR];                      tmp |= r << (IdxR * 8);  \
const auto g = (BitG == 5 ? lut5 : lut6)[(val >> ShiftG) & MaskG]; tmp |= g << (IdxG * 8);  \
const auto b = lut5[(val >> ShiftB) & MaskB];                      tmp |= b << (IdxB * 8);  \
*dest++ = tmp; } while(0)
    LOOP8(LOOP_XXX_RGB)
#undef LOOP_XXX_RGB
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<false, 0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<false, 2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<true, 0, 1, 2, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<true, 2, 1, 0, 5, 5, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<false, 0, 1, 2, 5, 6, 5>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, LOOP2)
{
    RGBxxxToRGBA8LUT<false, 2, 1, 0, 5, 6, 5>(dest, src, count);
}

template<bool NeedAlpha, bool TakeAlpha, uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB10ToRGBf(float* __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept
{
#define LOOP_10_RGB do { const auto val = *src++;   \
const auto r = (val >> (IdxR * 10)) & 0x3ffu;       \
const auto g = (val >> (IdxG * 10)) & 0x3ffu;       \
const auto b = (val >> (IdxB * 10)) & 0x3ffu;       \
*dest++ = static_cast<int32_t>(r) * mulVal;         \
*dest++ = static_cast<int32_t>(g) * mulVal;         \
*dest++ = static_cast<int32_t>(b) * mulVal;         \
if constexpr (NeedAlpha)                            \
    *dest++ = TakeAlpha ? static_cast<int32_t>(val >> 30) / 3.0f : 1.0f; \
} while (0)
    LOOP8(LOOP_10_RGB)
#undef LOOP_10_RGB
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBf, LOOP)
{
    RGB10ToRGBf<false, false, 0, 1, 2>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBf, LOOP)
{
    RGB10ToRGBf<false, false, 2, 1, 0>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBAf, LOOP)
{
    RGB10ToRGBf<true, false, 0, 1, 2>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBAf, LOOP)
{
    RGB10ToRGBf<true, false, 2, 1, 0>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, LOOP)
{
    RGB10ToRGBf<true, true, 0, 1, 2>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, LOOP)
{
    RGB10ToRGBf<true, true, 2, 1, 0>(dest, src, count, mulVal == 0 ? 1.0f : mulVal);
}


forceinline constexpr int32_t Clamp(int32_t val, int32_t vmin, int32_t vmax) noexcept
{
    return val < vmin ? vmin : (val > vmax ? vmax : val);
}
forceinline constexpr std::tuple<bool, bool, bool> CheckYCCMatrix(uint8_t mval) noexcept
{
    const auto matrix = static_cast<YCCMatrix>(mval);
    const bool isRGBFull = HAS_FIELD(matrix, YCCMatrix::RGBFull);
    const bool isYCCFull = HAS_FIELD(matrix, YCCMatrix::YCCFull);
    const bool needAddY = !isYCCFull && isRGBFull;
    return { isRGBFull, isYCCFull, needAddY };
}

DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, LOOP_F32)
{
    const auto& lut = RGB8ToYCC8LUTF32[mval];
    [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
    [[maybe_unused]] const int32_t yAdd = needAddY ? 16 : 0;
    common::RegionRounding rd(common::RoundMode::ToNearest);

#define LOOP_RGB_YUV do { const float r = src[0], g = src[1], b = src[2];           \
const auto y  = r * lut[0] + g * lut[1] + b * lut[2];                               \
const auto cb = r * lut[3] + g * lut[4] + b * lut[5];                               \
const auto cr = r * lut[5] + g * lut[6] + b * lut[7];                               \
*dest++ = static_cast<uint8_t>(static_cast<int32_t>(std::nearbyint( y)) + yAdd);    \
auto cb_ = static_cast<int32_t>(std::nearbyint(cb)) + 128;                          \
auto cr_ = static_cast<int32_t>(std::nearbyint(cr)) + 128;                          \
if (isYCCFull) { cb_ = Clamp(cb_, 0, 255); cr_ = Clamp(cr_, 0, 255); }              \
*dest++ = static_cast<uint8_t>(cb_); *dest++ = static_cast<uint8_t>(cr_);           \
src += 3; } while (0)
    LOOP4(LOOP_RGB_YUV)
#undef LOOP_RGB_YUV
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, LOOP_I16)
{
    const auto& lut = RGB8ToYCC8LUTI16[mval];
    [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
    [[maybe_unused]] const int32_t yAdd = needAddY ? 16 : 0;

#define LOOP_RGB_YUV do { const int16_t r = src[0], g = src[1], b = src[2]; \
const auto y  = r * lut[0] + g * lut[1] + b * lut[2];                       \
const auto cb = r * lut[3] + g * lut[4] + b * lut[5];                       \
const auto cr = r * lut[5] + g * lut[6] + b * lut[7];                       \
*dest++ = static_cast<uint8_t>((( y + 8192) >> 14) + yAdd);                 \
auto cb_ = ((cb + 8192) >> 14) + 128;                                       \
auto cr_ = ((cr + 8192) >> 14) + 128;                                       \
if (isYCCFull) { cb_ = Clamp(cb_, 0, 255); cr_ = Clamp(cr_, 0, 255); }      \
*dest++ = static_cast<uint8_t>(cb_); *dest++ = static_cast<uint8_t>(cr_);   \
src += 3; } while (0)
    LOOP4(LOOP_RGB_YUV)
#undef LOOP_RGB_YUV
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, LOOP_I16)
{
    RGB8ToYCbCr8FastPath::Func<LOOP_I16>(dest, src, count, mval);
}


DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, LOOP_F32)
{
    const auto& lut = YCC8ToRGB8LUTF32[mval];
    [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
    const uint8_t yAdd = needAddY ? 16 : 0;
    const int32_t rgbMin = isRGBFull ? 0 : 16;
    const int32_t rgbMax = isRGBFull ? 255 : 235;
    common::RegionRounding rd(common::RoundMode::ToNearest);

#define LOOP_YUV_RGB do { const float y = static_cast<float>(src[0] - yAdd),    \
cb = static_cast<float>(src[1] - 128), cr = static_cast<float>(src[2] - 128);   \
const auto yl = y * lut[0];                                                     \
const auto r = yl + cr * lut[2];                                                \
const auto b = yl + cb * lut[7];                                                \
const auto g = yl - cb * lut[4] - cr * lut[5];                                  \
const auto r_ = Clamp(static_cast<int32_t>(std::nearbyint(r)), rgbMin, rgbMax); \
const auto g_ = Clamp(static_cast<int32_t>(std::nearbyint(g)), rgbMin, rgbMax); \
const auto b_ = Clamp(static_cast<int32_t>(std::nearbyint(b)), rgbMin, rgbMax); \
*dest++ = static_cast<uint8_t>(r_);                                             \
*dest++ = static_cast<uint8_t>(g_);                                             \
*dest++ = static_cast<uint8_t>(b_);                                             \
src += 3; } while (0)
    LOOP4(LOOP_YUV_RGB)
#undef LOOP_YUV_RGB
}
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, LOOP_I16)
{
    const auto& lut = YCC8ToRGB8LUTI16[mval];
    [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
    const uint8_t yAdd = needAddY ? 16 : 0;
    const int32_t rgbMin = isRGBFull ? 0 : 16;
    const int32_t rgbMax = isRGBFull ? 255 : 235;
    common::RegionRounding rd(common::RoundMode::ToNearest);

#define LOOP_YUV_RGB do { const auto y = static_cast<int16_t>(src[0] - yAdd),       \
cb = static_cast<int16_t>(src[1] - 128), cr = static_cast<int16_t>(src[2] - 128);   \
const auto yl = (y * lut[0] + 8193) >> 1;                                           \
const auto r = yl + cr * lut[3];                                                    \
const auto b = yl + cb * lut[2];                                                    \
const auto g = yl - cb * lut[4] - cr * lut[5];                                      \
const auto r_ = Clamp(r >> 13, rgbMin, rgbMax);                                     \
const auto g_ = Clamp(g >> 13, rgbMin, rgbMax);                                     \
const auto b_ = Clamp(b >> 13, rgbMin, rgbMax);                                     \
*dest++ = static_cast<uint8_t>(r_);                                                 \
*dest++ = static_cast<uint8_t>(g_);                                                 \
*dest++ = static_cast<uint8_t>(b_);                                                 \
src += 3; } while (0)
    LOOP4(LOOP_YUV_RGB)
#undef LOOP_YUV_RGB
}



#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100 && (defined(__BMI2__) || COMMON_COMPILER_MSVC) // BMI2 should be at least AVX
#   pragma message("Compiling ColorConvert with BMI2")
struct G2GA_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    uint64_t Alpha;
    forceinline G2GA_BMI2(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha) * 0x0100010001000100u) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = *reinterpret_cast<const uint32_t*>(src);
        constexpr uint64_t mask = 0x00ff00ff00ff00ffu;
        const auto tmp = _pdep_u64(dat, mask);
        *reinterpret_cast<uint64_t*>(dst) = tmp | Alpha;
    }
};
struct G2RGB_BMI2
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = *reinterpret_cast<const uint64_t*>(src);
        constexpr uint64_t mask = 0x00ff0000ff0000ffu;
        const auto val012 = _pdep_u64(dat, mask);
        const auto val345 = _pdep_u64(dat >> 24, mask);
        const auto val67  = _pdep_u64(dat >> 48, mask);
        const auto rgb012 = val012 * 0x010101u;
        const auto rgb345 = val345 * 0x010101u;
        const auto rgb67  = val67  * 0x010101u;

        const auto out0 = rgb012;
        const auto out1 = (rgb345 <<  8) | (rgb012 >> 56);
        const auto out2 = (rgb67  << 16) | (rgb345 >> 48);
        reinterpret_cast<uint64_t*>(dst)[0] = out0;
        reinterpret_cast<uint64_t*>(dst)[1] = out1;
        reinterpret_cast<uint64_t*>(dst)[2] = out2;
    }
};
struct G2RGBA_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    uint64_t Alpha;
    forceinline G2RGBA_BMI2(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha) * 0x0100000001000000u) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint16_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint16_t*>(src)[1];
        constexpr uint64_t mask = 0x000000ff000000ffu;
        const auto val01 = _pdep_u64(dat0, mask);
        const auto val23 = _pdep_u64(dat1, mask);
        const auto rgb01 = val01 * 0x010101u;
        const auto rgb23 = val23 * 0x010101u;
        reinterpret_cast<uint64_t*>(dst)[0] = rgb01 | Alpha;
        reinterpret_cast<uint64_t*>(dst)[1] = rgb23 | Alpha;
    }
};
struct GA2RGB_BMI2
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0123 = reinterpret_cast<const uint64_t*>(src)[0] & 0x00ff00ff00ff00ffu;
        const auto dat4567 = reinterpret_cast<const uint64_t*>(src)[1] & 0x00ff00ff00ff00ffu;

        constexpr uint64_t mask0 = 0xffff00ffff00ffffu;
        const auto val012 = _pdep_u64(dat0123, mask0); // 0..1..2.
        const auto out0 = val012 * 0x010101u;
        
        const auto dat3456 = ::common::simd::Shift128Right(dat4567, dat0123, 3 * 2 * 8);
        constexpr uint64_t mask1 = 0xff00ffff00ffff00u;
        const auto val345 = _pdep_u64(dat3456, mask1); // .3..4..5
        const auto val2 = static_cast<uint8_t>(dat0123 >> (2 * 2 * 8));
        const auto out1 = (val345 * 0x010101u) | val2;

        const auto dat67 = dat4567 >> (2 * 2 * 8);
        constexpr uint64_t mask2 = 0x00ffff00ffff0000u;
        const auto val67 = _pdep_u64(dat67, mask2); // ..6..7..
        const uint32_t val5 = static_cast<uint8_t>(dat4567 >> (1 * 2 * 8));
        const auto val55 = (val5 << 8) | val5;
        const auto out2 = (val67 * 0x010101u) | val55;

        reinterpret_cast<uint64_t*>(dst)[0] = out0;
        reinterpret_cast<uint64_t*>(dst)[1] = out1;
        reinterpret_cast<uint64_t*>(dst)[2] = out2;
    }
};
struct GA2RGBA_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint32_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint32_t*>(src)[1];
        constexpr uint64_t mask = 0xff0000ffff0000ffu;
        const auto val01 = _pdep_u64(dat0, mask);
        const auto val23 = _pdep_u64(dat1, mask);
        constexpr uint64_t alphaMask = 0x000000ff000000ffu;
        const auto gb01 = (val01 & alphaMask) * 0x010100u;
        const auto gb23 = (val23 & alphaMask) * 0x010100u;
        reinterpret_cast<uint64_t*>(dst)[0] = gb01 | val01;
        reinterpret_cast<uint64_t*>(dst)[1] = gb23 | val23;
    }
};
struct RGB2RGBA_BMI2
{
    static constexpr size_t M = 24, N = 8, K = 8;
    uint64_t Alpha;
    forceinline RGB2RGBA_BMI2(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha) * 0x0100000001000000u) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];
        const auto dat2 = reinterpret_cast<const uint64_t*>(src)[2];

        constexpr uint64_t mask0 = 0x00ffffff00ffffffu;
        const auto val01 = _pdep_u64(dat0, mask0);
        const auto out0 = val01 | Alpha;
        reinterpret_cast<uint64_t*>(dst)[0] = out0;

        constexpr uint64_t mask1 = 0x00ffffff00ff0000u;
        const auto val2_rg = dat0 >> 48u;
        const auto val23 = _pdep_u64(dat1, mask1);
        const auto out1 = val23 | val2_rg | Alpha;
        reinterpret_cast<uint64_t*>(dst)[1] = out1;

        const auto dat12 = (dat1 >> 32) | (dat2 << 32);
        const auto val45 = _pdep_u64(dat12, mask0);
        const auto out2 = val45 | Alpha;
        reinterpret_cast<uint64_t*>(dst)[2] = out2;

        const auto dat2_ = dat2 >> 16u;
        const auto val67 = _pdep_u64(dat2_, mask0);
        const auto out3 = val67 | Alpha;
        reinterpret_cast<uint64_t*>(dst)[3] = out3;
    }
};
struct RGBA2RGB_BMI2
{
    static constexpr size_t M = 4, N = 12, K = 4;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];

        constexpr uint64_t mask0 = 0x00ffffff00ffffffu;
        const auto val01 = _pext_u64(dat0, mask0);
        constexpr uint64_t mask1 = 0xffff000000000000u;
        const auto val2_rg = _pdep_u64(dat1, mask1);
        const auto out0 = val01 | val2_rg;
        reinterpret_cast<uint64_t*>(dst)[0] = out0;

        constexpr uint64_t mask2 = 0x00ffffff00ff0000u;
        const auto val2_b_3 = _pext_u64(dat1, mask2);
        const auto out1 = static_cast<uint32_t>(val2_b_3);
        reinterpret_cast<uint32_t*>(dst)[2] = out1;
    }
};
template<uint32_t Idx>
struct KeepChannel3_1_BMI2
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];
        const auto dat2 = reinterpret_cast<const uint64_t*>(src)[2];

        if constexpr (Idx == 0)
        {
            constexpr auto mask0 = uint64_t(0x00ff0000ff0000ffu);
            constexpr auto mask1 = uint64_t(0xff0000ff0000ff00u);
            constexpr auto mask2 = uint64_t(0x0000ff0000ff0000u);
            const auto val012 = _pext_u64(dat0, mask0);
            const auto val345 = _pext_u64(dat1, mask1);
            const auto val67  = _pext_u64(dat2, mask2);
            const auto out = val012 | (val345 << 24) | (val67 << 48);
            reinterpret_cast<uint64_t*>(dst)[0] = out;
        }
        else if constexpr (Idx == 1)
        {
            constexpr auto mask0 = uint64_t(0xff0000ff0000ff00u);
            constexpr auto mask1 = uint64_t(0x0000ff0000ff0000u);
            constexpr auto mask2 = uint64_t(0x00ff0000ff0000ffu);
            const auto val012 = _pext_u64(dat0, mask0);
            const auto val34  = _pext_u64(dat1, mask1);
            const auto val567 = _pext_u64(dat2, mask2);
            const auto out = val012 | (val34 << 24) | (val567 << 40);
            reinterpret_cast<uint64_t*>(dst)[0] = out;
        }
        else
        {
            constexpr auto mask0 = uint64_t(0x0000ff0000ff0000u);
            constexpr auto mask1 = uint64_t(0x00ff0000ff0000ffu);
            constexpr auto mask2 = uint64_t(0xff0000ff0000ff00u);
            const auto val01  = _pext_u64(dat0, mask0);
            const auto val234 = _pext_u64(dat1, mask1);
            const auto val567 = _pext_u64(dat2, mask2);
            const auto out = val01 | (val234 << 16) | (val567 << 40);
            reinterpret_cast<uint64_t*>(dst)[0] = out;
        }
    }
};
template<bool TakeAlpha>
struct RGB555ToRGBA8_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = reinterpret_cast<const uint64_t*>(src)[0];
        
        constexpr uint64_t maskExtend = 0x001f001f001fu;
        const auto dat0 = _pdep_u64(dat >>  0, maskExtend);
        const auto dat1 = _pdep_u64(dat >> 16, maskExtend);
        const auto dat2 = _pdep_u64(dat >> 32, maskExtend);
        const auto dat3 = _pdep_u64(dat >> 48, maskExtend);

        const auto val0 = dat0 * 527 + 0x001700170017u;
        const auto val1 = dat1 * 527 + 0x001700170017u;
        const auto val2 = dat2 * 527 + 0x001700170017u;
        const auto val3 = dat3 * 527 + 0x001700170017u;
        
        constexpr uint64_t maskKeep = 0x3fc03fc03fc0u;
        const auto pack0 = _pext_u64(val0, maskKeep) | (_pext_u64(val1, maskKeep) << 32);
        const auto pack1 = _pext_u64(val2, maskKeep) | (_pext_u64(val3, maskKeep) << 32);
        if constexpr (TakeAlpha)
        {
            constexpr uint64_t alpahMask = 0x8000800080008000u;
            const auto alphas = static_cast<uint32_t>(_pext_u64(dat, alpahMask));
            CM_ASSUME(alphas <= 0xf);
            constexpr uint64_t AlphaVals[4] = /*00, 01, 10, 11*/
            {
                0x0000000000000000u, 0x00000000ff000000u, 0xff00000000000000u, 0xff000000ff000000u
            };
            const auto out0 = pack0 | AlphaVals[alphas & 0x3u];
            const auto out1 = pack1 | AlphaVals[alphas >> 2];
            reinterpret_cast<uint64_t*>(dst)[0] = out0;
            reinterpret_cast<uint64_t*>(dst)[1] = out1;
        }
        else
        {
            constexpr uint64_t alpha = 0xff000000ff000000u;
            const auto out0 = pack0 | alpha;
            const auto out1 = pack1 | alpha;
            reinterpret_cast<uint64_t*>(dst)[0] = out0;
            reinterpret_cast<uint64_t*>(dst)[1] = out1;
        }
    }
};
DEFINE_FASTPATH_METHOD(G8ToGA8, BMI2)
{
    ProcessLOOP4<G2GA_BMI2, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, BMI2)
{
    ProcessLOOP4<G2RGB_BMI2, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, BMI2)
{
    ProcessLOOP4<G2RGBA_BMI2, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, BMI2)
{
    ProcessLOOP4<GA2RGB_BMI2, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, BMI2)
{
    ProcessLOOP4<GA2RGBA_BMI2, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, BMI2)
{
    ProcessLOOP4<RGB2RGBA_BMI2, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, BMI2)
{
    ProcessLOOP4<RGBA2RGB_BMI2, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, BMI2)
{
    ProcessLOOP4<KeepChannel3_1_BMI2<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, BMI2)
{
    ProcessLOOP4<KeepChannel3_1_BMI2<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, BMI2)
{
    ProcessLOOP4<KeepChannel3_1_BMI2<2>, &Func<LOOP>>(dest, src, count);
}
//DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, BMI2)
//{
//    ProcessLOOP4<RGB555ToRGBA8_BMI2<false>, &Func<LOOP>>(dest, src, count);
//}
//DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, BMI2)
//{
//    ProcessLOOP4<RGB555ToRGBA8_BMI2<true>, &Func<LOOP>>(dest, src, count);
//}
#endif


// SIMD128
#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
struct G2GA8_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2GA8_128(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        const auto outLo = dat.ZipLo(Alpha).As<U16x8>();
        const auto outHi = dat.ZipHi(Alpha).As<U16x8>();
        outLo.Save(dst);
        outHi.Save(dst + 8);
    }
};
struct G2RGB8_128
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5>();
        const auto out1 = dat.Shuffle<5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10>();
        const auto out2 = dat.Shuffle<10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15>();
        out0.Save(dst);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct G2RGBA8_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2RGBA8_128(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        const auto rgLo = dat.ZipLo(dat).As<U16x8>();
        const auto rgHi = dat.ZipHi(dat).As<U16x8>();
        const auto baLo = dat.ZipLo(Alpha).As<U16x8>();
        const auto baHi = dat.ZipHi(Alpha).As<U16x8>();
        const auto rgba0123 = rgLo.ZipLo(baLo).As<U32x4>();
        const auto rgba4567 = rgLo.ZipHi(baLo).As<U32x4>();
        const auto rgba89ab = rgHi.ZipLo(baHi).As<U32x4>();
        const auto rgbacdef = rgHi.ZipHi(baHi).As<U32x4>();
        rgba0123.Save(dst);
        rgba4567.Save(dst + 4);
        rgba89ab.Save(dst + 8);
        rgbacdef.Save(dst + 12);
    }
};
struct G2GA16_128
{
    static constexpr size_t M = 8, N = 16, K = 8;
    U16x8 Alpha;
    forceinline G2GA16_128(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct G2RGB16_128
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 1, 1, 1, 2, 2>();
        const auto out1 = dat.Shuffle<2, 3, 3, 3, 4, 4, 4, 5>();
        const auto out2 = dat.Shuffle<5, 5, 6, 6, 6, 7, 7, 7>();
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct G2RGBA16_128
{
    static constexpr size_t M = 8, N = 32, K = 8;
    U16x8 Alpha;
    forceinline G2RGBA16_128(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 0, 1, 1, 1, 1>().SelectWith<0b10001000>(Alpha);
        const auto out1 = dat.Shuffle<2, 2, 2, 2, 3, 3, 3, 3>().SelectWith<0b10001000>(Alpha);
        const auto out2 = dat.Shuffle<4, 4, 4, 4, 5, 5, 5, 5>().SelectWith<0b10001000>(Alpha);
        const auto out3 = dat.Shuffle<6, 6, 6, 6, 7, 7, 7, 7>().SelectWith<0b10001000>(Alpha);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct GA2RGB16_128
{
    static constexpr size_t M = 16, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src);
        const auto dat1 = U16x8(src + 8);
        const auto out0 = dat0.Shuffle<0, 0, 0, 2, 2, 2, 4, 4>();
# if COMMON_ARCH_X86
        const auto out1 = dat0.SelectWith<0x0f>(dat1).Shuffle<4, 6, 6, 6, 0, 0, 0, 2>();
# else
        alignas(16) static const uint8_t indexes[] = { 8,9, 12,13, 12,13, 12,13, 16,17, 16,17, 16,17, 20,21 };
        const auto tbl = vld1q_u8(indexes);
        uint8x16x2_t dat01;
        dat01.val[0] = vreinterpretq_u8_u16(dat0);
        dat01.val[1] = vreinterpretq_u8_u16(dat1);
        const auto out1 = U8x16(vqtbl2q_u8(dat01, tbl)).As<U16x8>();
# endif
        const auto out2 = dat1.Shuffle<2, 2, 4, 4, 4, 6, 6, 6>();
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct GA2RGBA16_128
{
    static constexpr size_t M = 8, N = 16, K = 4;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 1, 2, 2, 2, 3>();
        const auto out1 = dat.Shuffle<4, 4, 4, 5, 6, 6, 6, 7>();
        out0.Save(dst + 0);
        out1.Save(dst + 8);
    }
};
struct G2GAf_128
{
    static constexpr size_t M = 4, N = 8, K = 4;
    F32x4 Alpha;
    forceinline G2GAf_128(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst + 0);
        out[1].Save(dst + 4);
    }
};
struct G2RGBf_128
{
    static constexpr size_t M = 4, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 1>();
        const auto out1 = dat.Shuffle<1, 1, 2, 2>();
        const auto out2 = dat.Shuffle<2, 3, 3, 3>();
        out0.Save(dst + 0);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
struct G2RGBAf_128
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Alpha;
    forceinline G2RGBAf_128(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        const auto out0 = dat.Broadcast<0>().SelectWith<0b1000>(Alpha);
        const auto out1 = dat.Broadcast<1>().SelectWith<0b1000>(Alpha);
        const auto out2 = dat.Broadcast<2>().SelectWith<0b1000>(Alpha);
        const auto out3 = dat.Broadcast<3>().SelectWith<0b1000>(Alpha);
        out0.Save(dst +  0);
        out1.Save(dst +  4);
        out2.Save(dst +  8);
        out3.Save(dst + 12);
    }
};
struct GA2RGBf_128
{
    static constexpr size_t M = 8, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = F32x4(src);
        const auto dat1 = F32x4(src + 4);
        const auto out0 = dat0.Shuffle<0, 0, 0, 2>();
# if COMMON_ARCH_X86
        const F32x4 out1 = _mm_shuffle_ps(dat0, dat1, 0b00001010);
# else
        const auto out1 = dat0.MoveToLoWith<2>(dat1).Shuffle<0, 0, 2, 2>();
# endif
        const auto out2 = dat1.Shuffle<0, 2, 2, 2>();
        out0.Save(dst + 0);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
struct GA2RGBAf_128
{
    static constexpr size_t M = 4, N = 8, K = 2;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        const auto out0 = dat.Shuffle<0, 0, 0, 1>();
        const auto out1 = dat.Shuffle<2, 2, 2, 3>();
        out0.Save(dst + 0);
        out1.Save(dst + 4);
    }
};
struct RGBAToBGRA8_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);

        const auto out0 = dat0.As<U8x16>().Shuffle<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x4>();
        const auto out1 = dat1.As<U8x16>().Shuffle<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x4>();
        const auto out2 = dat2.As<U8x16>().Shuffle<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x4>();
        const auto out3 = dat3.As<U8x16>().Shuffle<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x4>();

        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
        out3.Save(dst + 12);
    }
};
struct RGBAToBGRA16_128
{
    static constexpr size_t M = 32, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src);
        const U16x8 dat1(src + 8);
        const U16x8 dat2(src + 16);
        const U16x8 dat3(src + 24);

        const auto out0 = dat0.Shuffle<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out1 = dat1.Shuffle<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out2 = dat2.Shuffle<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out3 = dat3.Shuffle<2, 1, 0, 3, 6, 5, 4, 7>();

        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct RGBAToBGRAf_128
{
    static constexpr size_t M = 16, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src);
        const F32x4 dat1(src + 4);
        const F32x4 dat2(src + 8);
        const F32x4 dat3(src + 12);

        const auto out0 = dat0.Shuffle<2, 1, 0, 3>();
        const auto out1 = dat1.Shuffle<2, 1, 0, 3>();
        const auto out2 = dat2.Shuffle<2, 1, 0, 3>();
        const auto out3 = dat3.Shuffle<2, 1, 0, 3>();

        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
        out3.Save(dst + 12);
    }
};
struct Combine8x2_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);

        const auto out = dat0.Zip(dat1).As<U16x8>();
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct Combine8x4_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);
        const U8x16 dat2(src[2]);
        const U8x16 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<U16x8>();
        const auto mid23 = dat2.Zip(dat3).As<U16x8>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<U32x4>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<U32x4>();
        out01[0].Save(dst + 0);
        out01[1].Save(dst + 4);
        out23[0].Save(dst + 8);
        out23[1].Save(dst + 12);
    }
};
struct Combine16x2_128
{
    static constexpr size_t M = 8, N = 16, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);

        const auto out = dat0.Zip(dat1);
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct Combine16x4_128
{
    static constexpr size_t M = 8, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);
        const U16x8 dat2(src[2]);
        const U16x8 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<U32x4>();
        const auto mid23 = dat2.Zip(dat3).As<U32x4>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<U16x8>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<U16x8>();
        out01[0].Save(dst + 0);
        out01[1].Save(dst + 8);
        out23[0].Save(dst + 16);
        out23[1].Save(dst + 24);
    }
};
struct Combine32x2_128
{
    static constexpr size_t M = 4, N = 8, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);

        const auto out = dat0.Zip(dat1);
        out[0].Save(dst + 0);
        out[1].Save(dst + 4);
    }
};
struct Combine32x4_128
{
    static constexpr size_t M = 4, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);
        const F32x4 dat2(src[2]);
        const F32x4 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<F64x2>();
        const auto mid23 = dat2.Zip(dat3).As<F64x2>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<F32x4>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<F32x4>();
        out01[0].Save(dst + 0);
        out01[1].Save(dst + 4);
        out23[0].Save(dst + 8);
        out23[1].Save(dst + 12);
    }
};
struct G8ToG16_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat(src);

        const auto out = dat.Zip(dat).As<U16x8>();
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
template<bool Is555>
struct RGB5x5ToRGB8_128Base
{
    forceinline static std::array<U16x8, 3> RGB5x5ToRGB8Hi(const U16x8& dat) noexcept
    {
        const U16x8 maskLo5bit(uint16_t(0x1fu));
        const U16x8 maskLo6bit(uint16_t(0x3fu));
        const auto valR = dat.And(maskLo5bit);
        const auto valG = dat.ShiftRightLogic<5>().And(Is555 ? maskLo5bit : maskLo6bit);
        U16x8 valB = dat.ShiftRightLogic<Is555 ? 10 : 11>();
        if constexpr (Is555) valB = valB.And(maskLo5bit);

        // pre-shift-left by 2
        const U16x8 add5(23 << 2), add6(33 << 2);
        constexpr uint16_t muler5 = 527 << 2, muler6 = 259 << 2;
# if COMMON_ARCH_X86
        const U16x8 mul5(muler5), mul6(muler6);
        const U16x8 midR = valR.MulAddLo(mul5, add5); // at Hi
        const U16x8 midG = valG.MulAddLo(Is555 ? mul5 : mul6, Is555 ? add5 : add6); // at Hi
        const U16x8 midB = valB.MulAddLo(mul5, add5); // at Hi
# else
        const U16x8 muler = U32x4::LoadLo((static_cast<uint32_t>(muler6) << 16) + muler5).As<U16x8>();
        const U16x8 midR = valR.MulScalarAddLo<0>(muler, add5); // at Hi
        const U16x8 midG = valG.MulScalarAddLo<Is555 ? 0 : 1>(muler, Is555 ? add5 : add6); // at Hi
        const U16x8 midB = valB.MulScalarAddLo<0>(muler, add5); // at Hi
# endif

        return { midR, midG, midB };
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_128 : RGB5x5ToRGB8_128Base<Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat(src);
        const auto [midR, midG, midB] = this->RGB5x5ToRGB8Hi(dat);

        const U16x8 valLo = (IsRGB ? midR : midB).template As<U8x16>().ZipEven(midG.template As<U8x16>()).template As<U16x8>();

        U16x8 valHi;
        if constexpr (HasAlpha)
        {
            const auto midA = dat.As<I16x8>().ShiftRightArith<7>().As<U16x8>();
            valHi = (IsRGB ? midB : midR).template As<U8x16>().ZipEven(midA.As<U8x16>()).template As<U16x8>();
        }
        else
        {
            const U16x8 alphaMask(uint16_t(0xff00));
# if COMMON_ARCH_X86
            valHi = (IsRGB ? midB : midR).template ShiftRightLogic<8>().Or(alphaMask);
# else
            valHi = (IsRGB ? midB : midR).template As<U8x16>().ZipEven(alphaMask.As<U8x16>()).template As<U16x8>();
# endif
        }
# if COMMON_ARCH_X86
        const auto out = valLo.Zip(valHi).As<U32x4>();
        out[0].Save(dst + 0);
        out[1].Save(dst + 4);
# else
        uint16x8x2_t out2;
        out2.val[0] = valLo;
        out2.val[1] = valHi;
        vst2q_u16(reinterpret_cast<uint16_t*>(dst), out2);
# endif
    }
};

DEFINE_FASTPATH_METHOD(G8ToGA8, SIMD128)
{
    ProcessLOOP4<G2GA8_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, SIMD128)
{
    ProcessLOOP4<G2RGB8_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SIMD128)
{
    ProcessLOOP4<G2RGBA8_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G16ToGA16, SIMD128)
{
    ProcessLOOP4<G2GA16_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G16ToRGB16, SIMD128)
{
    ProcessLOOP4<G2RGB16_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToRGBA16, SIMD128)
{
    ProcessLOOP4<G2RGBA16_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA16ToRGB16, SIMD128)
{
    ProcessLOOP4<GA2RGB16_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA16ToRGBA16, SIMD128)
{
    ProcessLOOP4<GA2RGBA16_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, SIMD128)
{
    ProcessLOOP4<G2GAf_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, SIMD128)
{
    ProcessLOOP4<G2RGBf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, SIMD128)
{
    ProcessLOOP4<G2RGBAf_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, SIMD128)
{
    ProcessLOOP4<GA2RGBf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, SIMD128)
{
    ProcessLOOP4<GA2RGBAf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, SIMD128)
{
    ProcessLOOP4<RGBAToBGRA8_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGRA16, SIMD128)
{
    ProcessLOOP4<RGBAToBGRA16_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, SIMD128)
{
    ProcessLOOP4<RGBAToBGRAf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x2, SIMD128)
{
    CombineLOOP4<2, Combine8x2_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, SIMD128)
{
    CombineLOOP4<4, Combine8x4_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x2, SIMD128)
{
    CombineLOOP4<2, Combine16x2_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x4, SIMD128)
{
    CombineLOOP4<4, Combine16x4_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, SIMD128)
{
    CombineLOOP4<2, Combine32x2_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, SIMD128)
{
    CombineLOOP4<4, Combine32x4_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToG16, SIMD128)
{
    ProcessLOOP4<G8ToG16_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<true, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<false, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<true, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<false, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<true, false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB5x5ToRGBA8_128<false, false, false>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41
struct G2RGBA_SSSE3
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U32x4 Alpha;
    forceinline G2RGBA_SSSE3(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        const auto shuffleMask1 = _mm_setr_epi8( 0,  0,  0, -1,  1,  1,  1, -1,  2,  2,  2, -1,  3,  3,  3, -1);
        const auto shuffleMask2 = _mm_setr_epi8( 4,  4,  4, -1,  5,  5,  5, -1,  6,  6,  6, -1,  7,  7,  7, -1);
        const auto shuffleMask3 = _mm_setr_epi8( 8,  8,  8, -1,  9,  9,  9, -1, 10, 10, 10, -1, 11, 11, 11, -1);
        const auto shuffleMask4 = _mm_setr_epi8(12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1);
        const U32x4 rgb0123 = _mm_shuffle_epi8(dat.Data, shuffleMask1);
        const U32x4 rgb4567 = _mm_shuffle_epi8(dat.Data, shuffleMask2);
        const U32x4 rgb89ab = _mm_shuffle_epi8(dat.Data, shuffleMask3);
        const U32x4 rgbcdef = _mm_shuffle_epi8(dat.Data, shuffleMask4);
        const auto rgba0123 = rgb0123.Or(Alpha);
        const auto rgba4567 = rgb4567.Or(Alpha);
        const auto rgba89ab = rgb89ab.Or(Alpha);
        const auto rgbacdef = rgbcdef.Or(Alpha);
        rgba0123.Save(dst);
        rgba4567.Save(dst + 4);
        rgba89ab.Save(dst + 8);
        rgbacdef.Save(dst + 12);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_3To4_SSSE3
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U32x4 Alpha;
    forceinline RGB8_3To4_SSSE3(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24u) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat0(src +  0);
        const U8x16 dat1(src + 16);
        const U8x16 dat2(src + 32);

        const auto shuffleMask0 = _mm_setr_epi8(IdxR + 0, IdxG + 0, IdxB + 0, -1, IdxR + 3, IdxG + 3, IdxB + 3, -1, IdxR +  6, IdxG +  6, IdxB +  6, -1, IdxR +  9, IdxG +  9, IdxB +  9, -1);
        const auto shuffleMask1 = _mm_setr_epi8(IdxR + 4, IdxG + 4, IdxB + 4, -1, IdxR + 7, IdxG + 7, IdxB + 7, -1, IdxR + 10, IdxG + 10, IdxB + 10, -1, IdxR + 13, IdxG + 13, IdxB + 13, -1);

        const U32x4 mid0 = _mm_shuffle_epi8(dat0, shuffleMask0);
        const auto out0 = mid0.Or(Alpha);
        out0.Save(dst);

        const U8x16 dat1_ = dat0.MoveToLoWith<12>(dat1);
        const U32x4 mid1 = _mm_shuffle_epi8(dat1_, shuffleMask0);
        const auto out1 = mid1.Or(Alpha);
        out1.Save(dst + 4);

        const U8x16 dat2_ = dat1.MoveToLoWith<8>(dat2);
        const U32x4 mid2 = _mm_shuffle_epi8(dat2_, shuffleMask0); 
        const auto out2 = mid2.Or(Alpha);
        out2.Save(dst + 8);

        const U32x4 mid3 = _mm_shuffle_epi8(dat2, shuffleMask1);
        const auto out3 = mid3.Or(Alpha);
        out3.Save(dst + 12);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_4To3_SSSE3
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);

        const U8x16 mid0123 = dat0.As<U8x16>().Shuffle<        0,         0,         0,         0, IdxR +  0, IdxG +  0, IdxB +  0, IdxR +  4, IdxG +  4, IdxB +  4, IdxR +  8, IdxG +  8, IdxB + 8, IdxR + 12, IdxG + 12, IdxB + 12>();
        const U8x16 mid4567 = dat1.As<U8x16>().Shuffle<IdxR +  0, IdxG +  0, IdxB +  0, IdxR +  4, IdxG +  4, IdxB +  4, IdxR +  8, IdxG +  8, IdxB +  8, IdxR + 12, IdxG + 12, IdxB + 12,       15,        15,        15,        15>();
        const U8x16 mid89ab = dat2.As<U8x16>().Shuffle<        0,         0,         0,         0, IdxR +  0, IdxG +  0, IdxB +  0, IdxR +  4, IdxG +  4, IdxB +  4, IdxR +  8, IdxG +  8, IdxB + 8, IdxR + 12, IdxG + 12, IdxB + 12>();
        const U8x16 midcdef = dat3.As<U8x16>().Shuffle<IdxR +  0, IdxG +  0, IdxB +  0, IdxR +  4, IdxG +  4, IdxB +  4, IdxR +  8, IdxG +  8, IdxB +  8, IdxR + 12, IdxG + 12, IdxB + 12,       15,        15,        15,        15>();

        const U8x16 out0 = mid0123.MoveToLoWith<4>(mid4567);
        out0.Save(dst);
        const U8x16 out1 = F32x4(_mm_shuffle_ps(mid4567.As<F32x4>(), mid89ab.As<F32x4>(), 0b10011001)).As<U8x16>();
        out1.Save(dst + 16);
        const U8x16 out2 = mid89ab.MoveToLoWith<12>(midcdef);
        out2.Save(dst + 32);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB16_3To4_SSSE3
{
    static constexpr size_t M = 24, N = 32, K = 8;
    U16x8 Alpha;
    forceinline RGB16_3To4_SSSE3(uint16_t alpha) noexcept : Alpha(U64x2(static_cast<uint64_t>(alpha) << 48u).As<U16x8>()) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src +  0);
        const U16x8 dat1(src +  8);
        const U16x8 dat2(src + 16);

        constexpr uint8_t R = IdxR * 2, G = IdxG * 2, B = IdxB * 2;

        const auto shuffleMask0 = _mm_setr_epi8(R + 0, R + 1, G + 0, G + 1, B + 0, B + 1, -1, -1, R +  6, R +  7, G +  6, G +  7, B +  6, B +  7, -1, -1);
        const auto shuffleMask1 = _mm_setr_epi8(R + 4, R + 5, G + 4, G + 5, B + 4, B + 5, -1, -1, R + 10, R + 11, G + 10, G + 11, B + 10, B + 11, -1, -1);

        const U16x8 mid0 = _mm_shuffle_epi8(dat0, shuffleMask0);
        const auto out0 = mid0.Or(Alpha);
        out0.Save(dst);

        const U16x8 dat1_ = dat0.MoveToLoWith<6>(dat1);
        const U16x8 mid1 = _mm_shuffle_epi8(dat1_, shuffleMask0);
        const auto out1 = mid1.Or(Alpha);
        out1.Save(dst + 8);

        const U16x8 dat2_ = dat1.MoveToLoWith<4>(dat2);
        const U16x8 mid2 = _mm_shuffle_epi8(dat2_, shuffleMask0);
        const auto out2 = mid2.Or(Alpha);
        out2.Save(dst + 16);

        const U16x8 mid3 = _mm_shuffle_epi8(dat2, shuffleMask1);
        const auto out3 = mid3.Or(Alpha);
        out3.Save(dst + 24);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB16_4To3_SSSE3
{
    static constexpr size_t M = 32, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src);
        const U16x8 dat1(src + 8);
        const U16x8 dat2(src + 16);
        const U16x8 dat3(src + 24);

        const U16x8 mid01 = dat0.Shuffle<0, 0, IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4>();
        const U16x8 mid23 = dat1.Shuffle<IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4, 7, 7>();
        const U16x8 mid45 = dat2.Shuffle<0, 0, IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4>();
        const U16x8 mid67 = dat3.Shuffle<IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4, 7, 7>();

        const U16x8 out0 = mid01.MoveToLoWith<2>(mid23);
        out0.Save(dst);
        const U16x8 out1 = F32x4(_mm_shuffle_ps(mid23.As<F32x4>(), mid45.As<F32x4>(), 0b10011001)).As<U16x8>();
        out1.Save(dst + 8);
        const U16x8 out2 = mid45.MoveToLoWith<6>(mid67);
        out2.Save(dst + 16);
    }
};
template<bool IsRGB>
struct RGBf_3To4_SSSE3
{
    static constexpr size_t M = 12, N = 16, K = 4;
    F32x4 Alpha;
    forceinline RGBf_3To4_SSSE3(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src + 0); // R0 G0 B0 R1
        const F32x4 dat1(src + 4); // G1 B1 R2 G2
        const F32x4 dat2(src + 8); // B2 R3 G3 B3

        F32x4 out0 = dat0.SelectWith<0b1000>(Alpha);
        if constexpr (!IsRGB) out0 = out0.Shuffle<2, 1, 0, 3>();

        F32x4 out3 = dat2.MoveToLoWith<1>(Alpha);
        if constexpr (!IsRGB) out3 = out3.Shuffle<2, 1, 0, 3>();

        F32x4 out1, out2;
        if constexpr (IsRGB)
        {
            const auto mid1 = dat1.SelectWith<0b0100>(Alpha); // G1 B1 A G2
            out1 = dat0.MoveToLoWith<3>(mid1);
            const auto mid2 = dat2.SelectWith<0b0010>(Alpha); // B2 A G3 B3
            out2 = dat1.MoveToLoWith<2>(mid2);
        }
        else
        {
            const auto mid0 = dat0.SelectWith<0b0100>(Alpha); // R0 G0 A R1
            out1 = _mm_shuffle_ps(dat1, mid0, 0b10110001);
            const auto mid2 = dat2.SelectWith<0b1000>(Alpha); // B2 R3 G3 A
            const auto mid1 = dat1.Shuffle<0, 3, 2, 1>(); // G1 G2 R2 B1
            out2 = mid1.SelectWith<0b1001>(mid2);
        }

        out0.Save(dst + 0);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
        out3.Save(dst + 12);
    }
};
template<bool IsRGB>
struct RGBf_4To3_SSE41
{
    static constexpr size_t M = 16, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src);
        const F32x4 dat1(src + 4);
        const F32x4 dat2(src + 8);
        const F32x4 dat3(src + 12);

        F32x4 out0, out1, out2;
        if constexpr (IsRGB)
        {
            out0 = _mm_insert_ps(dat0, dat1, 0b00110000); // b[00]->a[11], zero[0000]
            out1 = _mm_shuffle_ps(dat1, dat2, 0b01001001);
            const F32x4 mid2 = _mm_moveldup_ps(dat2); // R2 R2 B2 B2
            out2 = mid2.MoveToLoWith<3>(dat3);
        }
        else
        {
            const auto mid0 = dat0.Shuffle<2, 1, 0, 3>(); // B0 G0 R0 A0
            out0 = _mm_insert_ps(mid0, dat1, 0b10110000); // b[10]->a[11], zero[0000]
            out1 = _mm_shuffle_ps(dat1, dat2, 0b01100001);
            const auto mid3 = dat3.Shuffle<3, 2, 1, 0>(); // A3 B3 G3 R3
            out2 = dat2.SelectWith<0b1110>(mid3);
        }

        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
struct RGBToBGR8_SSSE3
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat0(src);
        const U8x16 dat1(src + 16);
        const U8x16 dat2(src + 32);

        const auto mid0 = dat0.Shuffle<0, 2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12>(); // x000111222333444
        const auto merge01 = dat0.MoveToLoWith<15>(dat1); // 555666777888999a
        const U8x16 mid01 = _mm_shuffle_epi8(merge01, _mm_setr_epi8(2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, -1)); // 555666777888999.
        const auto merge12 = dat1.MoveToLoWith<1>(dat2); // 5666777888999aaa
        const U8x16 mid12 = _mm_shuffle_epi8(merge12, _mm_setr_epi8(-1, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13)); // .666777888999aaa
        const auto mid2 = dat2.Shuffle<3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13, 0>(); // bbbcccdddeeefffx
        const U8x16 mid1lo = mid01.MoveToLo<1>();
        const U8x16 mid1hi = mid12.MoveToHi<1>();

        const U8x16 out0 = mid0.MoveToLoWith<1>(mid01);
        out0.Save(dst);
        const auto out1 = mid1lo.Or(mid1hi);
        out1.Save(dst + 16);
        const U8x16 out2 = mid12.MoveToLoWith<15>(mid2);
        out2.Save(dst + 32);
    }
};
struct RGBToBGR16_SSE41
{
    static constexpr size_t M = 24, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src +  0); // R0 G0 B0 R1 G1 B1 R2 G2
        const U16x8 dat1(src +  8); // B2 R3 G3 B3 R4 G4 B4 R5
        const U16x8 dat2(src + 16); // G5 B5 R6 G6 B6 R7 G7 B7

        const auto mix01  = dat0 .SelectWith<0b10000001>(dat1); // B2 G0 B0 R1 G1 B1 R2 R5
        const auto mix012 = mix01.SelectWith<0b00000010>(dat2); // B2 B5 B0 R1 G1 B1 R2 R5

        const auto val0 = dat0.Shuffle<2, 1, 0, 5, 4, 3, 6, 7>(); // B0 G0 R0 B1 G1 R1 R2 G2
        const auto val1 = dat1.Shuffle<0, 3, 2, 1, 6, 5, 4, 7>(); // B2 B3 G3 R3 B4 G4 R4 R5
        const auto val2 = dat2.Shuffle<0, 1, 4, 3, 2, 7, 6, 5>(); // G5 B5 B6 G6 R6 B7 G7 R7
        const auto val012 = mix012.As<U32x4>().Shuffle<3, 1, 2, 0>().As<U16x8>(); // R2 R5 B0 R1 G1 B1 B2 B5

        const auto out0 = val0.SelectWith<0b01000000>(val012); // B0 G0 R0 B1 G1 R1 B2 G2
        const auto out1 = val1.SelectWith<0b10000001>(val012); // R2 B3 G3 R3 B4 G4 R4 B5
        const auto out2 = val2.SelectWith<0b00000010>(val012); // G5 R5 B6 G6 R6 B7 G7 R7

        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct RGBToBGRf_SSSE3
{
    static constexpr size_t M = 12, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src);
        const F32x4 dat1(src + 4);
        const F32x4 dat2(src + 8);

        const auto mix01 = _mm_shuffle_ps(dat0, dat1, 0b01001100); // R0 R1 G1 B1
        const auto mix12 = _mm_shuffle_ps(dat1, dat2, 0b11001110); // R2 G2 B2 B3

        const F32x4 out0 = _mm_shuffle_ps( dat0, mix01, 0b11000110); // B0 G0 R0 B1
        const F32x4 out1 = _mm_shuffle_ps(mix01, mix12, 0b01100110); // G1 R1 B2 G2
        const F32x4 out2 = _mm_shuffle_ps(mix12,  dat2, 0b01101100); // R2 B3 G3 R3

        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
template<uint8_t Idx>
struct KeepChannel8_3_1_SSE41
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        constexpr uint16_t SelectMask[3] = { 0b0100100100100100, 0b1001001001001001, 0b0010010010010010 };
        const U8x16 dat0(src +  0);
        const U8x16 dat1(src + 16); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const U8x16 dat2(src + 32); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1
        
        const U8x16 dat01  = dat0 .SelectWith<SelectMask[(Idx + 0) % 3]>(dat1); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const U8x16 dat012 = dat01.SelectWith<SelectMask[(Idx + 2) % 3]>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const auto shuffleMask = _mm_setr_epi8(Idx + 0, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx + 15, Idx + 18, Idx + 21, Idx + 24, Idx + 27, Idx + 30, Idx + 33, Idx + 36, Idx + 39, Idx + 42, Idx + 45);
        const U8x16 out = _mm_shuffle_epi8(dat012, shuffleMask);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel16_3_1_SSE41
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        constexpr uint8_t SelectMask[3] = { 0b10010010, 0b00100100, 0b01001001 };
        const U16x8 dat0(src +  0); // RGBRGBRG
        const U16x8 dat1(src +  8); // BRGBRGBR, R=0, G=1, B=2
        const U16x8 dat2(src + 16); // GBRGBRGB, R=1, G=2, B=0

        const U16x8 dat01  = dat0 .SelectWith<SelectMask[(Idx + 0) % 3]>(dat1);
        const U16x8 dat012 = dat01.SelectWith<SelectMask[(Idx + 1) % 3]>(dat2);

        constexpr uint8_t I = Idx * 2;
        const auto shuffleMask = _mm_setr_epi8(I + 0, I + 1, I + 6, I + 7, I + 12, I + 13, I + 18, I + 19, I + 24, I + 25, I + 30, I + 31, I + 36, I + 37, I + 42, I + 43);
        const U16x8 out = _mm_shuffle_epi8(dat012, shuffleMask);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF_4_1_SSSE3
{
    static constexpr size_t M = 16, N = 4, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = F32x4(src +  0).As<U32x4>();
        const auto dat1 = F32x4(src +  4).As<U32x4>();
        const auto dat2 = F32x4(src +  8).As<U32x4>();
        const auto dat3 = F32x4(src + 12).As<U32x4>();
        
        const auto mix01 = Idx >= 2 ? dat0.ZipHi(dat1).As<U64x2>() : dat0.ZipLo(dat1).As<U64x2>();
        const auto mix23 = Idx >= 2 ? dat2.ZipHi(dat3).As<U64x2>() : dat2.ZipLo(dat3).As<U64x2>();

        const auto out = Idx % 2 ? mix01.ZipHi(mix23).As<F32x4>() : mix01.ZipLo(mix23).As<F32x4>();

        out.Save(dst);
    }
};
template<int8_t Idx>
struct KeepChannelF_3_1_SSSE3
{
    static constexpr size_t M = 12, N = 4, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src + 0); // RGBR
        const F32x4 dat1(src + 4); // GBRG
        const F32x4 dat2(src + 8); // BRGB
        F32x4 out;
        if constexpr (Idx == 0)
        {
            const auto mix12 = dat1.SelectWith<0b0010>(dat2); // x32x
            out = _mm_shuffle_ps(dat0, mix12, 0b01101100);
        }
        else if constexpr (Idx == 1)
        {
            const auto mix01 = dat0.SelectWith<0b0001>(dat1); // 10xx
            const auto mix12 = dat1.SelectWith<0b0100>(dat2); // xx32
            out = _mm_shuffle_ps(mix01, mix12, 0b10110001);
        }
        else
        {
            const auto mix01 = dat0.SelectWith<0b0010>(dat1); // x10x
            out = _mm_shuffle_ps(mix01, dat2, 0b11000110);
        }
        out.Save(dst);
    }
};
struct Extract8x2_SSSE3
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src);
        const U16x8 dat1(src + 8);

        const U8x16 mid0 = dat0.As<U8x16>().Shuffle<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>();
        const U8x16 mid1 = dat1.As<U8x16>().Shuffle<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>();

        const auto out0 = mid0.As<U64x2>().ZipLo(mid1.As<U64x2>()).As<U8x16>();
        const auto out1 = mid0.As<U64x2>().ZipHi(mid1.As<U64x2>()).As<U8x16>();

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract8x3_SSE41
{
    static constexpr size_t M = 48, N = 16, K = 16;
    static forceinline std::tuple<U8x16, U8x16, U8x16> DoLoad(const uint8_t* __restrict src) noexcept
    {
        constexpr uint16_t SelectMask[3] = { 0b0100100100100100, 0b1001001001001001, 0b0010010010010010 };
        const U8x16 dat0(src + 0); // RGBRGBRGBRGBRGBR
        const U8x16 dat1(src + 16); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const U8x16 dat2(src + 32); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const auto datR = dat0.SelectWith<SelectMask[0]>(dat1).SelectWith<SelectMask[2]>(dat2);
        const auto datG = dat0.SelectWith<SelectMask[1]>(dat1).SelectWith<SelectMask[0]>(dat2);
        const auto datB = dat0.SelectWith<SelectMask[2]>(dat1).SelectWith<SelectMask[1]>(dat2);

        const auto shuffleMaskR = _mm_setr_epi8(0, 3, 6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45);
        const auto shuffleMaskG = _mm_setr_epi8(1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46);
        const auto shuffleMaskB = _mm_setr_epi8(2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47);
        const U8x16 out0 = _mm_shuffle_epi8(datR, shuffleMaskR);
        const U8x16 out1 = _mm_shuffle_epi8(datG, shuffleMaskG);
        const U8x16 out2 = _mm_shuffle_epi8(datB, shuffleMaskB);

        return { out0, out1, out2 };
    }
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto [out0, out1, out2] = DoLoad(src);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract8x4_SSSE3
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);

        const U8x16 mid0 = dat0.As<U8x16>().Shuffle<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // 0123
        const U8x16 mid1 = dat1.As<U8x16>().Shuffle<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // 4567
        const U8x16 mid2 = dat2.As<U8x16>().Shuffle<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // 89ab
        const U8x16 mid3 = dat3.As<U8x16>().Shuffle<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // cdef

        const U32x4 val0415 = mid0.As<U32x4>().ZipLo(mid1.As<U32x4>()); // 0415
        const U32x4 val2637 = mid0.As<U32x4>().ZipHi(mid1.As<U32x4>()); // 2637
        const U32x4 val8c9d = mid2.As<U32x4>().ZipLo(mid3.As<U32x4>()); // 8c9d
        const U32x4 valaebf = mid2.As<U32x4>().ZipHi(mid3.As<U32x4>()); // aebf

        const auto out0 = val0415.As<U64x2>().ZipLo(val8c9d.As<U64x2>()).As<U8x16>(); // 048c
        const auto out1 = val0415.As<U64x2>().ZipHi(val8c9d.As<U64x2>()).As<U8x16>(); // 159d
        const auto out2 = val2637.As<U64x2>().ZipLo(valaebf.As<U64x2>()).As<U8x16>(); // 26ae
        const auto out3 = val2637.As<U64x2>().ZipHi(valaebf.As<U64x2>()).As<U8x16>(); // 37bf

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract16x2_SSSE3
{
    static constexpr size_t M = 16, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src);
        const U16x8 dat1(src + 8);

        const auto mid0 = dat0.Shuffle<0, 2, 4, 6, 1, 3, 5, 7>();
        const auto mid1 = dat1.Shuffle<0, 2, 4, 6, 1, 3, 5, 7>();
        const auto out = mid0.As<U64x2>().Zip(mid1.As<U64x2>()).As<U16x8>();
        out[0].Save(dst[0]);
        out[1].Save(dst[1]);
    }
};
struct Extract16x3_SSE41
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        constexpr uint8_t SelectMask[3] = { 0b10010010, 0b00100100, 0b01001001 };
        const U16x8 dat0(src +  0); // RGBRGBRG
        const U16x8 dat1(src +  8); // BRGBRGBR, R=0, G=1, B=2
        const U16x8 dat2(src + 16); // GBRGBRGB, R=1, G=2, B=0

        const auto datR = dat0.SelectWith<SelectMask[0]>(dat1).SelectWith<SelectMask[1]>(dat2);
        const auto datG = dat0.SelectWith<SelectMask[1]>(dat1).SelectWith<SelectMask[2]>(dat2);
        const auto datB = dat0.SelectWith<SelectMask[2]>(dat1).SelectWith<SelectMask[0]>(dat2);

        const auto shuffleMaskR = _mm_setr_epi8(0, 1,  6,  7, 12, 13, 18, 19, 24, 25, 30, 31, 36, 37, 42, 43);
        const auto shuffleMaskG = _mm_setr_epi8(2, 3,  8,  9, 14, 15, 20, 21, 26, 27, 32, 33, 38, 39, 44, 45);
        const auto shuffleMaskB = _mm_setr_epi8(4, 5, 10, 11, 16, 17, 22, 23, 28, 29, 34, 35, 40, 41, 46, 47);
        const U16x8 out0 = _mm_shuffle_epi8(datR, shuffleMaskR);
        const U16x8 out1 = _mm_shuffle_epi8(datG, shuffleMaskG);
        const U16x8 out2 = _mm_shuffle_epi8(datB, shuffleMaskB);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract16x4_SSSE3
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src +  0);
        const U16x8 dat1(src +  8);
        const U16x8 dat2(src + 16);
        const U16x8 dat3(src + 24);

        const auto mid0 = dat0.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x4>(); // 0123
        const auto mid1 = dat1.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x4>(); // 0123
        const auto mid2 = dat2.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x4>(); // 0123
        const auto mid3 = dat3.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x4>(); // 0123

        const auto mid01 = mid0.Zip(mid1).As<U64x2>(); // 0011 2233
        const auto mid23 = mid2.Zip(mid3).As<U64x2>(); // 0011 2233

        const auto out01 = mid01[0].Zip(mid23[0]).As<U16x8>(); // 01
        const auto out23 = mid01[1].Zip(mid23[1]).As<U16x8>(); // 23
        out01[0].Save(dst[0]);
        out01[1].Save(dst[1]);
        out23[0].Save(dst[2]);
        out23[1].Save(dst[3]);
    }
};
struct Extract32x2_SSSE3
{
    static constexpr size_t M = 8, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src);
        const F32x4 dat1(src + 4);

        const F32x4 out0 = _mm_shuffle_ps(dat0, dat1, 0b10001000);
        const F32x4 out1 = _mm_shuffle_ps(dat0, dat1, 0b11011101);

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract32x3_SSSE3
{
    static constexpr size_t M = 12, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x4 dat0(src + 0); // 0123
        const F32x4 dat1(src + 4); // 4567
        const F32x4 dat2(src + 8); // 89ab

        const F32x4 mid1245 = _mm_shuffle_ps(dat0, dat1, 0b01001001); // 1245
        const F32x4 mid679a = _mm_shuffle_ps(dat1, dat2, 0b10011110); // 679a

        const F32x4 out0 = _mm_shuffle_ps(   dat0, mid679a, 0b10001100); // 0369
        const F32x4 out1 = _mm_shuffle_ps(mid1245, mid679a, 0b11011000); // 147a
        const F32x4 out2 = _mm_shuffle_ps(mid1245,    dat2, 0b11001101); // 258b

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract32x4_SSSE3
{
    static constexpr size_t M = 16, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        // int vector shuffle could be more efficient on intel core
        const auto dat0 = F32x4(src +  0).As<U32x4>();
        const auto dat1 = F32x4(src +  4).As<U32x4>();
        const auto dat2 = F32x4(src +  8).As<U32x4>();
        const auto dat3 = F32x4(src + 12).As<U32x4>();

        const U64x2 valRG01 = dat0.ZipLo(dat1).As<U64x2>(); // R01 G01
        const U64x2 valBA01 = dat0.ZipHi(dat1).As<U64x2>(); // B01 A01
        const U64x2 valRG23 = dat2.ZipLo(dat3).As<U64x2>(); // R23 G23
        const U64x2 valBA23 = dat2.ZipHi(dat3).As<U64x2>(); // B23 A23

        const auto out0 = valRG01.ZipLo(valRG23).As<F32x4>(); // R0123
        const auto out1 = valRG01.ZipHi(valRG23).As<F32x4>(); // G0123
        const auto out2 = valBA01.ZipLo(valBA23).As<F32x4>(); // B0123
        const auto out3 = valBA01.ZipHi(valBA23).As<F32x4>(); // A0123

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Combine8x3_SSE41
{
    static constexpr size_t M = 16, N = 48, K = 16;
    static forceinline void DoStore(uint8_t * __restrict dst, const U8x16 & dat0, const U8x16 & dat1, const U8x16 & dat2) noexcept
    {
        // RGBRGBRGBRGBRGBR, R=1001001001001001 G=0100100100100100 B=0010010010010010
        // GBRGBRGBRGBRGBRG, R=0010010010010010 G=1001001001001001 B=0100100100100100
        // BRGBRGBRGBRGBRGB, R=0100100100100100 G=0010010010010010 B=1001001001001001
        const auto datR = dat0.Shuffle< 0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15, 10,  5>();
        const auto datG = dat1.Shuffle< 5,  0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15, 10>();
        const auto datB = dat2.Shuffle<10,  5,  0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15>();

        const auto out0 = datR.SelectWith<0b0010010010010010>(datG).SelectWith<0b0100100100100100>(datB);
        const auto out1 = datR.SelectWith<0b1001001001001001>(datG).SelectWith<0b0010010010010010>(datB);
        const auto out2 = datR.SelectWith<0b0100100100100100>(datG).SelectWith<0b1001001001001001>(datB);

        out0.Save(dst + 0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);
        const U8x16 dat2(src[2]);
        DoStore(dst, dat0, dat1, dat2);
    }
};
struct Combine16x3_SSE41
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);
        const U16x8 dat2(src[2]);
        // RGBRGBRG, R=10010010 G=01001001 B=00100100
        // BRGBRGBR, R=01001001 G=00100100 B=10010010
        // GBRGBRGB, R=00100100 G=10010010 B=01001001
        const auto datR = dat0.Shuffle<0, 3, 6, 1, 4, 7, 2, 5>();
        const auto datG = dat1.Shuffle<5, 0, 3, 6, 1, 4, 7, 2>();
        const auto datB = dat2.Shuffle<2, 5, 0, 3, 6, 1, 4, 7>();

        const auto out0 = datR.SelectWith<0b10010010>(datG).SelectWith<0b00100100>(datB);
        const auto out1 = datR.SelectWith<0b00100100>(datG).SelectWith<0b01001001>(datB);
        const auto out2 = datR.SelectWith<0b01001001>(datG).SelectWith<0b10010010>(datB);

        out0.Save(dst + 0);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
    }
};
struct Combine32x3_SSSE3
{
    static constexpr size_t M = 4, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);
        const F32x4 dat2(src[2]);

        const auto valR02G02 = _mm_shuffle_ps(dat0, dat1, 0b10001000); // R0R2 G0G2
        const auto valR13B02 = _mm_shuffle_ps(dat0, dat2, 0b10001101); // R1R3 B0B2
        const auto valG13B13 = _mm_shuffle_ps(dat1, dat2, 0b11011101); // G1G3 B1B3

        const F32x4 out0 = _mm_shuffle_ps(valR02G02, valR13B02, 0b00101000); // R0G0 B0R1
        const F32x4 out1 = _mm_shuffle_ps(valG13B13, valR02G02, 0b11011000); // G1B1 R2G2
        const F32x4 out2 = _mm_shuffle_ps(valR13B02, valG13B13, 0b11010111); // B2R3 G3B3
        
        out0.Save(dst + 0);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
struct G16ToG8_SSSE3
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat0(src);
        const U16x8 dat1(src + 8);
        const U16x8 muler((uint16_t)0xff01u); // y => x / 0xffff * 0xff => x / 257 => ((x << 24) / 257) >> 24 => (x * (1 << 24) / 257) >> 24 ~> (x * 0xff01) >> 24

        const auto mid0 = dat0.MulHi(muler);
        const auto mid1 = dat1.MulHi(muler);

        const auto val0 = mid0.As<U8x16>().Shuffle<1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14>();
        const auto val1 = mid1.As<U8x16>().Shuffle<1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14>();
        const auto out = val0.As<U64x2>().ZipLo(val1.As<U64x2>()).As<U8x16>();

        out.Save(dst);
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_SSE41 : RGB5x5ToRGB8_128Base<Is555>
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src + 0); // 01234567
        const auto dat1 = U16x8(src + 8); // 89abcdef
        
        const auto [midR0, midG0, midB0] = this->RGB5x5ToRGB8Hi(dat0);
        const auto [midR1, midG1, midB1] = this->RGB5x5ToRGB8Hi(dat1);

        const U8x16 midRB0 = (IsRGB ? midR0 : midB0).template As<U8x16>().ZipEven((IsRGB ? midB0 : midR0).template As<U8x16>());
        const U8x16 midRB1 = (IsRGB ? midR1 : midB1).template As<U8x16>().ZipEven((IsRGB ? midB1 : midR1).template As<U8x16>());

        const auto shuffle0_RB = _mm_setr_epi8(0, -1, 1, 2, -1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10);
        const auto shuffle0_G  = _mm_setr_epi8(-1, 1, -1, -1, 3, -1, -1, 5, -1, -1, 7, -1, -1, 9, -1, -1);
        const U8x16 out0R_B = _mm_shuffle_epi8(midRB0, shuffle0_RB);
        const U8x16 out0_G_ = _mm_shuffle_epi8(midG0,  shuffle0_G);
        const auto out0 = out0R_B.Or(out0_G_);

        const auto midRB01 = midRB0.As<U64x2>().SelectWith<0b01>(midRB1.As<U64x2>()); // 89ab4567
        const auto midG01 = midG0.template As<U64x2>().template SelectWith<0b01>(midG1.template As<U64x2>()); // 89ab4567
        const auto shuffle1_RB = _mm_setr_epi8(-1, 11, 12, -1, 13, 14, -1, 15, 0, -1, 1, 2, -1, 3, 4, -1);
        const auto shuffle1_G  = _mm_setr_epi8(11, -1, -1, 13, -1, -1, 15, -1, -1, 1, -1, -1, 3, -1, -1, 5);
        const U8x16 out1R_B = _mm_shuffle_epi8(midRB01, shuffle1_RB);
        const U8x16 out1_G_ = _mm_shuffle_epi8(midG01,  shuffle1_G);
        const auto out1 = out1R_B.Or(out1_G_);

        const auto shuffle2_RB = _mm_setr_epi8(5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1, 13, 14, -1, 15);
        const auto shuffle2_G  = _mm_setr_epi8(-1, -1, 7, -1, -1, 9, -1, -1, 11, -1, -1, 13, -1, -1, 15, -1);
        const U8x16 out2R_B = _mm_shuffle_epi8(midRB1, shuffle2_RB);
        const U8x16 out2_G_ = _mm_shuffle_epi8(midG1,  shuffle2_G);
        const auto out2 = out2R_B.Or(out2_G_);

        out0.Save(dst + 0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
template<bool Scale, bool IsRGB>
struct RGB10ToRGBf_SSE41
{
    static constexpr size_t M = 4, N = 12, K = 4;
    F32x4 Scaler;
    RGB10ToRGBf_SSE41(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat(src);

        const U32x4 maskLo10bit(0x3ffu);
        const auto dat0 = dat.And(maskLo10bit).As<I32x4>();
        const auto dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit).As<I32x4>();
        const auto dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit).As<I32x4>();

        F32x4 valR = (IsRGB ? dat0 : dat2).Cast<F32x4>();
        F32x4 valG = dat1.Cast<F32x4>();
        F32x4 valB = (IsRGB ? dat2 : dat0).Cast<F32x4>();
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }

        const F32x4 valRRBB = _mm_shuffle_ps(valR, valB, 0b11001100); // R0 R3 B0 B3
        const F32x4 valGGRR = _mm_shuffle_ps(valG, valR, 0b01100001); // G1 G0 R2 R1
        const F32x4 valBBGG = _mm_shuffle_ps(valB, valG, 0b10110110); // B2 B1 G3 G2

        const F32x4 out0 = valRRBB.SelectWith<0b1010>(valGGRR); // R0 G0 B0 R1
        const F32x4 out1 = valGGRR.SelectWith<0b1010>(valBBGG); // G1 B1 R2 G2
        const F32x4 out2 = valBBGG.SelectWith<0b1010>(valRRBB); // B2 R3 G3 B3
        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
    }
};
template<bool Scale, bool IsRGB, bool HasAlpha>
struct RGB10ToRGBAf_SSE41
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Scaler;
    RGB10ToRGBAf_SSE41(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat(src);

        const U32x4 maskLo10bit(0x3ffu);
        const I32x4 dat0 = dat.And(maskLo10bit).As<I32x4>();
        const I32x4 dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit).As<I32x4>();
        const I32x4 dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit).As<I32x4>();

        F32x4 valR = (IsRGB ? dat0 : dat2).Cast<F32x4>();
        F32x4 valG = dat1.Cast<F32x4>();
        F32x4 valB = (IsRGB ? dat2 : dat0).Cast<F32x4>();
        F32x4 valA;
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }
        if constexpr (HasAlpha)
        {
#if COMMON_SIMD_LV >= 100
            const auto alphaLUT = _mm_setr_ps(0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f);
            valA = _mm_permutevar_ps(alphaLUT, dat.ShiftRightLogic<30>());
#else
            valA = dat.ShiftRightLogic<30>().As<I32x4>().Cast<F32x4>().Mul(1.0f / 3.0f);
#endif
        }
        else
        {
            valA = F32x4(1.0f);
        }

        const auto valRG01 = valR.ZipLo(valG); // R0 G0 R1 G1
        const auto valRG23 = valR.ZipHi(valG); // R2 G2 R3 G3
        const auto valBA01 = valB.ZipLo(valA); // B0 A0 B1 A1
        const auto valBA23 = valB.ZipHi(valA); // B2 A2 B3 A3

        // integer xmm could have higher throughput on Intel Core
        const auto out0 = valRG01.As<U64x2>().ZipLo(valBA01.As<U64x2>()).As<F32x4>();
        const auto out1 = valRG01.As<U64x2>().ZipHi(valBA01.As<U64x2>()).As<F32x4>();
        const auto out2 = valRG23.As<U64x2>().ZipLo(valBA23.As<U64x2>()).As<F32x4>();
        const auto out3 = valRG23.As<U64x2>().ZipHi(valBA23.As<U64x2>()).As<F32x4>();
        out0.Save(dst);
        out1.Save(dst + 4);
        out2.Save(dst + 8);
        out3.Save(dst + 12);
    }
};

struct RGB8ToYUV8_F32_SSE41
{
    static constexpr size_t M = 48, N = 48, K = 16;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
        alignas(32) static inline const uint8_t Shuf[] = { R + 0, 255, 255, 255, G + 0, 255, 255, 255, B + 0, 255, 255, 255, 255, 255, 255, 255 };
    };
    __m128i U8ToI32x4Shuf;
    F32x4 LutY, LutCb, LutCr;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_F32_SSE41(const std::array<float, 9>& lut, RGBOrder<R, G, B>) noexcept :
        U8ToI32x4Shuf(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::Shuf))), LutY(lut.data()), LutCb(lut.data() + 3), LutCr(lut.data() + 5)
    { }
    static forceinline auto DP4(const F32x4& src0, const F32x4& src1, const F32x4& src2, const F32x4& src3, const F32x4& lut) noexcept
    {
        const auto mid0 = src0.Dot<DotPos::XYZ, DotPos::X>(lut);
        const auto mid1 = src1.Dot<DotPos::XYZ, DotPos::Y>(lut);
        const auto mid2 = src2.Dot<DotPos::XYZ, DotPos::Z>(lut);
        const auto mid3 = src3.Dot<DotPos::XYZ, DotPos::W>(lut);
        return _mm_cvtps_epi32(mid0.Or(mid1).Or(mid2.Or(mid3)));
    }
    template<bool ClampC>
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        const F32x4 mid0  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0,                U8ToI32x4Shuf));
        const F32x4 mid1  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0.MoveToLo< 3>(), U8ToI32x4Shuf));
        const F32x4 mid2  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0.MoveToLo< 6>(), U8ToI32x4Shuf));
        const F32x4 mid3  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0.MoveToLo< 9>(), U8ToI32x4Shuf));
        const F32x4 mid4  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0.MoveToLo<12>(), U8ToI32x4Shuf));
        const F32x4 mid5  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat0.MoveToLoWith<15>(dat1), U8ToI32x4Shuf));
        const F32x4 mid6  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat1.MoveToLo< 2>(), U8ToI32x4Shuf));
        const F32x4 mid7  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat1.MoveToLo< 5>(), U8ToI32x4Shuf));
        const F32x4 mid8  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat1.MoveToLo< 8>(), U8ToI32x4Shuf));
        const F32x4 mid9  = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat1.MoveToLo<11>(), U8ToI32x4Shuf));
        const F32x4 mid10 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat1.MoveToLoWith<14>(dat2), U8ToI32x4Shuf));
        const F32x4 mid11 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat2.MoveToLo< 1>(), U8ToI32x4Shuf));
        const F32x4 mid12 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat2.MoveToLo< 4>(), U8ToI32x4Shuf));
        const F32x4 mid13 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat2.MoveToLo< 7>(), U8ToI32x4Shuf));
        const F32x4 mid14 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat2.MoveToLo<10>(), U8ToI32x4Shuf));
        const F32x4 mid15 = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat2.MoveToLo<13>(), U8ToI32x4Shuf));
#define DP4x4(type, out, lut) const type out##0 = DP4(mid0, mid1, mid2, mid3, lut), out##1 = DP4(mid4, mid5, mid6, mid7, lut), \
    out##2 = DP4(mid8, mid9, mid10, mid11, lut), out##3 = DP4(mid12, mid13, mid14, mid15, lut)
        DP4x4(U8x16, lumi4i, LutY);
        // only lowest byte should be used
        U8x16 lumiAll = lumi4i0.Or(lumi4i1.MoveToHi<1>()).Or(lumi4i2.MoveToHi<2>().Or(lumi4i3.MoveToHi<3>()));
        if (needAddY)
            lumiAll = lumiAll.Add(16);
        const auto outY = lumiAll.Shuffle<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>();

        DP4x4(U16x8, cb4i, LutCb);
        DP4x4(U16x8, cr4i, LutCr);
#undef DP4x4
        U8x16 outCb, outCr;
        if constexpr (ClampC) // keep 16bit in case of overflow
        {
            I16x8 cAdd(128);
            I16x8 cbi0 = _mm_packs_epi32(cb4i0, cb4i1), cbi1 = _mm_packs_epi32(cb4i2, cb4i3);
            cbi0 = cbi0.Add(cAdd), cbi1 = cbi1.Add(cAdd);
            outCb = _mm_packus_epi16(cbi0, cbi1);
            I16x8 cri0 = _mm_packs_epi32(cr4i0, cr4i1), cri1 = _mm_packs_epi32(cr4i2, cr4i3);
            cri0 = cri0.Add(cAdd), cri1 = cri1.Add(cAdd);
            outCr = _mm_packus_epi16(cri0, cri1);
        }
        else // only lowest byte should be used
        {
            // 0x4x 1x5x 2x6x 3x7x, 8xcx 9xdx axex bxfx
            const auto cbi01 = cb4i0.ZipOdd(cb4i1).As<U8x16>(), cbi23 = cb4i2.ZipOdd(cb4i3).As<U8x16>();
            const auto cri01 = cr4i0.ZipOdd(cr4i1).As<U8x16>(), cri23 = cr4i2.ZipOdd(cr4i3).As<U8x16>();
            // 084c 195d 2a6e 3b7f
            const auto cbAll = cbi01.ZipOdd(cbi23).Add(128);
            const auto crAll = cri01.ZipOdd(cri23).Add(128);
            outCb = cbAll.Shuffle<0, 4, 8, 12, 2, 6, 10, 14, 1, 5, 9, 13, 3, 7, 11, 15>();
            outCr = crAll.Shuffle<0, 4, 8, 12, 2, 6, 10, 14, 1, 5, 9, 13, 3, 7, 11, 15>();
        }

        Combine8x3_SSE41::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I16_SSE41_Base
{
    static constexpr size_t M = 48, N = 48, K = 16;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
        alignas(32) static inline const uint8_t ShufRB[] = { R +  0, 255, B +  0, 255, R +  3, 255, B +  3, 255, R +  6, 255, B +  6, 255, R +  9, 255, B +  9, 255 };
        alignas(32) static inline const uint8_t ShufG0[] = { G +  0, 255, G +  3, 255, G +  6, 255, G +  9, 255, G + 12, 255, G + 15, 255, G + 18, 255, G + 21, 255 };
        alignas(32) static inline const uint8_t ShufG1[] = { G + 24, 255, G + 27, 255, G + 30, 255, G + 33, 255, G + 36, 255, G + 39, 255, G + 42, 255, G + 45, 255 };
    };
    __m128i U8ToRBShuf, U8ToGShuf0, U8ToGShuf1;
    __m128i LutYRB, LutYG, LutCbRB, LutCbG, LutCrRB, LutCrG;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I16_SSE41_Base(const I16x8& lut, RGBOrder<R, G, B>) noexcept :
        U8ToRBShuf(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufRB))),
        U8ToGShuf0(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufG0))),
        U8ToGShuf1(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufG1))),
        LutYRB (lut.Shuffle<0, 2, 0, 2, 0, 2, 0, 2>()),
        LutYG  (lut.Shuffle<1, 1, 1, 1, 1, 1, 1, 1>()),
        LutCbRB(lut.Shuffle<3, 5, 3, 5, 3, 5, 3, 5>()),
        LutCbG (lut.Shuffle<4, 4, 4, 4, 4, 4, 4, 4>()),
        LutCrRB(lut.Shuffle<5, 7, 5, 7, 5, 7, 5, 7>()),
        LutCrG (lut.Shuffle<6, 6, 6, 6, 6, 6, 6, 6>())
    { }
};
struct RGB8ToYUV8_I16_SSE41 : public RGB8ToYUV8_I16_SSE41_Base
{
    using RGB8ToYUV8_I16_SSE41_Base::RGB8ToYUV8_I16_SSE41_Base;
    static forceinline auto DP3(const I16x8& rbLo, const I16x8& rbHi, const I16x8& g, const __m128i& lutRB, const __m128i& lutG) noexcept
    {
        const auto midRB0 = _mm_mulhrs_epi16(rbLo, lutRB);
        const auto midRB1 = _mm_mulhrs_epi16(rbHi, lutRB);
        const I16x8 midG  = _mm_mulhrs_epi16(g,    lutG );
        const I16x8 midRB = _mm_hadd_epi16(midRB0, midRB1);
        const auto sum = midRB.Add(midG.Add(32)).ShiftRightArith<7 + 14 - 15>();
        return sum;
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        const I16x8 midRB0 = _mm_slli_epi16(_mm_shuffle_epi8(dat0, U8ToRBShuf), 7);
        const I16x8 midRB1 = _mm_slli_epi16(_mm_shuffle_epi8(dat0.MoveToLoWith<12>(dat1), U8ToRBShuf), 7);
        const I16x8 midRB2 = _mm_slli_epi16(_mm_shuffle_epi8(dat1.MoveToLoWith< 8>(dat2), U8ToRBShuf), 7);
        const I16x8 midRB3 = _mm_slli_epi16(_mm_shuffle_epi8(dat2.MoveToLo<4>(), U8ToRBShuf), 7);
        const U8x16 datG = dat0.SelectWith<0b1001001001001001>(dat1).SelectWith<0b0100100100100100>(dat2);
        const I16x8 midG0 = _mm_slli_epi16(_mm_shuffle_epi8(datG, U8ToGShuf0), 7);
        const I16x8 midG1 = _mm_slli_epi16(_mm_shuffle_epi8(datG, U8ToGShuf1), 7);

#define DP3x4(out, lut) const auto out##0 = DP3(midRB0, midRB1, midG0, lut##RB, lut##G), out##1 = DP3(midRB2, midRB3, midG1, lut##RB, lut##G)
        DP3x4(lumi, LutY);
        DP3x4(cb_, LutCb);
        DP3x4(cr_, LutCr);
#undef DP3x4
        U8x16 outY = _mm_packus_epi16(lumi0, lumi1);
        if (needAddY)
            outY = outY.Add(16);
        const U8x16 outCb = cb_0.Cast<I8x16, CastMode::RangeSaturate>(cb_1).As<U8x16>().Add(128);
        const U8x16 outCr = cr_0.Cast<I8x16, CastMode::RangeSaturate>(cr_1).As<U8x16>().Add(128);

        Combine8x3_SSE41::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I16_2_SSE41 : public RGB8ToYUV8_I16_SSE41_Base
{
    using RGB8ToYUV8_I16_SSE41_Base::RGB8ToYUV8_I16_SSE41_Base;
    static forceinline auto DP3(const I16x8& rbLo, const I16x8& rbHi, const I16x8& g, const __m128i& lutRB, const __m128i& lutG) noexcept
    {
        const auto ShufMid16 = _mm_setr_epi8(1, 2, 5, 6, 9, 10, 13, 14, 1, 2, 5, 6, 9, 10, 13, 14);
        const auto midRB0 = _mm_madd_epi16(rbLo, lutRB);
        const auto midRB1 = _mm_madd_epi16(rbHi, lutRB);
        const I16x8 midRB = _mm_blend_epi16(_mm_shuffle_epi8(midRB0, ShufMid16), _mm_shuffle_epi8(midRB1, ShufMid16), 0b11110000); // << (14 - 8)=6
        const I16x8 midG  = _mm_mulhrs_epi16(g, lutG); // << (7 + 14 - 15)=6
        const auto sum = midRB.Add(midG.Add(32)).ShiftRightArith<6>();
        return sum;
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        const I16x8 midRB0 = _mm_shuffle_epi8(dat0, U8ToRBShuf);
        const I16x8 midRB1 = _mm_shuffle_epi8(dat0.MoveToLoWith<12>(dat1), U8ToRBShuf);
        const I16x8 midRB2 = _mm_shuffle_epi8(dat1.MoveToLoWith< 8>(dat2), U8ToRBShuf);
        const I16x8 midRB3 = _mm_shuffle_epi8(dat2.MoveToLo<4>(), U8ToRBShuf);
        const U8x16 datG = dat0.SelectWith<0b1001001001001001>(dat1).SelectWith<0b0100100100100100>(dat2);
        const I16x8 midG0 = _mm_slli_epi16(_mm_shuffle_epi8(datG, U8ToGShuf0), 7);
        const I16x8 midG1 = _mm_slli_epi16(_mm_shuffle_epi8(datG, U8ToGShuf1), 7);

#define DP3x4(out, lut) const auto out##0 = DP3(midRB0, midRB1, midG0, lut##RB, lut##G), out##1 = DP3(midRB2, midRB3, midG1, lut##RB, lut##G)
        DP3x4(lumi, LutY);
        DP3x4(cb_, LutCb);
        DP3x4(cr_, LutCr);
#undef DP3x4
        U8x16 outY = _mm_packus_epi16(lumi0, lumi1);
        if (needAddY)
            outY = outY.Add(16);
        const U8x16 outCb = cb_0.Cast<I8x16, CastMode::RangeSaturate>(cb_1).As<U8x16>().Add(128);
        const U8x16 outCr = cr_0.Cast<I8x16, CastMode::RangeSaturate>(cr_1).As<U8x16>().Add(128);

        Combine8x3_SSE41::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I8_SSE41
{
    static constexpr size_t M = 48, N = 48, K = 16;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
#define PUT4(a,b,c,d,i) a + i, b + i, c + i, d + i 
#define SHUF16(a,b,c,d) alignas(32) static inline const uint8_t Shuf##a##b##c##d [] = { PUT4(a,b,c,d,0), PUT4(a,b,c,d,3), PUT4(a,b,c,d,6), PUT4(a,b,c,d,9) }
        SHUF16(R, G, G, B);
        SHUF16(R, G, R, B);
        SHUF16(R, B, G, B);
#undef SHUF16
#undef PUT4
    };
    __m128i ShufRGGB, ShufRGRB, ShufRBGB;
    __m128i LutY, LutCb, LutCr;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I8_SSE41(const int8_t* lut, RGBOrder<R, G, B>) noexcept :
        ShufRGGB(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufRGGB))),
        ShufRGRB(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufRGRB))),
        ShufRBGB(_mm_loadu_si128(reinterpret_cast<const __m128i*>(Shuffler<R, G, B>::ShufRBGB))),
        LutY (_mm_set1_epi32(reinterpret_cast<const int32_t*>(lut)[0])),
        LutCb(_mm_set1_epi32(reinterpret_cast<const int32_t*>(lut)[1])),
        LutCr(_mm_set1_epi32(reinterpret_cast<const int32_t*>(lut)[2]))
    { }
    template<bool IsY>
    static forceinline auto DP4(const U8x16& rgb0, const U8x16& rgb1, const __m128i& lut) noexcept
    {
        const auto mid0 = _mm_maddubs_epi16(rgb0, lut), mid1 = _mm_maddubs_epi16(rgb1, lut);
        const I16x8 sum = _mm_hadd_epi16(mid0, mid1);
        if constexpr (IsY)
            return sum.As<U16x8>().Add(128).ShiftRightLogic<8>();
        else
            return sum.SatAdd(128).ShiftRightArith<8>();
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        const U8x16 part0 = dat0, part1 = dat0.MoveToLoWith<12>(dat1), part2 = dat1.MoveToLoWith< 8>(dat2), part3 = dat2.MoveToLo<4>();

#define DP4x4(out, isy, sm, lut) const auto out##0 = DP4<isy>(part0.Shuffle(Shuf##sm), part1.Shuffle(Shuf##sm), lut), out##1 = DP4<isy>(part2.Shuffle(Shuf##sm), part3.Shuffle(Shuf##sm), lut)
        DP4x4(lumi, true, RGGB, LutY);
        DP4x4(cb,  false, RBGB, LutCb);
        DP4x4(cr,  false, RGRB, LutCr);
#undef DP4x4
        U8x16 outY = _mm_packus_epi16(lumi0, lumi1);
        if (needAddY)
            outY = outY.Add(16);
        const U8x16 outCb = cb0.Cast<I8x16, CastMode::RangeSaturate>(cb1).As<U8x16>().Add(128);
        const U8x16 outCr = cr0.Cast<I8x16, CastMode::RangeSaturate>(cr1).As<U8x16>().Add(128);

        Combine8x3_SSE41::DoStore(dst, outY, outCb, outCr);
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, SSE41_F32)
{
    if (count >= RGB8ToYUV8_F32_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_F32_SSE41 proc(RGB8ToYCC8LUTF32[mval], RGBOrder<0, 1, 2>{});
        common::RegionRounding rd(common::RoundMode::ToNearest);
        do
        {
            if (isYCCFull)
                proc.Convert<true>(dest, src, needAddY);
            else
                proc.Convert<false>(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, SSE41_I16)
{
    if (count >= RGB8ToYUV8_I16_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16_SSE41 proc(I16x8(RGB8ToYCC8LUTI16[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, SSE41_I16_2)
{
    if (count >= RGB8ToYUV8_I16_2_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16_2_SSE41 proc(I16x8(RGB8ToYCC8LUTI16[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, SSE41_I8)
{
    if (count >= RGB8ToYUV8_I8_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8_SSE41 proc(RGB8ToYCC8LUTI8x4[mval].data(), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}

struct YUV8ToRGB8_F32_SSE41
{
    static constexpr size_t M = 48, N = 48, K = 16;
    F32x4 MulY, MulCb, MulCr, MulGCb, MulGCr, SubR, AddG, SubB;
    YUV8ToRGB8_F32_SSE41(const std::array<float, 9>& lut) noexcept :
        MulY(lut[0]), MulCb(lut[7]), MulCr(lut[2]), MulGCb(lut[4]), MulGCr(lut[5]),
        SubR(MulCr.Mul(128.f)), AddG(MulGCb.Add(MulGCr).Mul(128.f)), SubB(MulCb.Mul(128.f))
    { }
    forceinline std::tuple<__m128i, __m128i, __m128i> Calc(const F32x4& y, const F32x4& cb, const F32x4& cr) const noexcept
    {
        const auto r = cr.Mul(MulCr).Add(y).Sub(SubR);
        const auto b = cb.Mul(MulCb).Add(y).Sub(SubB);
        const auto g = y.Sub(cb.Mul(MulGCb)).Sub(cr.Mul(MulGCr)).Add(AddG);
        return { _mm_cvtps_epi32(r), _mm_cvtps_epi32(g), _mm_cvtps_epi32(b) };
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY, bool isRGBFull, bool isRGB) const noexcept
    {
        // RGBRGBRGBRGBRGBR
        // GBRGBRGBRGBRGBRG
        // BRGBRGBRGBRGBRGB
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        // 0b61c72d83e94fa5
        U8x16 datY  = dat0.SelectWith<0b0100100100100100>(dat1).SelectWith<0b0010010010010010>(dat2);
        U8x16 datCb = dat0.SelectWith<0b1001001001001001>(dat1).SelectWith<0b0100100100100100>(dat2);
        U8x16 datCr = dat0.SelectWith<0b0010010010010010>(dat1).SelectWith<0b1001001001001001>(dat2);
        if (needAddY)
            datY = datY.Sub(16);
        // datCb = datCb.Sub(128), datCr = datCr.Sub(128);
#define P84(x) x, -1, -1, -1
#define Shuf84(n,a,b,c,d) const auto shuf##n = _mm_setr_epi8(P84(a), P84(b), P84(c), P84(d))
        Shuf84(Y0,  0,  3,  6,  9);
        Shuf84(Y1, 12, 15, 18, 21);
        Shuf84(Y2, 24, 27, 30, 33);
        Shuf84(Y3, 36, 39, 42, 45);
#undef  Shuf84
#undef  P84
        const auto add1 = _mm_set1_epi8(1);
        const auto shufCb0 = _mm_adds_epu8(shufY0, add1), shufCb1 = _mm_adds_epu8(shufY1, add1), shufCb2 = _mm_adds_epu8(shufY2, add1), shufCb3 = _mm_adds_epu8(shufY3, add1);
        const auto shufCr0 = _mm_adds_epu8(shufCb0, add1), shufCr1 = _mm_adds_epu8(shufCb1, add1), shufCr2 = _mm_adds_epu8(shufCb2, add1), shufCr3 = _mm_adds_epu8(shufCb3, add1);
#define U8ToF32(n,x,i) n##i = _mm_cvtepi32_ps(_mm_shuffle_epi8(dat##x, shuf##x##i))
#define U8ToF32x4(n,x) F32x4 U8ToF32(n,x,0), U8ToF32(n,x,1), U8ToF32(n,x,2), U8ToF32(n,x,3)
        U8ToF32x4( y,  Y);
        U8ToF32x4(cb, Cb);
        U8ToF32x4(cr, Cr);
#undef  U8ToF32x4
        y0 = y0.Mul(MulY), y1 = y1.Mul(MulY), y2 = y2.Mul(MulY), y3 = y3.Mul(MulY);

#define CalcRGB(x) const auto [r##x, g##x, b##x] = Calc(y##x, cb##x, cr##x)
        CalcRGB(0);
        CalcRGB(1);
        CalcRGB(2);
        CalcRGB(3);
#undef  CalcRGB

#define Combine4(x) auto x = _mm_packus_epi16(_mm_packs_epi32(x##0, x##1), _mm_packs_epi32(x##2, x##3))
        Combine4(r);
        Combine4(g);
        Combine4(b);
#undef  Combine4
        if (!isRGBFull)
        {
            const auto rgbMin = _mm_set1_epi8(16), rgbMax = _mm_set1_epi8(static_cast<char>(235));
#define ClampRGB(x) x = _mm_min_epu8(_mm_max_epu8(x, rgbMin), rgbMax)
            ClampRGB(r);
            ClampRGB(g);
            ClampRGB(b);
#undef  ClampRGB
        }
        if (isRGB)
            Combine8x3_SSE41::DoStore(dst, r, g, b);
        else
            Combine8x3_SSE41::DoStore(dst, b, g, r);
    }
};
template<typename T>
struct YUV8ToRGB8_I16_SSE41_Base
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY, bool isRGBFull, bool isRGB) const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        // RGBRGBRGBRGBRGBR
        // GBRGBRGBRGBRGBRG
        // BRGBRGBRGBRGBRGB
        const U8x16 dat0(src), dat1(src + 16), dat2(src + 32);
        // 0b61c72d83e94fa5
        U8x16 datY  = dat0.SelectWith<0b0100100100100100>(dat1).SelectWith<0b0010010010010010>(dat2);
        U8x16 datCb = dat0.SelectWith<0b1001001001001001>(dat1).SelectWith<0b0100100100100100>(dat2);
        U8x16 datCr = dat0.SelectWith<0b0010010010010010>(dat1).SelectWith<0b1001001001001001>(dat2);
        if (needAddY)
            datY = datY.Sub(16);
        datCb = datCb.Sub(128), datCr = datCr.Sub(128);
#define Shuf82(n,x,a) const auto n = _mm_shuffle_epi8(dat##x, _mm_setr_epi8(a, -1, a+3, -1, a+6, -1, a+9, -1, a+12, -1, a+15, -1, a+18, -1, a+21, -1))
        Shuf82(y_0, Y,  0);
        Shuf82(y_1, Y, 24);
#undef  Shuf82
        // (7 + x) - 15
        const I16x8 y0 = _mm_mulhrs_epi16(_mm_slli_epi16(y_0, 7), self.MulY);
        const I16x8 y1 = _mm_mulhrs_epi16(_mm_slli_epi16(y_1, 7), self.MulY);
#define Shuf82(n,x,a) const auto n = _mm_shuffle_epi8(dat##x, _mm_setr_epi8(-1, a, -1, a+6, -1, a+12, -1, a+18, -1, a+24, -1, a+30, -1, a+36, -1, a+42))
        Shuf82(cb_e, Cb, 1);
        Shuf82(cb_o, Cb, 4);
        Shuf82(cr_e, Cr, 2);
        Shuf82(cr_o, Cr, 5);
#undef  Shuf82
        const auto cbr0e = _mm_unpacklo_epi16(cb_e, cr_e); // 0246
        const auto cbr0o = _mm_unpacklo_epi16(cb_o, cr_o); // 1357
        const auto cbr1e = _mm_unpackhi_epi16(cb_e, cr_e); // 8ace
        const auto cbr1o = _mm_unpackhi_epi16(cb_o, cr_o); // 9bdf

#define CalcRGB(x) const auto [r##x, g##x, b##x] = self.Calc(y##x, cbr##x##e, cbr##x##o)
        CalcRGB(0);
        CalcRGB(1);
#undef  CalcRGB

#define Combine4(x) auto x = _mm_packus_epi16(x##0, x##1)
        Combine4(r);
        Combine4(g);
        Combine4(b);
#undef  Combine4
        if (!isRGBFull)
        {
            const auto rgbMin = _mm_set1_epi8(16), rgbMax = _mm_set1_epi8(static_cast<char>(235));
#define ClampRGB(x) x = _mm_min_epu8(_mm_max_epu8(x, rgbMin), rgbMax)
            ClampRGB(r);
            ClampRGB(g);
            ClampRGB(b);
#undef  ClampRGB
        }
        if (isRGB)
            Combine8x3_SSE41::DoStore(dst, r, g, b);
        else
            Combine8x3_SSE41::DoStore(dst, b, g, r);
    }
};
struct YUV8ToRGB8_I16_SSE41 : public YUV8ToRGB8_I16_SSE41_Base<YUV8ToRGB8_I16_SSE41>
{
    I16x8 MulY, MulR, MulB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_SSE41(const uint32_t* lut) noexcept : MulY(static_cast<int16_t>(lut[0] >> 1)),
        MulR(U32x4(lut[1] & 0xffff0000u).As<I16x8>()),
        MulB(U32x4(lut[1] & 0x0000ffffu).As<I16x8>()),
        MulG(U32x4(lut[2]).As<I16x8>())
    { }
    forceinline std::tuple<__m128i, __m128i, __m128i> Calc(const I16x8& y5, const __m128i& cbrE, const __m128i& cbrO) const noexcept
    {
        const I16x8 rE = _mm_madd_epi16(cbrE, MulR), rO = _mm_madd_epi16(cbrO, MulR);
        const I16x8 gE = _mm_madd_epi16(cbrE, MulG), gO = _mm_madd_epi16(cbrO, MulG);
        const I16x8 bE = _mm_madd_epi16(cbrE, MulB), bO = _mm_madd_epi16(cbrO, MulB);
        // (8 + 13) - 16 = 5
        const auto r5 = rE.ZipEven(rO), g5 = gE.ZipEven(gO), b5 = bE.ZipEven(bO);
        const auto y = y5.Add(16); // add rounding
        const auto r = r5.Add(y).ShiftRightArith<5>(), b = b5.Add(y).ShiftRightArith<5>();
        const auto g = y.Sub(g5).ShiftRightArith<5>();
        return { r, g, b };
    }
};
struct YUV8ToRGB8_I16_2_SSE41 : public YUV8ToRGB8_I16_SSE41_Base<YUV8ToRGB8_I16_2_SSE41>
{
    I16x8 MulY, MulRB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_2_SSE41(const uint32_t* lut) noexcept : MulY(static_cast<int16_t>(lut[0])),
        MulRB(U32x4(lut[1]).As<I16x8>()),
        MulG(U32x4(lut[2]).As<I16x8>())
    { }
    forceinline std::tuple<__m128i, __m128i, __m128i> Calc(const I16x8& y6, const __m128i& cbrE, const __m128i& cbrO) const noexcept
    {
        // (8 + 13) - 16 = 5
        const I16x8 gE = _mm_madd_epi16(cbrE, MulG), gO = _mm_madd_epi16(cbrO, MulG);
        // (8 + 13) - 15 = 6
        const I16x8 rbE = _mm_mulhrs_epi16(cbrE, MulRB), rbO = _mm_mulhrs_epi16(cbrO, MulRB);
        const auto r6 = rbE.ZipEven(rbO), b6 = rbE.ZipOdd(rbO);
        const auto g5 = gE.ZipEven(gO);
        const auto yrb = y6.Add(32); // add rounding
        const auto r = r6.Add(yrb).ShiftRightArith<6>(), b = b6.Add(yrb).ShiftRightArith<6>();
        const auto yg = yrb.ShiftRightArith<1>();
        const auto g = yg.Sub(g5).ShiftRightArith<5>();
        return { r, g, b };
    }
};
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, SSE41_F32)
{
    if (count >= YUV8ToRGB8_F32_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_F32_SSE41 proc(YCC8ToRGB8LUTF32[mval]);
        common::RegionRounding rd(common::RoundMode::ToNearest);
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, SSE41_I16)
{
    if (count >= YUV8ToRGB8_I16_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_SSE41 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, SSE41_I16_2)
{
    if (count >= YUV8ToRGB8_I16_2_SSE41::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_2_SSE41 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}

DEFINE_FASTPATH_METHOD(G8ToRGBA8, SSSE3)
{
    ProcessLOOP4<G2RGBA_SSSE3, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, SSSE3)
{
    ProcessLOOP4<RGB8_3To4_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, SSSE3)
{
    ProcessLOOP4<RGB8_3To4_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, SSSE3)
{
    ProcessLOOP4<RGB8_4To3_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, SSSE3)
{
    ProcessLOOP4<RGB8_4To3_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToRGBA16, SSSE3)
{
    ProcessLOOP4<RGB16_3To4_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR16ToRGBA16, SSSE3)
{
    ProcessLOOP4<RGB16_3To4_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA16ToRGB16, SSSE3)
{
    ProcessLOOP4<RGB16_4To3_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGR16, SSSE3)
{
    ProcessLOOP4<RGB16_4To3_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, SSSE3)
{
    ProcessLOOP4<RGBf_3To4_SSSE3<true>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, SSSE3)
{
    ProcessLOOP4<RGBf_3To4_SSSE3<false>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, SSE41)
{
    ProcessLOOP4<RGBf_4To3_SSE41<true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, SSE41)
{
    ProcessLOOP4<RGBf_4To3_SSE41<false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, SSSE3)
{
    ProcessLOOP4<RGBToBGR8_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToBGR16, SSE41)
{
    ProcessLOOP4<RGBToBGR16_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, SSSE3)
{
    ProcessLOOP4<RGBToBGRf_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, SSE41)
{
    ProcessLOOP4<KeepChannel8_3_1_SSE41<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, SSE41)
{
    ProcessLOOP4<KeepChannel8_3_1_SSE41<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, SSE41)
{
    ProcessLOOP4<KeepChannel8_3_1_SSE41<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToR16, SSE41)
{
    ProcessLOOP4<KeepChannel16_3_1_SSE41<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToG16, SSE41)
{
    ProcessLOOP4<KeepChannel16_3_1_SSE41<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToB16, SSE41)
{
    ProcessLOOP4<KeepChannel16_3_1_SSE41<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_4_1_SSSE3<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_4_1_SSSE3<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_4_1_SSSE3<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_4_1_SSSE3<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_3_1_SSSE3<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_3_1_SSSE3<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, SSSE3)
{
    ProcessLOOP4<KeepChannelF_3_1_SSSE3<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, SSSE3)
{
    ExtractLOOP4<2, Extract8x2_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, SSE41)
{
    ExtractLOOP4<3, Extract8x3_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, SSSE3)
{
    ExtractLOOP4<4, Extract8x4_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x2, SSSE3)
{
    ExtractLOOP4<2, Extract16x2_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x3, SSE41)
{
    ExtractLOOP4<3, Extract16x3_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x4, SSSE3)
{
    ExtractLOOP4<4, Extract16x4_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x2, SSSE3)
{
    ExtractLOOP4<2, Extract32x2_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x3, SSSE3)
{
    ExtractLOOP4<3, Extract32x3_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x4, SSSE3)
{
    ExtractLOOP4<4, Extract32x4_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, SSE41)
{
    CombineLOOP4<3, Combine8x3_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x3, SSE41)
{
    CombineLOOP4<3, Combine16x3_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, SSSE3)
{
    CombineLOOP4<3, Combine32x3_SSSE3, &Func<LOOP>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(G16ToG8, SSSE3)
{
    ProcessLOOP4<G16ToG8_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, SSE41)
{
    ProcessLOOP4<RGB5x5ToRGB8_SSE41<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, SSE41)
{
    ProcessLOOP4<RGB5x5ToRGB8_SSE41<false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, SSE41)
{
    ProcessLOOP4<RGB5x5ToRGB8_SSE41<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, SSE41)
{
    ProcessLOOP4<RGB5x5ToRGB8_SSE41<false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_SSE41<false, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_SSE41<true, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_SSE41<false, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_SSE41<true, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBAf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, true, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, true, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBAf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, false, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, false, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, SSE41)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
# if COMMON_ARCH_X86
#   define ALGO SSSE3
# else
#   define ALGO SIMD128
# endif
struct PPCAT(GA2RGB_, ALGO)
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src).As<U8x16>(); // 0011223344556677
        const auto dat1 = U16x8(src + 8).As<U8x16>(); // 8899aabbccddeeff
        const auto out0 = dat0.Shuffle<0, 0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10>(); // 0001112223334445
        const auto mid01 = dat0.MoveToLoWith<10>(dat1); // 5566778899aabbcc
        const auto out1 = mid01.Shuffle<0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10, 10>(); // 55666777888999aa
        const auto out2 = dat1.Shuffle<4, 6, 6, 6, 8, 8, 8, 10, 10, 10, 12, 12, 12, 14, 14, 14>(); // abbbcccdddeeefff
        out0.Save(dst);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
DEFINE_FASTPATH_METHOD(GA8ToRGB8, ALGO)
{
    ProcessLOOP4<PPCAT(GA2RGB_, ALGO), &Func<LOOP>>(dest, src, count);
}
struct PPCAT(GA2RGBA_, ALGO)
{
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint32_t * __restrict dst, const uint16_t * __restrict src) const noexcept
    {
        const auto dat = U16x8(src).As<U8x16>();
        const auto out0 = dat.Shuffle<0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7>();
        const auto out1 = dat.Shuffle<8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12, 13, 14, 14, 14, 15>();
        out0.As<U32x4>().Save(dst);
        out1.As<U32x4>().Save(dst + 4);
    }
};
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, ALGO)
{
    ProcessLOOP4<PPCAT(GA2RGBA_, ALGO), &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20
struct G2GA8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2GA8_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        uint8x16x2_t out2;
        out2.val[0] = dat;
        out2.val[1] = Alpha;
        vst2q_u8(reinterpret_cast<uint8_t*>(dst), out2);
    }
};
struct G2RGB8_NEON
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        uint8x16x3_t out3;
        out3.val[0] = dat;
        out3.val[1] = dat;
        out3.val[2] = dat;
        vst3q_u8(dst, out3);
    }
};
struct G2RGBA8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2RGBA8_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        uint8x16x4_t out4;
        out4.val[0] = dat;
        out4.val[1] = dat;
        out4.val[2] = dat;
        out4.val[3] = Alpha;
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
struct GA2RGB8_NEON
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x3_t out3;
        out3.val[0] = dat.val[0];
        out3.val[1] = dat.val[0];
        out3.val[2] = dat.val[0];
        vst3q_u8(dst, out3);
    }
};
struct GA2RGBA8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t * __restrict dst, const uint16_t * __restrict src) const noexcept
    {
        const auto dat = vld2q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x4_t out4;
        out4.val[0] = dat.val[0];
        out4.val[1] = dat.val[0];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[1];
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
struct G2GA16_NEON
{
    static constexpr size_t M = 8, N = 16, K = 8;
    U16x8 Alpha;
    forceinline G2GA16_NEON(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        uint16x8x2_t out2;
        out2.val[0] = dat;
        out2.val[1] = Alpha;
        vst2q_u16(dst, out2);
    }
};
struct G2RGB16_NEON
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        uint16x8x3_t out3;
        out3.val[0] = dat;
        out3.val[1] = dat;
        out3.val[2] = dat;
        vst3q_u16(dst, out3);
    }
};
struct G2RGBA16_NEON
{
    static constexpr size_t M = 8, N = 32, K = 8;
    U16x8 Alpha;
    forceinline G2RGBA16_NEON(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x8(src);
        uint16x8x4_t out4;
        out4.val[0] = dat;
        out4.val[1] = dat;
        out4.val[2] = dat;
        out4.val[3] = Alpha;
        vst4q_u16(dst, out4);
    }
};
struct GA2RGB16_NEON
{
    static constexpr size_t M = 16, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u16(src);
        uint16x8x3_t out3;
        out3.val[0] = dat.val[0];
        out3.val[1] = dat.val[0];
        out3.val[2] = dat.val[0];
        vst3q_u16(dst, out3);
    }
};
struct GA2RGBA16_NEON
{
    static constexpr size_t M = 16, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u16(src);
        uint16x8x4_t out4;
        out4.val[0] = dat.val[0];
        out4.val[1] = dat.val[0];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[1];
        vst4q_u16(dst, out4);
    }
};
struct G2GAf_NEON
{
    static constexpr size_t M = 4, N = 8, K = 4;
    F32x4 Alpha;
    forceinline G2GAf_NEON(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        float32x4x2_t out2;
        out2.val[0] = dat;
        out2.val[1] = Alpha;
        vst2q_f32(dst, out2);
    }
};
struct G2RGBf_NEON
{
    static constexpr size_t M = 4, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        float32x4x3_t out3;
        out3.val[0] = dat;
        out3.val[1] = dat;
        out3.val[2] = dat;
        vst3q_f32(dst, out3);
    }
};
struct G2RGBAf_NEON
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Alpha;
    forceinline G2RGBAf_NEON(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        float32x4x4_t out4;
        out4.val[0] = dat;
        out4.val[1] = dat;
        out4.val[2] = dat;
        out4.val[3] = Alpha;
        vst4q_f32(dst, out4);
    }
};
struct GA2RGBf_NEON
{
    static constexpr size_t M = 8, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld2q_f32(src);
        float32x4x3_t out3;
        out3.val[0] = dat.val[0];
        out3.val[1] = dat.val[0];
        out3.val[2] = dat.val[0];
        vst3q_f32(dst, out3);
    }
};
struct GA2RGBAf_NEON
{
    static constexpr size_t M = 8, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld2q_f32(src);
        float32x4x4_t out4;
        out4.val[0] = dat.val[0];
        out4.val[1] = dat.val[0];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[1];
        vst4q_f32(dst, out4);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_3To4_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U8x16 Alpha;
    forceinline RGB8_3To4_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x4_t out4;
        out4.val[0] = dat.val[IdxR];
        out4.val[1] = dat.val[IdxG];
        out4.val[2] = dat.val[IdxB];
        out4.val[3] = Alpha;
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_4To3_NEON
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x3_t out3;
        out3.val[0] = dat.val[IdxR];
        out3.val[1] = dat.val[IdxG];
        out3.val[2] = dat.val[IdxB];
        vst3q_u8(dst, out3);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB16_3To4_NEON
{
    static constexpr size_t M = 24, N = 32, K = 8;
    U16x8 Alpha;
    forceinline RGB16_3To4_NEON(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u16(src);
        uint16x8x4_t out4;
        out4.val[0] = dat.val[IdxR];
        out4.val[1] = dat.val[IdxG];
        out4.val[2] = dat.val[IdxB];
        out4.val[3] = Alpha;
        vst4q_u16(dst, out4);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB16_4To3_NEON
{
    static constexpr size_t M = 32, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u16(src);
        uint16x8x3_t out3;
        out3.val[0] = dat.val[IdxR];
        out3.val[1] = dat.val[IdxG];
        out3.val[2] = dat.val[IdxB];
        vst3q_u16(dst, out3);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGBf_3To4_NEON
{
    static constexpr size_t M = 12, N = 16, K = 4;
    F32x4 Alpha;
    forceinline RGBf_3To4_NEON(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld3q_f32(src);
        float32x4x4_t out4;
        out4.val[0] = dat.val[IdxR];
        out4.val[1] = dat.val[IdxG];
        out4.val[2] = dat.val[IdxB];
        out4.val[3] = Alpha;
        vst4q_f32(dst, out4);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGBf_4To3_NEON
{
    static constexpr size_t M = 16, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_f32(src);
        float32x4x3_t out3;
        out3.val[0] = dat.val[IdxR];
        out3.val[1] = dat.val[IdxG];
        out3.val[2] = dat.val[IdxB];
        vst3q_f32(dst, out3);
    }
};
struct RGBToBGR8_NEON
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(src);
        uint8x16x3_t out3;
        out3.val[0] = dat.val[2];
        out3.val[1] = dat.val[1];
        out3.val[2] = dat.val[0];
        vst3q_u8(dst, out3);
    }
};
struct RGBAToBGRA8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x4_t out4;
        out4.val[0] = dat.val[2];
        out4.val[1] = dat.val[1];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[3];
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
struct RGBToBGR16_NEON
{
    static constexpr size_t M = 24, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u16(src);
        uint16x8x3_t out3;
        out3.val[0] = dat.val[2];
        out3.val[1] = dat.val[1];
        out3.val[2] = dat.val[0];
        vst3q_u16(dst, out3);
    }
};
struct RGBAToBGRA16_NEON
{
    static constexpr size_t M = 32, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u16(src);
        uint16x8x4_t out4;
        out4.val[0] = dat.val[2];
        out4.val[1] = dat.val[1];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[3];
        vst4q_u16(dst, out4);
    }
};
struct RGBToBGRf_NEON
{
    static constexpr size_t M = 12, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld3q_f32(src);
        float32x4x3_t out3;
        out3.val[0] = dat.val[2];
        out3.val[1] = dat.val[1];
        out3.val[2] = dat.val[0];
        vst3q_f32(dst, out3);
    }
};
struct RGBAToBGRAf_NEON
{
    static constexpr size_t M = 16, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_f32(src);
        float32x4x4_t out4;
        out4.val[0] = dat.val[2];
        out4.val[1] = dat.val[1];
        out4.val[2] = dat.val[0];
        out4.val[3] = dat.val[3];
        vst4q_f32(dst, out4);
    }
};
template<uint8_t Idx>
struct KeepChannel8_3_1_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(src);
        U8x16 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel16_3_1_NEON
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u16(src);
        U16x8 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF_4_1_NEON
{
    static constexpr size_t M = 16, N = 4, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_f32(src);
        F32x4 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF_3_1_NEON
{
    static constexpr size_t M = 12, N = 4, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld3q_f32(src);
        F32x4 out(dat.val[Idx]);
        out.Save(dst);
    }
};
struct Extract8x2_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u8(reinterpret_cast<const uint8_t*>(src));
        const U8x16 out0(dat.val[0]);
        const U8x16 out1(dat.val[1]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract8x3_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(src);
        const U8x16 out0(dat.val[0]);
        const U8x16 out1(dat.val[1]);
        const U8x16 out2(dat.val[2]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract8x4_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        const U8x16 out0(dat.val[0]);
        const U8x16 out1(dat.val[1]);
        const U8x16 out2(dat.val[2]);
        const U8x16 out3(dat.val[3]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract16x2_NEON
{
    static constexpr size_t M = 16, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld2q_u16(src);
        const U16x8 out0(dat.val[0]);
        const U16x8 out1(dat.val[1]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract16x3_NEON
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u16(src);
        const U16x8 out0(dat.val[0]);
        const U16x8 out1(dat.val[1]);
        const U16x8 out2(dat.val[2]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract16x4_NEON
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u16(src);
        const U16x8 out0(dat.val[0]);
        const U16x8 out1(dat.val[1]);
        const U16x8 out2(dat.val[2]);
        const U16x8 out3(dat.val[3]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract32x2_NEON
{
    static constexpr size_t M = 8, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld2q_f32(src);
        const F32x4 out0(dat.val[0]);
        const F32x4 out1(dat.val[1]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract32x3_NEON
{
    static constexpr size_t M = 12, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld3q_f32(src);
        const F32x4 out0(dat.val[0]);
        const F32x4 out1(dat.val[1]);
        const F32x4 out2(dat.val[2]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract32x4_NEON
{
    static constexpr size_t M = 16, N = 4, K = 4;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_f32(src);
        const F32x4 out0(dat.val[0]);
        const F32x4 out1(dat.val[1]);
        const F32x4 out2(dat.val[2]);
        const F32x4 out3(dat.val[3]);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Combine8x2_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);

        uint8x16x2_t out2;
        out2.val[0] = dat0;
        out2.val[1] = dat1;
        vst2q_u8(reinterpret_cast<uint8_t*>(dst), out2);
    }
};
struct Combine8x3_NEON
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);
        const U8x16 dat2(src[2]);

        uint8x16x3_t out3;
        out3.val[0] = dat0;
        out3.val[1] = dat1;
        out3.val[2] = dat2;
        vst3q_u8(dst, out3);
    }
};
struct Combine8x4_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);
        const U8x16 dat2(src[2]);
        const U8x16 dat3(src[3]);

        uint8x16x4_t out4;
        out4.val[0] = dat0;
        out4.val[1] = dat1;
        out4.val[2] = dat2;
        out4.val[3] = dat3;
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
struct Combine16x2_NEON
{
    static constexpr size_t M = 8, N = 16, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);

        uint16x8x2_t out2;
        out2.val[0] = dat0;
        out2.val[1] = dat1;
        vst2q_u16(dst, out2);
    }
};
struct Combine16x3_NEON
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);
        const U16x8 dat2(src[2]);

        uint16x8x3_t out3;
        out3.val[0] = dat0;
        out3.val[1] = dat1;
        out3.val[2] = dat2;
        vst3q_u16(dst, out3);
    }
};
struct Combine16x4_NEON
{
    static constexpr size_t M = 8, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x8 dat0(src[0]);
        const U16x8 dat1(src[1]);
        const U16x8 dat2(src[2]);
        const U16x8 dat3(src[3]);

        uint16x8x4_t out4;
        out4.val[0] = dat0;
        out4.val[1] = dat1;
        out4.val[2] = dat2;
        out4.val[3] = dat3;
        vst4q_u16(dst, out4);
    }
};
struct Combine32x2_NEON
{
    static constexpr size_t M = 4, N = 8, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);

        float32x4x2_t out2;
        out2.val[0] = dat0;
        out2.val[1] = dat1;
        vst2q_f32(dst, out2);
    }
};
struct Combine32x3_NEON
{
    static constexpr size_t M = 4, N = 12, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);
        const F32x4 dat2(src[2]);

        float32x4x3_t out3;
        out3.val[0] = dat0;
        out3.val[1] = dat1;
        out3.val[2] = dat2;
        vst3q_f32(dst, out3);
    }
};
struct Combine32x4_NEON
{
    static constexpr size_t M = 4, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x4 dat0(src[0]);
        const F32x4 dat1(src[1]);
        const F32x4 dat2(src[2]);
        const F32x4 dat3(src[3]);

        float32x4x4_t out4;
        out4.val[0] = dat0;
        out4.val[1] = dat1;
        out4.val[2] = dat2;
        out4.val[3] = dat3;
        vst4q_f32(dst, out4);
    }
};
struct G16ToG8_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld1q_u16_x2(src);
        const U16x8 dat0(dat.val[0]);
        const U16x8 dat1(dat.val[1]);
        const U16x8 muler((uint16_t)0xff01u); // y => x / 0xffff * 0xff => x / 257 => ((x << 24) / 257) >> 24 => (x * (1 << 24) / 257) >> 24 ~> (x * 0xff01) >> 24

        const auto mid0 = dat0.MulX(muler).As<U16x8>();
        const auto mid1 = dat1.MulX(muler).As<U16x8>();
# if COMMON_SIMD_LV >= 200
        const auto val0 = vreinterpretq_u8_u16(vuzp2q_u16(mid0[0], mid0[1]));
        const auto val1 = vreinterpretq_u8_u16(vuzp2q_u16(mid1[0], mid1[1]));
        const U8x16 out = vuzp2q_u8(val0, val1);
# else
        const auto val0 = vreinterpretq_u8_u16(vuzpq_u16(mid0[0], mid0[1]).val[1]);
        const auto val1 = vreinterpretq_u8_u16(vuzpq_u16(mid1[0], mid1[1]).val[1]);
        const U8x16 out = vuzpq_u8(val0, val1).val[1];
# endif
        out.Save(dst);
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_NEON : RGB5x5ToRGB8_128Base<Is555>
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld1q_u16_x2(src);

        const auto [midR0, midG0, midB0] = this->RGB5x5ToRGB8Hi(dat.val[0]);
        const auto [midR1, midG1, midB1] = this->RGB5x5ToRGB8Hi(dat.val[1]);

# if COMMON_SIMD_LV >= 200
        const auto outR = vuzp2q_u8(midR0.template As<U8x16>(), midR1.template As<U8x16>());
        const auto outG = vuzp2q_u8(midG0.template As<U8x16>(), midG1.template As<U8x16>());
        const auto outB = vuzp2q_u8(midB0.template As<U8x16>(), midB1.template As<U8x16>());
# else
        const auto outR = vuzpq_u8(midR0.template As<U8x16>(), midR1.template As<U8x16>()).val[1];
        const auto outG = vuzpq_u8(midG0.template As<U8x16>(), midG1.template As<U8x16>()).val[1];
        const auto outB = vuzpq_u8(midB0.template As<U8x16>(), midB1.template As<U8x16>()).val[1];
# endif
        uint8x16x3_t out3;
        out3.val[0] = IsRGB ? outR : outB;
        out3.val[1] = outG;
        out3.val[2] = IsRGB ? outB : outR;
        vst3q_u8(dst, out3);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_NEON : RGB5x5ToRGB8_128Base<Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld1q_u16_x2(src);
        const U16x8 dat0 = dat.val[0], dat1 = dat.val[1];

        const auto [midR0, midG0, midB0] = this->RGB5x5ToRGB8Hi(dat0);
        const auto [midR1, midG1, midB1] = this->RGB5x5ToRGB8Hi(dat1);

# if COMMON_SIMD_LV >= 200
        const auto outR = vuzp2q_u8(midR0.template As<U8x16>(), midR1.template As<U8x16>());
        const auto outG = vuzp2q_u8(midG0.template As<U8x16>(), midG1.template As<U8x16>());
        const auto outB = vuzp2q_u8(midB0.template As<U8x16>(), midB1.template As<U8x16>());
# else
        const auto outR = vuzpq_u8(midR0.template As<U8x16>(), midR1.template As<U8x16>()).val[1];
        const auto outG = vuzpq_u8(midG0.template As<U8x16>(), midG1.template As<U8x16>()).val[1];
        const auto outB = vuzpq_u8(midB0.template As<U8x16>(), midB1.template As<U8x16>()).val[1];
# endif
        uint8x16x4_t out4;
        out4.val[0] = IsRGB ? outR : outB;
        out4.val[1] = outG;
        out4.val[2] = IsRGB ? outB : outR;
        if constexpr (HasAlpha)
        {
# if COMMON_SIMD_LV >= 200
            const U8x16 datHi = vuzp2q_u8(dat0.As<U8x16>(), dat1.As<U8x16>());
# else
            const U8x16 datHi = vuzpq_u8(dat0.As<U8x16>(), dat1.As<U8x16>()).val[1];
# endif
            out4.val[3] = vtstq_u8(datHi, vdupq_n_u8(0x80));
            // out4.val[3] = datHi.As<I8x16>().ShiftRightArith<7>().As<U8x16>();
        }
        else
        {
            out4.val[3] = vdupq_n_u8(0xff);
        }
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_NeonLUTBase
{
    forceinline static std::array<U8x16, 3> RGB5x5ToRGB8(const U16x8& dat0, const U16x8& dat1) noexcept
    {
        const U16x8 maskRB(Is555 ? 0x7c1fu : 0xf81fu);
        const auto dat0_ = dat0.And(maskRB), dat1_ = dat1.And(maskRB);
        const auto dat01LH = vuzpq_u8(dat0_.As<U8x16>(), dat1_.As<U8x16>());

# if COMMON_SIMD_LV >= 200
        const auto val01G_ = vshrn_high_n_u16(vshrn_n_u16(dat0, 5), dat1, 5);
        const auto val01G = U8x16(val01G_).And(U8x16(Is555 ? 0x1f : 0x3f));

        const auto lut5 = vld1q_u8_x2(LUT5Bit.data());
        const U8x16 midLo = vqtbl2q_u8(lut5, dat01LH.val[0]);
        const U8x16 midHi = vqtbl2q_u8(lut5, vshrq_n_u8(dat01LH.val[1], Is555 ? 2 : 3));
        U8x16 midG;
        if constexpr (Is555)
        {
            midG = vqtbl2q_u8(lut5, val01G);
        }
        else
        {
            const auto lut6 = vld1q_u8_x4(LUT6Bit.data());
            midG = vqtbl4q_u8(lut6, val01G);
        }
# else
        const auto val01G_ = vcombine_u8(vshrn_n_u16(dat0, 5), vshrn_n_u16(dat1, 5));
        const auto val01G = U8x16(val01G_).And(U8x16(Is555 ? 0x1f : 0x3f));
        const auto val0Lo = vget_low_u8(dat01LH.val[0]), val1Lo = vget_high_u8(dat01LH.val[0]);
        const auto val0Hi = vget_low_u8(dat01LH.val[1]), val1Hi = vget_high_u8(dat01LH.val[1]);
        const auto val0G  = vget_low_u8(val01G), val1G = vget_high_u8(val01G);

        const auto lut5 = vld1_u8_x4(LUT5Bit.data());
        const auto mid0Lo = vtbl4_u8(lut5, val0Lo), mid1Lo = vtbl4_u8(lut5, val1Lo);
        const auto mid0Hi = vtbl4_u8(lut5, val0Hi), mid1Hi = vtbl4_u8(lut5, val1Hi);
        uint8x8_t mid0G, mid1G;
        if constexpr (Is555)
        {
            mid0G = vtbl4_u8(lut5, val0G), mid1G = vtbl4_u8(lut5, val1G);
        }
        else
        {
            const auto lut6lo = vld1_u8_x4(LUT6Bit.data()), lut6hi = vld1_u8_x4(LUT6Bit.data() + 32);
            mid0G = vtbl4_u8(lut6lo, val0G), mid1G = vtbl4_u8(lut6lo, val1G);
            const auto val01H = val01G.Xor(U8x16(0x20));
            const auto val0H = vget_low_u8(val01H), val1H = vget_high_u8(val01H);
            mid0G = vtbx4_u8(mid0G, lut6lo, val0H), mid1G = vtbx4_u8(mid1G, lut6lo, val1H);
        }
        const U8x16 midLo = vcombine_u8(mid0Lo, mid1Lo);
        const U8x16 midHi = vcombine_u8(mid0Hi, mid1Hi);
        const U8x16 midG  = vcombine_u8(mid0G, mid1G);
# endif
        return { IsRGB ? midLo : midHi, midG, IsRGB ? midHi : midLo };
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_LUT_NEON : RGB5x5ToRGB8_NeonLUTBase<IsRGB, Is555>
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld1q_u16_x2(src);

        const auto [midR, midG, midB] = this->RGB5x5ToRGB8(dat.val[0], dat.val[1]);

        uint8x16x3_t out3;
        out3.val[0] = midR;
        out3.val[1] = midG;
        out3.val[2] = midB;
        vst3q_u8(dst, out3);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_LUT_NEON : RGB5x5ToRGB8_NeonLUTBase<IsRGB, Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = vld1q_u16_x2(src);
        const U16x8 dat0 = dat.val[0], dat1 = dat.val[1];
        const auto [midR, midG, midB] = this->RGB5x5ToRGB8(dat0, dat1);

        uint8x16x4_t out4;
        out4.val[0] = midR;
        out4.val[1] = midG;
        out4.val[2] = midB;
        if constexpr (HasAlpha)
        {
# if COMMON_SIMD_LV >= 200
            const U8x16 datHi = vuzp2q_u8(dat0.As<U8x16>(), dat1.As<U8x16>());
# else
            const U8x16 datHi = vuzpq_u8(dat0.As<U8x16>(), dat1.As<U8x16>()).val[1];
# endif
            out4.val[3] = vtstq_u8(datHi, vdupq_n_u8(0x80));
        }
        else
        {
            out4.val[3] = vdupq_n_u8(0xff);
        }
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
template<bool Scale, bool IsRGB>
struct RGB10ToRGBf_NEON
{
    static constexpr size_t M = 4, N = 12, K = 4;
    F32x4 Scaler;
    RGB10ToRGBf_NEON(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat(src);

        const U32x4 maskLo10bit(0x3ffu);
        const auto dat0 = dat.And(maskLo10bit);
        const auto dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit);
        const auto dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit);

        F32x4 valR = (IsRGB ? dat0 : dat2).Cast<F32x4>();
        F32x4 valG = dat1.Cast<F32x4>();
        F32x4 valB = (IsRGB ? dat2 : dat0).Cast<F32x4>();
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }

        float32x4x3_t out3;
        out3.val[0] = valR;
        out3.val[1] = valG;
        out3.val[2] = valB;
        vst3q_f32(dst, out3);
    }
};
template<bool Scale, bool IsRGB, bool HasAlpha>
struct RGB10ToRGBAf_NEON
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Scaler;
    RGB10ToRGBAf_NEON(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat(src);

        const U32x4 maskLo10bit(0x3ffu);
        const auto dat0 = dat.And(maskLo10bit);
        const auto dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit);
        const auto dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit);

        F32x4 valR = (IsRGB ? dat0 : dat2).Cast<F32x4>();
        F32x4 valG = dat1.Cast<F32x4>();
        F32x4 valB = (IsRGB ? dat2 : dat0).Cast<F32x4>();
        F32x4 valA;
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }
        if constexpr (HasAlpha)
        {
            valA = dat.ShiftRightLogic<30>().Cast<F32x4>().Mul(1.0f / 3.0f);
        }
        else
        {
            valA = F32x4(1.0f);
        }

        float32x4x4_t out4;
        out4.val[0] = valR;
        out4.val[1] = valG;
        out4.val[2] = valB;
        out4.val[3] = valA;
        vst4q_f32(dst, out4);
    }
};


struct RGB8ToYUV8_F32_NEON
{
    static constexpr size_t M = 48, N = 48, K = 16;
    F32x4 LutY, LutCb, LutCr;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_F32_NEON(const std::array<float, 9>& lut, RGBOrder<R, G, B>) noexcept : LutY(lut.data()), LutCb(lut.data() + 3), LutCr(lut.data() + 5)
    {
        LutY  = LutY .Shuffle<R, G, B, 3>();
        LutCb = LutCb.Shuffle<R, G, B, 3>();
        LutCr = LutCr.Shuffle<R, G, B, 3>();
    }
    forceinline static int16x8_t DP3(const float32x4_t& r, const float32x4_t& g, const float32x4_t& b, const F32x4& lut, const float base) noexcept
    {
        const auto adder  = vdupq_n_f32(base);
        const auto midR   = vfmaq_laneq_f32(adder, r, lut, 0);
        const auto midRG  = vfmaq_laneq_f32(midR,  g, lut, 1);
        const auto midRGB = vfmaq_laneq_f32(midRG, b, lut, 2);
# if COMMON_SIMD_LV >= 100
        return vreinterpretq_s16_s32(vcvtnq_s32_f32(midRGB));
# else
        return vreinterpretq_s16_s32(vcvtq_s32_f32(midRGB)); // no way to round, leave it low prec
# endif
    }
    template<bool ClampC>
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat = vld3q_u8(src);
        const auto z8 = vdupq_n_u8(0);
        const auto z16 = vreinterpretq_u16_u8(z8);

# define U8ToF32(x, i) const auto dat##x##_ = vzipq_u8(dat.val[i], z8);\
        const auto dat##x##L_ = vzipq_u16(vreinterpretq_u16_u8(dat##x##_.val[0]), z16), dat##x##H_ = vzipq_u16(vreinterpretq_u16_u8(dat##x##_.val[1]), z16);\
        const auto dat##x##0 = vcvtq_f32_s32(vreinterpretq_s32_u16(dat##x##L_.val[0]));\
        const auto dat##x##1 = vcvtq_f32_s32(vreinterpretq_s32_u16(dat##x##L_.val[1]));\
        const auto dat##x##2 = vcvtq_f32_s32(vreinterpretq_s32_u16(dat##x##H_.val[0]));\
        const auto dat##x##3 = vcvtq_f32_s32(vreinterpretq_s32_u16(dat##x##H_.val[1]))
        U8ToF32(R, 0);
        U8ToF32(G, 1);
        U8ToF32(B, 2);
# undef U8ToF32

#define DP3x4(out, lut, add) const auto out##0 = DP3(datR0, datG0, datB0, lut, add), out##1 = DP3(datR1, datG1, datB1, lut, add), \
    out##2 = DP3(datR2, datG2, datB2, lut, add), out##3 = DP3(datR3, datG3, datB3, lut, add)
        DP3x4(lumi4i,  LutY, needAddY ? 16.f : 0.f);
        DP3x4(  cb4i, LutCb, 128.f);
        DP3x4(  cr4i, LutCr, 128.f);
#undef DP3x4
        uint8x16x3_t out3;

# if COMMON_SIMD_LV >= 200
#   define I32ToU8(x) vuzp1q_u8(vreinterpretq_u8_s16(vuzp1q_s16(x##0, x##1)), vreinterpretq_u8_s16(vuzp1q_s16(x##2, x##3)))
# else
#   define I32ToU8(x) vuzpq_u8(vreinterpretq_u8_s16(vuzpq_s16(x##0, x##1).val[0]), vreinterpretq_u8_s16(vuzpq_s16(x##2, x##3).val[0])).val[0]
# endif
        // only lowest byte should be used
        out3.val[0] = I32ToU8(lumi4i);
        if constexpr (ClampC)
        {
# if COMMON_SIMD_LV >= 200
            const auto cb0 = vuzp1q_s16(cb4i0, cb4i1), cb1 = vuzp1q_s16(cb4i2, cb4i3), cr0 = vuzp1q_s16(cr4i0, cr4i1), cr1 = vuzp1q_s16(cr4i2, cr4i3);
            out3.val[1] = vqmovun_high_s16(vqmovun_s16(cb0), cb1);
            out3.val[2] = vqmovun_high_s16(vqmovun_s16(cr0), cr1);
# else
            const auto cb0 = vuzpq_s16(cb4i0, cb4i1).val[0], cb1 = vuzpq_s16(cb4i2, cb4i3).val[0], cr0 = vuzpq_s16(cr4i0, cr4i1).val[0], cr1 = vuzpq_s16(cr4i2, cr4i3).val[0];
            out3.val[1] = vcombine_u8(vqmovun_s16(cb0), vqmovun_s16(cb1));
            out3.val[2] = vcombine_u8(vqmovun_s16(cr0), vqmovun_s16(cr1));
# endif
        }
        else
        {
            out3.val[1] = I32ToU8(cb4i);
            out3.val[2] = I32ToU8(cr4i);
        }
#undef I32ToU8
        vst3q_u8(dst, out3);
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, NEON_F32)
{
    if (count >= RGB8ToYUV8_F32_NEON::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_F32_NEON proc(RGB8ToYCC8LUTF32[mval], RGBOrder<0, 1, 2>{});
        common::RegionRounding rd(common::RoundMode::ToNearest);
        do
        {
            if (isYCCFull)
                proc.Convert<true>(dest, src, needAddY);
            else
                proc.Convert<false>(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}

DEFINE_FASTPATH_METHOD(G8ToGA8, NEON)
{
    ProcessLOOP4<G2GA8_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, NEON)
{
    ProcessLOOP4<G2RGB8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, NEON)
{
    ProcessLOOP4<G2RGBA8_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, NEON)
{
    ProcessLOOP4<GA2RGB8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, NEON)
{
    ProcessLOOP4<GA2RGBA8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToGA16, NEON)
{
    ProcessLOOP4<G2GA16_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G16ToRGB16, NEON)
{
    ProcessLOOP4<G2RGB16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToRGBA16, NEON)
{
    ProcessLOOP4<G2RGBA16_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA16ToRGB16, NEON)
{
    ProcessLOOP4<GA2RGB16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA16ToRGBA16, NEON)
{
    ProcessLOOP4<GA2RGBA16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, NEON)
{
    ProcessLOOP4<G2GAf_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, NEON)
{
    ProcessLOOP4<G2RGBf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, NEON)
{
    ProcessLOOP4<G2RGBAf_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, NEON)
{
    ProcessLOOP4<GA2RGBf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, NEON)
{
    ProcessLOOP4<GA2RGBAf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, NEON)
{
    ProcessLOOP4<RGB8_3To4_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, NEON)
{
    ProcessLOOP4<RGB8_3To4_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, NEON)
{
    ProcessLOOP4<RGB8_4To3_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, NEON)
{
    ProcessLOOP4<RGB8_4To3_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToRGBA16, NEON)
{
    ProcessLOOP4<RGB16_3To4_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR16ToRGBA16, NEON)
{
    ProcessLOOP4<RGB16_3To4_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA16ToRGB16, NEON)
{
    ProcessLOOP4<RGB16_4To3_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGR16, NEON)
{
    ProcessLOOP4<RGB16_4To3_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, NEON)
{
    ProcessLOOP4<RGBf_3To4_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, NEON)
{
    ProcessLOOP4<RGBf_3To4_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, NEON)
{
    ProcessLOOP4<RGBf_4To3_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, NEON)
{
    ProcessLOOP4<RGBf_4To3_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, NEON)
{
    ProcessLOOP4<RGBToBGR8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, NEON)
{
    ProcessLOOP4<RGBAToBGRA8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToBGR16, NEON)
{
    ProcessLOOP4<RGBToBGR16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGRA16, NEON)
{
    ProcessLOOP4<RGBAToBGRA16_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, NEON)
{
    ProcessLOOP4<RGBToBGRf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, NEON)
{
    ProcessLOOP4<RGBAToBGRAf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, NEON)
{
    ProcessLOOP4<KeepChannel8_3_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, NEON)
{
    ProcessLOOP4<KeepChannel8_3_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, NEON)
{
    ProcessLOOP4<KeepChannel8_3_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToR16, NEON)
{
    ProcessLOOP4<KeepChannel16_3_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToG16, NEON)
{
    ProcessLOOP4<KeepChannel16_3_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToB16, NEON)
{
    ProcessLOOP4<KeepChannel16_3_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, NEON)
{
    ProcessLOOP4<KeepChannelF_4_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, NEON)
{
    ProcessLOOP4<KeepChannelF_4_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, NEON)
{
    ProcessLOOP4<KeepChannelF_4_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, NEON)
{
    ProcessLOOP4<KeepChannelF_4_1_NEON<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, NEON)
{
    ProcessLOOP4<KeepChannelF_3_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, NEON)
{
    ProcessLOOP4<KeepChannelF_3_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, NEON)
{
    ProcessLOOP4<KeepChannelF_3_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, NEON)
{
    ExtractLOOP4<2, Extract8x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, NEON)
{
    ExtractLOOP4<3, Extract8x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, NEON)
{
    ExtractLOOP4<4, Extract8x4_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x2, NEON)
{
    ExtractLOOP4<2, Extract16x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x3, NEON)
{
    ExtractLOOP4<3, Extract16x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x4, NEON)
{
    ExtractLOOP4<4, Extract16x4_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x2, NEON)
{
    ExtractLOOP4<2, Extract32x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x3, NEON)
{
    ExtractLOOP4<3, Extract32x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x4, NEON)
{
    ExtractLOOP4<4, Extract32x4_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x2, NEON)
{
    CombineLOOP4<2, Combine8x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, NEON)
{
    CombineLOOP4<3, Combine8x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, NEON)
{
    CombineLOOP4<4, Combine8x4_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x2, NEON)
{
    CombineLOOP4<2, Combine16x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x3, NEON)
{
    CombineLOOP4<3, Combine16x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x4, NEON)
{
    CombineLOOP4<4, Combine16x4_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, NEON)
{
    CombineLOOP4<2, Combine32x2_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, NEON)
{
    CombineLOOP4<3, Combine32x3_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, NEON)
{
    CombineLOOP4<4, Combine32x4_NEON, &Func<LOOP>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(G16ToG8, NEON)
{
    ProcessLOOP4<G16ToG8_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEON<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEON<false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEON<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEON<false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_NEON<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_NEON<false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_NEON<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_NEON<false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<true, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<false, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<true, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<false, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<true, false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, NEON)
{
    ProcessLOOP4<RGB5x5ToRGBA8_NEON<false, false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<true, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<false, false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<true, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<false, true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<true, false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, NEON2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_NEON<false, false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_NEON<false, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_NEON<true, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_NEON<false, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_NEON<true, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBAf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_NEON<false, true, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_NEON<true, true, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBAf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_NEON<false, false, false>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_NEON<true, false, false>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_NEON<false, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_NEON<true, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, NEON)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_NEON<false, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_NEON<true, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 200
struct Gf2RGBAf_NEON64
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Alpha;
    forceinline Gf2RGBAf_NEON64(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_dup_f32(src);
        float32x4x4_t out4;
        out4.val[0] = vextq_f32(dat.val[0], Alpha, 1);
        out4.val[1] = vextq_f32(dat.val[1], Alpha, 1);
        out4.val[2] = vextq_f32(dat.val[2], Alpha, 1);
        out4.val[3] = vextq_f32(dat.val[3], Alpha, 1);
        vst1q_f32_x4(dst, out4);
    }
};
struct GAf2RGBAf_NEON64
{
    static constexpr size_t M = 4, N = 8, K = 2;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld4q_dup_f32(src);
        float32x4x2_t out2;
        out2.val[0] = vextq_f32(dat.val[0], dat.val[1], 1);
        out2.val[1] = vextq_f32(dat.val[2], dat.val[3], 1);
        vst1q_f32_x2(dst, out2);
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_NEONA64
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src + 0); // 01234567
        const auto dat1 = U16x8(src + 8); // 89abcdef

        const U16x8 maskLo5bit(uint16_t(0x1fu));
        const U16x8 maskLo6bit(uint16_t(0x3fu));

        const auto valR0 = dat0.And(maskLo5bit);
        const auto valR1 = dat1.And(maskLo5bit);
        const auto valG0 = dat0.ShiftRightLogic<5>().And(Is555 ? maskLo5bit : maskLo6bit);
        const auto valG1 = dat1.ShiftRightLogic<5>().And(Is555 ? maskLo5bit : maskLo6bit);
        U16x8 valB0 = dat0.ShiftRightLogic<Is555 ? 10 : 11>();
        U16x8 valB1 = dat1.ShiftRightLogic<Is555 ? 10 : 11>();
        if constexpr (Is555)
        {
            valB0 = valB0.And(maskLo5bit);
            valB1 = valB1.And(maskLo5bit);
        }

        // pre-shift-left by 2
        const U16x8 add5(23 << 2), add6(33 << 2);
        const uint16_t muler5 = 527 << 2, muler6 = 259 << 2;
        const U16x8 mul5(muler5), mul6(muler6);
        const U16x8 midR0 = valR0.MulAddLo(mul5, add5); // at Hi
        const U16x8 midR1 = valR1.MulAddLo(mul5, add5); // at Hi
        const U8x16 midG0 = valG0.MulAddLo(Is555 ? mul5 : mul6, Is555 ? add5 : add6).As<U8x16>(); // at Hi
        const U8x16 midG1 = valG1.MulAddLo(Is555 ? mul5 : mul6, Is555 ? add5 : add6).As<U8x16>(); // at Hi
        const U16x8 midB0 = valB0.MulAddLo(mul5, add5); // at Hi
        const U16x8 midB1 = valB1.MulAddLo(mul5, add5); // at Hi
        const auto midRB0 = (IsRGB ? midR0 : midB0).As<U8x16>().ZipEven((IsRGB ? midB0 : midR0).As<U8x16>());
        const auto midRB1 = (IsRGB ? midR1 : midB1).As<U8x16>().ZipEven((IsRGB ? midB1 : midR1).As<U8x16>());
        
        alignas(16) static constexpr uint8_t indexes[] =
        {
             0, 17,  1,  2, 19,  3,  4, 21,  5,  6, 23,  7,  8, 25,  9, 10,
            19,  3,  4, 21,  5,  6, 23,  7,  8, 25,  9, 10, 27, 11, 12, 29,
             5,  6, 23,  7,  8, 25,  9, 10, 27, 11, 12, 29, 13, 14, 31, 15,
        };
        const auto shuffle_RGB = vld1q_u8_x3(indexes);

        uint8x16x2_t midRGB0;
        midRGB0.val[0] = midRB0;
        midRGB0.val[1] = midG0;
        const U8x16 out0 = vqtbl2q_u8(midRGB0, shuffle_RGB.val[0]);

        uint8x16x2_t midRGB1;
        midRGB1.val[0] = midRB0.MoveToLoWith<8>(midRB1);
        midRGB1.val[1] = midG0 .MoveToLoWith<8>(midG1);
        const U8x16 out1 = vqtbl2q_u8(midRGB1, shuffle_RGB.val[1]);

        uint8x16x2_t midRGB2;
        midRGB2.val[0] = midRB1;
        midRGB2.val[1] = midG1;
        const U8x16 out2 = vqtbl2q_u8(midRGB2, shuffle_RGB.val[2]);

        out0.Save(dst + 0 );
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct RGB8ToYUV8_F16_NEON_Base
{
    static constexpr size_t M = 48, N = 48, K = 16;
    F16x8 Lut;
    RGB8ToYUV8_F16_NEON_Base(const std::array<float, 9>& lut) noexcept
    {
        const auto lut32 = vld1q_f32_x2(lut.data());
        Lut = vcvt_high_f16_f32(vcvt_f16_f32(lut32.val[0]), lut32.val[1]);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB8ToYUV8_F16_NEON : public RGB8ToYUV8_F16_NEON_Base
{
    using RGB8ToYUV8_F16_NEON_Base::RGB8ToYUV8_F16_NEON_Base;
    template<uint8_t Idx>
    forceinline int16x8_t DP3(const float16x8_t& r, const float16x8_t& g, const float16x8_t& b, const uint16_t base) const noexcept
    {
        const auto adder  = vreinterpretq_f16_u16(vdupq_n_u16(base));
        const auto midR   = vfmaq_laneq_f16(adder, r, Lut, Idx + R);
        const auto midRG  = vfmaq_laneq_f16(midR,  g, Lut, Idx + G);
        const auto midRGB = vfmaq_laneq_f16(midRG, b, Lut, Idx + B);
        return vcvtnq_s16_f16(midRGB);
    }
    template<bool ClampC>
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat = vld3q_u8(src);
        const auto z8 = vdupq_n_u8(0);

# define U8ToF16(x, i) const auto dat##x = vzipq_u8(dat.val[i], z8);                \
        const auto dat##x##0 = vcvtq_f16_s16(vreinterpretq_s16_u8(dat##x.val[0]));  \
        const auto dat##x##1 = vcvtq_f16_s16(vreinterpretq_s16_u8(dat##x.val[1]));
        U8ToF16(R, 0);
        U8ToF16(G, 1);
        U8ToF16(B, 2);
# undef U8ToF16

#define DP3x2(out, idx, add) const auto out##0 = DP3<idx>(datR0, datG0, datB0, add), out##1 = DP3<idx>(datR1, datG1, datB1, add)
        DP3x2(lumi2i, 0, needAddY ? 0x4c00u : 0x0u);
        DP3x2(  cb2i, 3, 0x5800u);
        DP3x2(  cr2i, 5, 0x5800u);
#undef DP3x2
        uint8x16x3_t out3;

#define I16ToU8(x) vuzp1q_u8(vreinterpretq_u8_s16(x##0), vreinterpretq_u8_s16(x##1))
        // only lowest byte should be used
        out3.val[0] = I16ToU8(lumi2i);
        if constexpr (ClampC)
        {
            out3.val[1] = vqmovun_high_s16(vqmovun_s16(cb2i0), cb2i1);
            out3.val[2] = vqmovun_high_s16(vqmovun_s16(cr2i0), cr2i1);
        }
        else
        {
            out3.val[1] = I16ToU8(cb2i);
            out3.val[2] = I16ToU8(cr2i);
        }
#undef I16ToU8
        vst3q_u8(dst, out3);
    }
};
struct RGB8ToYUV8_I16_NEON_Base
{
    static constexpr size_t M = 48, N = 48, K = 16;
    I16x8 Lut;
    RGB8ToYUV8_I16_NEON_Base(const std::array<int16_t, 9>& lut) noexcept : Lut(lut.data())
    { }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB8ToYUV8_I16_NEON : public RGB8ToYUV8_I16_NEON_Base
{
    using RGB8ToYUV8_I16_NEON_Base::RGB8ToYUV8_I16_NEON_Base;
    template<uint8_t Idx>
    forceinline int16x8_t DP3(const int16x8_t& r, const int16x8_t& g, const int16x8_t& b, const int16_t base) const noexcept
    {
        const auto midR   = vqrdmlahq_laneq_s16(vdupq_n_s16(base + 32), r, Lut, Idx + R); // pre add rounding
        const auto midRG  = vqrdmlahq_laneq_s16(midR,  g, Lut, Idx + G);
        const auto midRGB = vqrdmlahq_laneq_s16(midRG, b, Lut, Idx + B);
        return midRGB;
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat = vld3q_u8(src);

# if COMMON_SIMD_LV >= 200
#   define U8SL7(x, i) const auto dat##x##0 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(dat.val[i]), 7)), dat##x##1 = vreinterpretq_s16_u16(vshll_high_n_u8(dat.val[i], 7))
# else
#   define U8SL7(x, i) const auto dat##x##0 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(dat.val[i]), 7)), dat##x##1 = vreinterpretq_s16_u16(vshll_n_u8(vget_high_s32(dat.val[i]), 7))
# endif
        U8SL7(R, 0);
        U8SL7(G, 1);
        U8SL7(B, 2);
# undef U8SL7

#define DP3x2(out, idx, add) const auto out##0 = DP3<idx>(datR0, datG0, datB0, add), out##1 = DP3<idx>(datR1, datG1, datB1, add)
        DP3x2(lumi2i, 0, needAddY ? (16 << 6) : 0);
        DP3x2(  cb2i, 3, 128 << 6);
        DP3x2(  cr2i, 5, 128 << 6);
#undef DP3x2

        uint8x16x3_t out3;
# if COMMON_SIMD_LV >= 200
#   define I16SRU8(x) vshrn_high_n_u16(vshrn_n_u16(vreinterpretq_u16_s16(x##0), 6), vreinterpretq_u16_s16(x##1), 6)
#   define I16SatU8(x) vqshrun_high_n_s16(vqshrun_n_s16(x##0, 6), x##1, 6)
# else
#   define I16SRU8(x) vcombine_u8(vshrn_n_u16(vreinterpretq_u16_s16(x##0), 6), vshrn_n_u16(vreinterpretq_u16_s16(x##1), 6))
#   define I16SatU8(x) vcombine_u8(vqshrun_n_s16(x##0, 6), vqshrun_n_s16(x##1, 6))
# endif
        // only lowest byte should be used
        out3.val[0] = I16SRU8(lumi2i);
        out3.val[1] = I16SatU8(cb2i);
        out3.val[2] = I16SatU8(cr2i);
#undef I16SRU8
#undef I16SatU8
        vst3q_u8(dst, out3);
    }
};
struct RGB8ToYUV8_I8_NEON_Base
{
    static constexpr size_t M = 48, N = 48, K = 16;
};
template<uint8_t R, uint8_t G, uint8_t B, typename T>
struct RGB8ToYUV8_I8_NEON : public RGB8ToYUV8_I8_NEON_Base
{
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat = vld3q_u8(src);
        const auto datRG = vzipq_u8(dat.val[R], dat.val[G]);
        const auto datGB = vzipq_u8(dat.val[G], dat.val[B]);
        const auto datRB = vzipq_u8(dat.val[R], dat.val[B]);
        const auto& self = *reinterpret_cast<const T*>(this);

#define DP4x4(out, idx, sm0, sm1, base) const auto out##0 = self.template DP4<idx>(dat##sm0.val[0], dat##sm1.val[0], base), out##1 = self.template DP4<idx>(dat##sm0.val[1], dat##sm1.val[1], base)
        DP4x4(lumi, 0, RG, GB, needAddY ? 16 : 0);
        DP4x4(cb,   1, RB, GB, 128);
        DP4x4(cr,   2, RG, RB, 128);
#undef DP4x4
        uint8x16x3_t out3;
        out3.val[0] = vuzp2q_u8(lumi0, lumi1);
        out3.val[1] = vuzp2q_u8(cb0,   cb1);
        out3.val[2] = vuzp2q_u8(cr0,   cr1);
        vst3q_u8(dst, out3);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB8ToYUV8_U8DP4A_NEON : public RGB8ToYUV8_I8_NEON<R, G, B, RGB8ToYUV8_U8DP4A_NEON<R, G, B>>
{
    U8x16 Lut;
    RGB8ToYUV8_U8DP4A_NEON(const std::array<uint8_t, 16>& lut) noexcept : Lut(lut.data()) {}
    template<uint8_t Idx>
    forceinline uint8x16_t DP4(const uint8x16_t& lo, const uint8x16_t& hi, const uint32_t base) const noexcept
    {
        const auto rgb = vzipq_u16(vreinterpretq_u16_u8(lo), vreinterpretq_u16_u8(hi));
        const auto rgb0 = vreinterpretq_u8_u16(rgb.val[0]), rgb1 = vreinterpretq_u8_u16(rgb.val[1]);
        const auto adder_ = (base << 8) + 128; // pre-add round
        const auto adder = vdupq_n_u32(Idx == 0 ? (adder_ << 1) : adder_); // lumi use extra scale
        const auto sum0 = vdotq_laneq_u32(adder, rgb0, Lut, Idx);
        const auto sum1 = vdotq_laneq_u32(adder, rgb1, Lut, Idx);
        if constexpr (Idx == 0) // Y(lumi), scaled to 512, need to >> 1
            return vreinterpretq_u8_u16(vshrn_high_n_s32(vshrn_n_u32(sum0, 1), sum1, 1));
        else // chroma, shifted(pre add 0.5)
        {
            const auto zero = vdupq_n_u32(0); // pre-add round
            const auto shift0 = vdotq_laneq_u32(zero, rgb0, Lut, 3);
            const auto shift1 = vdotq_laneq_u32(zero, rgb1, Lut, 3);
            const auto ret0 = vreinterpretq_s32_u32(vsubq_u32(sum0, shift0)), ret1 = vreinterpretq_s32_u32(vsubq_u32(sum1, shift1));
            return vreinterpretq_u8_u16(vqmovun_high_s32(vqmovun_s32(sum0), sum1));
        }
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB8ToYUV8_I8DP4A_NEON : public RGB8ToYUV8_I8_NEON<R, G, B, RGB8ToYUV8_I8DP4A_NEON<R, G, B>>
{
    I8x16 Lut;
    RGB8ToYUV8_I8DP4A_NEON(const std::array<int8_t, 16>& lut) noexcept : Lut(lut.data()) {}
    template<uint8_t Idx>
    forceinline uint8x16_t DP4(const uint8x16_t& lo, const uint8x16_t& hi, const int32_t base) const noexcept
    {
        const auto rgb = vzipq_u16(vreinterpretq_u16_u8(lo), vreinterpretq_u16_u8(hi));
        const auto rgb0 = vreinterpretq_u8_u16(rgb.val[0]), rgb1 = vreinterpretq_u8_u16(rgb.val[1]);
        const auto adder = vdupq_n_s32((base << 8) + 128); // pre-add round
        const auto sum0 = vusdotq_laneq_s32(adder, rgb0, Lut, Idx);
        const auto sum1 = vusdotq_laneq_s32(adder, rgb1, Lut, Idx);
        if constexpr (Idx == 0) // Y(lumi)
            return vreinterpretq_u8_u16(vuzp1q_u16(vreinterpretq_u16_s32(sum0), vreinterpretq_u16_s32(sum1)));
        else
            return vreinterpretq_u8_u16(vqmovun_high_s32(vqmovun_s32(sum0), sum1));
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, NEON_F16)
{
    if (count >= RGB8ToYUV8_F16_NEON_Base::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_F16_NEON<0, 1, 2> proc(RGB8ToYCC8LUTF32[mval]);
        do
        {
            if (isYCCFull)
                proc.Convert<true>(dest, src, needAddY);
            else
                proc.Convert<false>(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, NEON_I16)
{
    if (count >= RGB8ToYUV8_I16_NEON_Base::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16_NEON<0, 1, 2> proc(RGB8ToYCC8LUTI16[mval]);
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, NEON_U8DP4A)
{
    if (count >= RGB8ToYUV8_I8_NEON_Base::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_U8DP4A_NEON<0, 1, 2> proc(RGB8ToYCC8LUTU8x4[mval]);
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, NEON_I8DP4A)
{
    if (count >= RGB8ToYUV8_I8_NEON_Base::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8DP4A_NEON<0, 1, 2> proc(RGB8ToYCC8LUTI8x4[mval]);
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, NEONA64)
{
    ProcessLOOP4<Gf2RGBAf_NEON64, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, NEONA64)
{
    ProcessLOOP4<GAf2RGBAf_NEON64, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, NEONA64)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEONA64<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, NEONA64)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEONA64<false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, NEONA64)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEONA64<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, NEONA64)
{
    ProcessLOOP4<RGB5x5ToRGB8_NEONA64<false, false>, &Func<LOOP>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
struct G2GA8_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U8x32 Alpha;
    forceinline G2GA8_256(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x32(src);
        const auto out = dat.Zip(Alpha);
        out.Data[0].As<U16x16>().Save(dst);
        out.Data[1].As<U16x16>().Save(dst + 16);
    }
};
struct G2GA8_256_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U16x16 Alpha;
    forceinline G2GA8_256_2(std::byte alpha) noexcept : Alpha(static_cast<uint16_t>(static_cast<uint16_t>(alpha) << 8)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = U8x16(src);
        const auto dat1 = U8x16(src + 16);
        const U16x16 mid0 = _mm256_cvtepu8_epi16(dat0);
        const U16x16 mid1 = _mm256_cvtepu8_epi16(dat1);
        const auto out0 = mid0.Or(Alpha);
        const auto out1 = mid1.Or(Alpha);
        out0.Save(dst);
        out1.Save(dst + 16);
    }
};
struct G2RGB8_256
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x32(src);
        const auto mid0 = dat.ShuffleLane< 0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5>();
        const auto mid1 = dat.ShuffleLane< 5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10>();
        const auto mid2 = dat.ShuffleLane<10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15>();
        const U8x32 out0 = mid0.PermuteLane<0, 2>(mid1);
        const U8x32 out1 = mid2.PermuteLane<0, 3>(mid0);
        const U8x32 out2 = mid1.PermuteLane<1, 3>(mid2);
        out0.Save(dst);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
struct G2RGBA8_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U8x32 Alpha;
    forceinline G2RGBA8_256(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x32(src);
        const auto rgLo = dat.ZipLoLane(dat).As<U16x16>(); // [0~7][10~17]
        const auto rgHi = dat.ZipHiLane(dat).As<U16x16>(); // [8~f][18~1f]
        const auto baLo = dat.ZipLoLane(Alpha).As<U16x16>(); // [0~7][10~17]
        const auto baHi = dat.ZipHiLane(Alpha).As<U16x16>(); // [8~f][18~1f]
        const auto rgbaLo = rgLo.Zip(baLo).As<U32x8>(); // [0~3][4~7] [10~13][14~17]
        const auto rgbaHi = rgHi.Zip(baHi).As<U32x8>(); // [8~b][c~f] [18~1b][1c~1f]
        rgbaLo.Data[0].Save(dst + 0);
        rgbaHi.Data[0].Save(dst + 8);
        rgbaLo.Data[1].Save(dst + 16);
        rgbaHi.Data[1].Save(dst + 24);
    }
};
struct G2RGBA8_256_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U32x8 Alpha;
    forceinline G2RGBA8_256_2(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = U8x16(src);
        const auto dat1 = U8x16(src + 16);
        const auto mid0 = U8x32::BroadcastLane(dat0);
        const auto mid1 = U8x32::BroadcastLane(dat1);
        const auto shuffleMaskLo = _mm256_setr_epi8( 0,  0,  0, -1,  1,  1,  1, -1,  2,  2,  2, -1,  3,  3,  3, -1,  4,  4,  4, -1,  5,  5,  5, -1,  6,  6,  6, -1,  7,  7,  7, -1);
        const auto shuffleMaskHi = _mm256_setr_epi8( 8,  8,  8, -1,  9,  9,  9, -1, 10, 10, 10, -1, 11, 11, 11, -1, 12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1);
        const U32x8 rgb0Lo = _mm256_shuffle_epi8(mid0, shuffleMaskLo);
        const U32x8 rgb0Hi = _mm256_shuffle_epi8(mid0, shuffleMaskHi);
        const U32x8 rgb1Lo = _mm256_shuffle_epi8(mid1, shuffleMaskLo);
        const U32x8 rgb1Hi = _mm256_shuffle_epi8(mid1, shuffleMaskHi);
        const auto out0 = rgb0Lo.Or(Alpha);
        const auto out1 = rgb0Hi.Or(Alpha);
        const auto out2 = rgb1Lo.Or(Alpha);
        const auto out3 = rgb1Hi.Or(Alpha);
        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct GA2RGB8_256
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x16(src).As<U8x32>(); // 0011223344556677 8899aabbccddeeff
        const auto dat1 = U16x16(src + 16).As<U8x32>(); // 0011223344556677 8899aabbccddeeff
        const auto dat01Lo = dat0.PermuteLane<0, 2>(dat1); // 0011223344556677
        const auto dat01Hi = dat0.PermuteLane<1, 3>(dat1); // 8899aabbccddeeff

        const auto val0 = dat01Lo.ShuffleLane<0, 0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10>(); // 0001112223334445
        const auto mid01 = dat01Lo.MoveLaneToLoWith<10>(dat01Hi); // 5566778899aabbcc
        const auto val1 = mid01.ShuffleLane<0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10, 10>(); // 55666777888999aa
        const auto val2 = dat01Hi.ShuffleLane<4, 6, 6, 6, 8, 8, 8, 10, 10, 10, 12, 12, 12, 14, 14, 14>(); // abbbcccdddeeefff

        const auto out0 = val0.PermuteLane<0, 2>(val1);
        const auto out1 = val2.PermuteLane<0, 3>(val0);
        const auto out2 = val1.PermuteLane<1, 3>(val2);
        out0.Save(dst);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
struct GA2RGBA8_256
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src).As<U8x32>();
        const auto mid0 = dat.ShuffleLane<0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7>();
        const auto mid1 = dat.ShuffleLane<8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12, 13, 14, 14, 14, 15>();
        const auto out0 = mid0.PermuteLane<0, 2>(mid1).As<U32x8>();
        const auto out1 = mid0.PermuteLane<1, 3>(mid1).As<U32x8>();
        out0.Save(dst);
        out1.Save(dst + 8);
    }
};
struct G2GA16_256
{
    static constexpr size_t M = 16, N = 32, K = 16;
    U16x16 Alpha;
    forceinline G2GA16_256(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst +  0);
        out[1].Save(dst + 16);
    }
};
struct G2RGB16_256
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src);
        const auto mid0 = dat.ShuffleLane<0, 0, 0, 1, 1, 1, 2, 2>(); // 00011122 888999aa
        const auto mid1 = dat.ShuffleLane<2, 3, 3, 3, 4, 4, 4, 5>(); // 23334445 1bbbcccd
        const auto mid2 = dat.ShuffleLane<5, 5, 6, 6, 6, 7, 7, 7>(); // 55666777 ddeeefff
        const auto out0 = mid0.PermuteLane<0, 2>(mid1); // 00011122 23334445
        const auto out1 = mid2.PermuteLane<0, 3>(mid0); // 55666777 888999aa
        const auto out2 = mid1.PermuteLane<1, 3>(mid2); // 1bbbcccd ddeeefff
        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct G2RGBA16_256
{
    static constexpr size_t M = 16, N = 64, K = 16;
    U16x16 Alpha;
    forceinline G2RGBA16_256(uint16_t alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src);
        const auto out0189 = dat.ShuffleLane<0, 0, 0, 0, 1, 1, 1, 1>().SelectWith<0x8888>(Alpha);
        const auto out23ab = dat.ShuffleLane<2, 2, 2, 2, 3, 3, 3, 3>().SelectWith<0x8888>(Alpha);
        const auto out45cd = dat.ShuffleLane<4, 4, 4, 4, 5, 5, 5, 5>().SelectWith<0x8888>(Alpha);
        const auto out67ef = dat.ShuffleLane<6, 6, 6, 6, 7, 7, 7, 7>().SelectWith<0x8888>(Alpha);
        const auto out0 = out0189.PermuteLane<0, 2>(out23ab);
        const auto out1 = out45cd.PermuteLane<0, 2>(out67ef);
        const auto out2 = out0189.PermuteLane<1, 3>(out23ab);
        const auto out3 = out45cd.PermuteLane<1, 3>(out67ef);
        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
        out3.Save(dst + 48);
    }
};
struct GA2RGB16_256
{
    static constexpr size_t M = 32, N = 48, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x16(src);
        const auto dat1 = U16x16(src + 16);

        const auto dat02 = dat0.PermuteLane<0, 2>(dat1);
        const auto dat13 = dat0.PermuteLane<1, 3>(dat1);

        const auto mid0 = dat02.ShuffleLane<0, 0, 0, 2, 2, 2, 4, 4>();
        const auto mid1 = dat02.SelectWith<0x0f0f>(dat13).ShuffleLane<4, 6, 6, 6, 0, 0, 0, 2>();
        const auto mid2 = dat13.ShuffleLane<2, 2, 4, 4, 4, 6, 6, 6>();

        const auto out0 = mid0.PermuteLane<0, 2>(mid1);
        const auto out1 = mid2.PermuteLane<0, 3>(mid0);
        const auto out2 = mid1.PermuteLane<1, 3>(mid2);

        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct GA2RGBA16_256
{
    static constexpr size_t M = 16, N = 32, K = 8;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src);
        const auto out02 = dat.ShuffleLane<0, 0, 0, 1, 2, 2, 2, 3>();
        const auto out13 = dat.ShuffleLane<4, 4, 4, 5, 6, 6, 6, 7>();
        const auto out0 = out02.PermuteLane<0, 2>(out13);
        const auto out1 = out02.PermuteLane<1, 3>(out13);
        out0.Save(dst +  0);
        out1.Save(dst + 16);
    }
};
struct G2GAf_256
{
    static constexpr size_t M = 8, N = 16, K = 8;
    F32x8 Alpha;
    forceinline G2GAf_256(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x8(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct G2RGBf_256
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x8(src);
        const auto mid0 = dat.ShuffleLane<0, 0, 0, 1>(); // 0001 4445
        const auto mid1 = dat.ShuffleLane<1, 1, 2, 2>(); // 1122 5566
        const auto mid2 = dat.ShuffleLane<2, 3, 3, 3>(); // 2333 6777
        const auto out0 = mid0.PermuteLane<0, 2>(mid1); // 0001 1122
        const auto out1 = mid2.PermuteLane<0, 3>(mid0); // 2333 4445
        const auto out2 = mid1.PermuteLane<1, 3>(mid2); // 5566 6777
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct G2RGBAf_256
{
    static constexpr size_t M = 8, N = 32, K = 8;
    F32x8 Alpha;
    forceinline G2RGBAf_256(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x8(src);
        const auto out04 = dat.BroadcastLane<0>().SelectWith<0b10001000>(Alpha);
        const auto out15 = dat.BroadcastLane<1>().SelectWith<0b10001000>(Alpha);
        const auto out26 = dat.BroadcastLane<2>().SelectWith<0b10001000>(Alpha);
        const auto out37 = dat.BroadcastLane<3>().SelectWith<0b10001000>(Alpha);
        const auto out0 = out04.PermuteLane<0, 2>(out15);
        const auto out1 = out26.PermuteLane<0, 2>(out37);
        const auto out2 = out04.PermuteLane<1, 3>(out15);
        const auto out3 = out26.PermuteLane<1, 3>(out37);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct GA2RGBf_256
{
    static constexpr size_t M = 16, N = 24, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = F32x8(src);
        const auto dat1 = F32x8(src + 8);

        const auto dat02 = dat0.PermuteLane<0, 2>(dat1);
        const auto dat13 = dat0.PermuteLane<1, 3>(dat1);

        const auto mid0 = dat02.ShuffleLane<0, 0, 0, 2>();
        const F32x8 mid1 = _mm256_shuffle_ps(dat02, dat13, 0b00001010);
        const auto mid2 = dat13.ShuffleLane<0, 2, 2, 2>();

        const auto out0 = mid0.PermuteLane<0, 2>(mid1);
        const auto out1 = mid2.PermuteLane<0, 3>(mid0);
        const auto out2 = mid1.PermuteLane<1, 3>(mid2);

        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct GA2RGBAf_256
{
    static constexpr size_t M = 8, N = 16, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x8(src);
        const auto out02 = dat.ShuffleLane<0, 0, 0, 1>();
        const auto out13 = dat.ShuffleLane<2, 2, 2, 3>();
        const auto out0 = out02.PermuteLane<0, 2>(out13);
        const auto out1 = out02.PermuteLane<1, 3>(out13);
        out0.Save(dst + 0);
        out1.Save(dst + 8);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_3To4_256
{
    static constexpr size_t M = 96, N = 32, K = 32;
    U32x8 Alpha;
    forceinline RGB8_3To4_256(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24u) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x32 dat0(src +  0);
        const U8x32 dat1(src + 32);
        const U8x32 dat2(src + 64);

        constexpr auto idx1hR = int8_t(IdxR - 1), idx1hG = int8_t(IdxG - 1), idx1hB = int8_t(IdxB - 1); // 0 will be -1
        const auto shuffleMask0 = _mm256_setr_epi8(
            IdxR + 0, IdxG + 0, IdxB + 0, -1, IdxR + 3, IdxG + 3, IdxB + 3, -1, IdxR + 6, IdxG + 6, IdxB + 6, -1, IdxR + 9, IdxG + 9, IdxB + 9, -1,
                  -1,       -1,       -1, -1,   idx1hR,   idx1hG,   idx1hB, -1, IdxR + 2, IdxG + 2, IdxB + 2, -1, IdxR + 5, IdxG + 5, IdxB + 5, -1);
        const U32x8 mid01 = _mm256_shuffle_epi8(dat0, shuffleMask0); // 00 01 02 x 03 04 05 x 06 07 08 x 09 0a 0b x | x x x x x 10 11 x 12 13 14 x 15 16 17 x

        const auto shuffleMask2 = _mm256_setr_epi8(
            IdxR + 4, IdxG + 4, IdxB + 4, -1, IdxR + 7, IdxG + 7, IdxB + 7, -1, IdxR + 10, IdxG + 10, IdxB + 10, -1, IdxR + 13, IdxG + 13, IdxB + 13, -1,
            IdxR + 0, IdxG + 0, IdxB + 0, -1, IdxR + 3, IdxG + 3, IdxB + 3, -1, IdxR +  6, IdxG +  6, IdxB +  6, -1, IdxR +  9, IdxG +  9, IdxB +  9, -1);
        const U32x8 mid34 = _mm256_shuffle_epi8(dat1, shuffleMask2); // 24 25 26 x 27 28 29 x 2a 2b 2c x 2d 2e 2f x | 30 31 32 x 33 34 35 x 36 37 38 x 39 3a 3b x

        const U32x8 dat12_ = dat0.As<U32x8>().SelectWith<0b00000001>(dat1.As<U32x8>()); // 20 21 22 23 x x x x x x x x 0c 0d 0e 0f | x x x x x x x x 18 19 1a 1b 1c 1d 1e 1f
        const U32x8 dat12 = dat12_.Shuffle<6, 7, 0, 0, 3, 0, 0, 0>(); // 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 20 21 22 23 | 0c 0d 0e 0f 20 21 22 23 20 21 22 23 20 21 22 23
        constexpr int8_t idx1lR = IdxR == 0 ? 3 : -1, idx1lG = IdxG == 0 ? 3 : -1, idx1lB = IdxB == 0 ? 3 : -1;
        const auto shuffleMask1 = _mm256_setr_epi8(
            IdxR + 0, IdxG + 0, IdxB + 0, -1, IdxR + 3, IdxG + 3, IdxB + 3, -1, IdxR + 6, IdxG + 6, IdxB + 6, -1, IdxR + 9, IdxG + 9, IdxB + 9, -1,
            IdxR + 0, IdxG + 0, IdxB + 0, -1,   idx1lR,   idx1lG,   idx1lB, -1,       -1,       -1,       -1, -1,       -1,       -1,       -1, -1);
        const U32x8 mid12 = _mm256_shuffle_epi8(dat12, shuffleMask1); // 18 19 1a x 1b 1c 1d x 1e 1f 20 x 21 22 23 x | 0c 0d 0e x 0f x x x x x x x x x x x

        const U32x8 merge01 = mid01.Or(mid12);
        const U32x8 out01 = mid01.SelectWith<0b00110000>(merge01).Or(Alpha);
        out01.Save(dst);

        const U32x8 merge23 = mid12.PermuteLane<0, 2>(mid34);
        const U32x8 out23 = merge23.Or(Alpha);
        out23.Save(dst + 8);
        
        constexpr int8_t idx6lR = IdxR == 2 ? -1 : IdxR + 14, idx6lG = IdxG == 2 ? -1 : IdxG + 14, idx6lB = IdxB == 2 ? -1 : IdxB + 14;
        const auto shuffleMask4 = _mm256_setr_epi8(
            IdxR + 8, IdxG + 8, IdxB + 8, -1, IdxR + 11, IdxG + 11, IdxB + 11, -1,    idx6lR,    idx6lG,    idx6lB, -1,        -1,        -1,        -1, -1,
            IdxR + 4, IdxG + 4, IdxB + 4, -1, IdxR +  7, IdxG +  7, IdxB +  7, -1, IdxR + 10, IdxG + 10, IdxB + 10, -1, IdxR + 13, IdxG + 13, IdxB + 13, -1);
        const U32x8 mid67 = _mm256_shuffle_epi8(dat2, shuffleMask4); // 48 49 4a x 4b 4c 4d x 4e 4f x x x x x x | 54 55 56 x 57 58 59 x 5a 5b 5c x 5d 5e 5f x

        const U32x8 dat56_ = dat2.As<U32x8>().SelectWith<0b10000000>(dat1.As<U32x8>()); // 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f | 50 51 52 53 54 55 56 57 58 59 5a 5b 3c 3d 3e 3f
        const U32x8 dat65 = dat56_.Shuffle<4, 0, 0, 0, 7, 0, 1, 0>(); // 50 51 52 53 40 41 42 43 40 41 42 43 40 41 42 43 | 3c 3d 3e 3f 40 41 42 43 44 45 46 47 40 41 42 43
        constexpr int8_t idx6hR = IdxR == 2 ? 0 : -1, idx6hG = IdxG == 2 ? 0 : -1, idx6hB = IdxB == 2 ? 0 : -1;
        const auto shuffleMask3 = _mm256_setr_epi8(
                  -1,       -1,       -1, -1,       -1,       -1,       -1, -1,   idx6hR,   idx6hG,   idx6hB, -1, IdxR + 1, IdxG + 1, IdxB + 1, -1,
            IdxR + 0, IdxG + 0, IdxB + 0, -1, IdxR + 3, IdxG + 3, IdxB + 3, -1, IdxR + 6, IdxG + 6, IdxB + 6, -1, IdxR + 9, IdxG + 9, IdxB + 9, -1);
        const U32x8 mid65 = _mm256_shuffle_epi8(dat65, shuffleMask3); // x x x x x x x x x x 50 x 51 52 53 x | 3c 3d 3e x 3f 40 41 x 42 43 44 x 45 46 47 x

        const U32x8 merge45 = mid34.PermuteLane<1, 3>(mid65);
        const U32x8 out45 = merge45.Or(Alpha);
        out45.Save(dst + 16);

        const U32x8 merge67 = mid67.Or(mid65);
        const U32x8 out67 = mid67.SelectWith<0b00001100>(merge67).Or(Alpha);
        out67.Save(dst + 24);
    }
};
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
struct RGB8_4To3_256
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x8 dat0(src);
        const U32x8 dat1(src + 8);
        const U32x8 dat2(src + 16);
        const U32x8 dat3(src + 24);

        const auto compressMask = _mm256_setr_epi8(
            IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4, IdxR + 8, IdxG + 8, IdxB + 8, IdxR + 12, IdxG + 12, IdxB + 12, -1, -1, -1, -1,
            IdxR + 0, IdxG + 0, IdxB + 0, IdxR + 4, IdxG + 4, IdxB + 4, IdxR + 8, IdxG + 8, IdxB + 8, IdxR + 12, IdxG + 12, IdxB + 12, -1, -1, -1, -1);
        const auto mid0123 = _mm256_shuffle_epi8(dat0, compressMask);
        const auto mid4567 = _mm256_shuffle_epi8(dat1, compressMask);
        const auto mid89ab = _mm256_shuffle_epi8(dat2, compressMask);
        const auto midcdef = _mm256_shuffle_epi8(dat3, compressMask);

        const U32x8 val012345 = _mm256_permutevar8x32_epi32(mid0123, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, 3, 7)); // 0123 45xx
        const U32x8 val6789ab = _mm256_permutevar8x32_epi32(mid4567, _mm256_setr_epi32(2, 4, 5, 6, 3, 7, 0, 1)); // 89ab xx67
        const U32x8 valcdefgh = _mm256_permutevar8x32_epi32(mid89ab, _mm256_setr_epi32(5, 6, 3, 7, 0, 1, 2, 4)); // ghxx cdef
        const U32x8 valijklmn = _mm256_permutevar8x32_epi32(midcdef, _mm256_setr_epi32(3, 7, 0, 1, 2, 4, 5, 6)); // xxij klmn
        
        const U32x8 merge4567 = val012345.Or(val6789ab); // xxxx 4567
        const auto out0 = val012345.SelectWith<0b11110000>(merge4567); // 01234567
        out0.As<U8x32>().Save(dst);

        const auto out1 = val6789ab.SelectWith<0b11110000>(valcdefgh); // 89abcdef
        out1.As<U8x32>().Save(dst + 32);

        const U32x8 mergeghij = valcdefgh.Or(valijklmn); // ghij xxxx
        const auto out2 = mergeghij.SelectWith<0b11110000>(valijklmn); // 01234567
        out2.As<U8x32>().Save(dst + 64);
    }
};
template<bool IsRGB>
struct RGB16_3To4_256
{
    static constexpr size_t M = 48, N = 64, K = 16;
    U16x16 Alpha;
    forceinline RGB16_3To4_256(uint16_t alpha) noexcept : Alpha(U64x4(static_cast<uint64_t>(alpha) << 48).As<U16x16>()) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src +  0); // R0 G0 B0 R1 G1 B1 R2 G2 | B2 R3 G3 B3 R4 G4 B4 R5
        const U16x16 dat1(src + 16); // G5 B5 R6 G6 B6 R7 G7 B7 | R8 G8 B8 R9 G9 B9 Ra Ga
        const U16x16 dat2(src + 32); // Ba Rb Gb Bb Rc Gc Bc Rd | Gd Bd Re Ge Be Rf Gf Df

        constexpr uint8_t R = IsRGB ? 0 : 4, G = 2, B = IsRGB ? 4 : 0;

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // R0 G0 B0 R1 G1 B1 R2 G2 | R8 G8 B8 R9 G9 B9 Ra Ga
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // B2 R3 G3 B3 R4 G4 B4 R5 | Ba Rb Gb Bb Rc Gc Bc Rd
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // G5 B5 R6 G6 B6 R7 G7 B7 | Gd Bd Re Ge Be Rf Gf Df
        
        const auto shuffleMask0 = _mm256_setr_epi8(R + 0, R + 1, G + 0, G + 1, B + 0, B + 1, -1, -1, R + 6, R + 7, G + 6, G + 7, B + 6, B + 7, -1, -1,
            R + 0, R + 1, G + 0, G + 1, B + 0, B + 1, -1, -1, R + 6, R + 7, G + 6, G + 7, B + 6, B + 7, -1, -1);
        const U16x16 out04 = _mm256_or_si256(_mm256_shuffle_epi8(dat03, shuffleMask0), Alpha); // RGBA0 RGBA1 | RGBA8 RGBA9

        const auto mid01 = dat03.SelectWith<0x0f0f>(dat14);
        const auto shuffleMask1 = _mm256_setr_epi8(R + 12, R + 13, G + 12, G + 13, B + 12, B + 13, -1, -1, R + 2, R + 3, G + 2, G + 3, B + 2, B + 3, -1, -1,
            R + 12, R + 13, G + 12, G + 13, B + 12, B + 13, -1, -1, R + 2, R + 3, G + 2, G + 3, B + 2, B + 3, -1, -1);
        const U16x16 out15 = _mm256_or_si256(_mm256_shuffle_epi8(mid01, shuffleMask1), Alpha); // RGBA2 RGBA3 | RGBAa RGBAb

        const auto mid12 = dat14.SelectWith<0x0f0f>(dat25);
        const auto shuffleMask2 = _mm256_setr_epi8(R + 8, R + 9, G + 8, G + 9, B + 8, B + 9, -1, -1, R + 14, R + 15, G + 14, G + 15, B + 14, B + 15, -1, -1,
            R + 8, R + 9, G + 8, G + 9, B + 8, B + 9, -1, -1, R + 14, R + 15, G + 14, G + 15, B + 14, B + 15, -1, -1);
        const U16x16 out26 = _mm256_or_si256(_mm256_shuffle_epi8(mid12, shuffleMask2), Alpha); // RGBA4 RGBA5 | RGBAc RGBAd

        const auto shuffleMask3 = _mm256_setr_epi8(R + 4, R + 5, G + 4, G + 5, B + 4, B + 5, -1, -1, R + 10, R + 11, G + 10, G + 11, B + 10, B + 11, -1, -1,
            R + 4, R + 5, G + 4, G + 5, B + 4, B + 5, -1, -1, R + 10, R + 11, G + 10, G + 11, B + 10, B + 11, -1, -1);
        const U16x16 out37 = _mm256_or_si256(_mm256_shuffle_epi8(dat25, shuffleMask3), Alpha); // RGBA6 RGBA7 | RGBAe RGBAf

        const auto out0 = out04.PermuteLane<0, 2>(out15);
        const auto out1 = out26.PermuteLane<0, 2>(out37);
        const auto out2 = out04.PermuteLane<1, 3>(out15);
        const auto out3 = out26.PermuteLane<1, 3>(out37);
        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
        out3.Save(dst + 48);
    }
};
template<bool IsRGB>
struct RGB16_4To3_256
{
    static constexpr size_t M = 64, N = 48, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src +  0);
        const U16x16 dat1(src + 16);
        const U16x16 dat2(src + 32);
        const U16x16 dat3(src + 48);

        __m256i shuffleMask;
        if constexpr (IsRGB)
            shuffleMask = _mm256_setr_epi8(0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1,
                0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1);
        else
            shuffleMask = _mm256_setr_epi8(4, 5, 2, 3, 0, 1, 12, 13, 10, 11, 8, 9, -1, -1, -1, -1,
                4, 5, 2, 3, 0, 1, 12, 13, 10, 11, 8, 9, -1, -1, -1, -1);
        const auto val0 = _mm256_shuffle_epi8(dat0, shuffleMask); // 0RGB 1RGB .. | 2RGB 3RGB ..
        const auto val1 = _mm256_shuffle_epi8(dat1, shuffleMask); // 4RGB 5RGB .. | 6RGB 7RGB ..
        const auto val2 = _mm256_shuffle_epi8(dat2, shuffleMask); // 8RGB 9RGB .. | aRGB bRGB ..
        const auto val3 = _mm256_shuffle_epi8(dat3, shuffleMask); // cRGB dRGB .. | eRGB fRGB ..

        const U64x4 mid0 = _mm256_permutevar8x32_epi32(val0, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, 3, 7)); // 0RGB 1RGB 2RG | 2B 3RGB ....
        const U64x4 mid1 = _mm256_permutevar8x32_epi32(val1, _mm256_setr_epi32(2, 4, 5, 6, 3, 7, 0, 1)); // 5GB 6RGB 7RGB | .... 4RGB 5R
        const U64x4 mid2 = _mm256_permutevar8x32_epi32(val2, _mm256_setr_epi32(5, 6, 3, 7, 0, 1, 2, 4)); // aB bRGB .... | 8RGB 9RGB aRG
        const U64x4 mid3 = _mm256_permutevar8x32_epi32(val3, _mm256_setr_epi32(3, 7, 0, 1, 2, 4, 5, 6)); // .... cRGB dR | dGB eRGB fRGB
        const auto out0 = mid0.SelectWith<0b1000>(mid1).As<U16x16>();
        const auto out1 = mid1.PermuteLane<0, 3>(mid2).As<U16x16>();
        const auto out2 = mid2.SelectWith<0b1110>(mid3).As<U16x16>();

        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
template<bool IsRGB>
struct RGBf_3To4_256
{
    static constexpr size_t M = 24, N = 32, K = 8;
    F32x8 Alpha;
    forceinline RGBf_3To4_256(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src +  0); // R0 G0 B0 R1 | G1 B1 R2 G2
        const F32x8 dat1(src +  8); // B2 R3 G3 B3 | R4 G4 B4 R5
        const F32x8 dat2(src + 16); // G5 B5 R6 G6 | B6 R7 G7 B7

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // R0 G0 B0 R1 | R4 G4 B4 R5
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // G1 B1 R2 G2 | G5 B5 R6 G6
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // B2 R3 G3 B3 | B6 R7 G7 B7

        F32x8 out04 = dat03.SelectWith<0b10001000>(Alpha);
        if constexpr (!IsRGB) out04 = out04.ShuffleLane<2, 1, 0, 3>();

        F32x8 out37 = dat25.MoveLaneToLoWith<1>(Alpha);
        if constexpr (!IsRGB) out37 = out37.ShuffleLane<2, 1, 0, 3>();

        F32x8 out15, out26;
        if constexpr (IsRGB)
        {
            const auto mid14 = dat14.SelectWith<0b01000100>(Alpha); // G1 B1 A G2 | G5 B5 A G6
            out15 = dat03.MoveLaneToLoWith<3>(mid14);
            const auto mid25 = dat25.SelectWith<0b00100010>(Alpha); // B2 A G3 B3 | B6 A G7 B7
            out26 = dat14.MoveLaneToLoWith<2>(mid25);
        }
        else
        {
            const auto mid03 = dat03.SelectWith<0b01000100>(Alpha); // R0 G0 A R1 | R4 G4 A R5
            out15 = _mm256_shuffle_ps(dat14, mid03, 0b10110001);
            const auto mid25 = dat25.SelectWith<0b10001000>(Alpha); // B2 R3 G3 A | B6 R7 G7 A
            const auto mid14 = dat14.ShuffleLane<0, 3, 2, 1>(); // G1 G2 R2 B1
            out26 = mid14.SelectWith<0b10011001>(mid25);
        }

        const auto out0 = out04.PermuteLane<0, 2>(out15);
        const auto out1 = out26.PermuteLane<0, 2>(out37);
        const auto out2 = out04.PermuteLane<1, 3>(out15);
        const auto out3 = out26.PermuteLane<1, 3>(out37);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
template<bool IsRGB>
struct RGBf_4To3_256
{
    static constexpr size_t M = 32, N = 24, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src +  0);
        const F32x8 dat1(src +  8);
        const F32x8 dat2(src + 16);
        const F32x8 dat3(src + 24);

        F32x8 val0, val1, val2, val3;
        if constexpr (IsRGB)
        {
            val0 = dat0.Shuffle<0, 1, 2, 4, 5, 6, 0, 0>(); // R0 G0 B0 R1 | G1 B1 R0 R0
            val1 = dat1.Shuffle<2, 4, 5, 6, 0, 0, 0, 1>(); // B2 R3 G3 B3 | R2 R2 R2 G2
            val2 = dat2.Shuffle<5, 6, 0, 0, 0, 1, 2, 4>(); // G5 B5 R4 R4 | R4 G4 B4 R5
            val3 = dat3.Shuffle<0, 0, 0, 1, 2, 4, 5, 6>(); // R6 R6 R6 G6 | B6 R7 G7 B7
        }
        else
        {
            val0 = dat0.Shuffle<2, 1, 0, 6, 5, 4, 0, 0>(); // B0 G0 R0 B1 | G1 R1 R0 R0
            val1 = dat1.Shuffle<0, 6, 5, 4, 0, 0, 2, 1>(); // R2 B3 G3 R3 | R2 R2 B2 G2
            val2 = dat2.Shuffle<5, 4, 0, 0, 2, 1, 0, 6>(); // G5 R5 R4 R4 | B4 G4 R4 B5
            val3 = dat3.Shuffle<0, 0, 2, 1, 0, 6, 5, 4>(); // R6 R6 B6 G6 | R6 B7 G7 R7
        }

        const auto out0 = val0.SelectWith<0b11000000>(val1);
        const auto out1 = val1.SelectWith<0b11110000>(val2);
        const auto out2 = val3.SelectWith<0b00000011>(val2);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct RGBToBGR8_256
{
    static constexpr size_t M = 96, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x32 dat0(src +  0); // 01
        const U8x32 dat1(src + 32); // 23
        const U8x32 dat2(src + 64); // 45

        const auto dat03 = dat0.SelectWith<0xffff0000u>(dat1);
        const auto dat14 = _mm256_permute2x128_si256(dat0, dat2, 0x21);
        const auto dat25 = dat1.SelectWith<0xffff0000u>(dat2);

        const auto mid0 = dat03.ShuffleLane<0, 2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12>(); // x000111222333444
        const auto merge01 = _mm256_alignr_epi8(dat14, dat03, 15); // 555666777888999a
        const auto shuffleMask01 = _mm256_setr_epi8(2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, -1, 2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, -1);
        const U8x32 mid01 = _mm256_shuffle_epi8(merge01, shuffleMask01); // 555666777888999.
        const auto merge12 = _mm256_alignr_epi8(dat25, dat14, 1); // 5666777888999aaa
        const auto shuffleMask12 = _mm256_setr_epi8(-1, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13, -1, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13);
        const U8x32 mid12 = _mm256_shuffle_epi8(merge12, shuffleMask12); // .666777888999aaa
        const auto mid2 = dat25.ShuffleLane<3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13, 0>(); // bbbcccdddeeefffx
        const U8x32 mid1lo = _mm256_bsrli_epi128(mid01, 1);
        const U8x32 mid1hi = _mm256_bslli_epi128(mid12, 1);

        const U8x32 out03 = _mm256_alignr_epi8(mid01, mid0, 1);
        const auto out14 = mid1lo.Or(mid1hi);
        const U8x32 out25 = _mm256_alignr_epi8(mid2, mid12, 15);

        const U8x32 out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.SelectWith<0xffff0000u>(out03);
        const U8x32 out2 = out14.PermuteLane<1, 3>(out25);
        out0.Save(dst);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
struct RGBToBGR8_256_2
{
    static constexpr size_t M = 96, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x32 dat0(src +  0); // 01
        const U8x32 dat1(src + 32); // 23
        const U8x32 dat2(src + 64); // 45

        const auto shuffleMask0 = _mm256_setr_epi8(
            2,  1, 0, 5, 4, 3, 8, 7,  6, 11, 10,  9, 14, 13, 12, -1,
            0, -1, 4, 3, 2, 7, 6, 5, 10,  9,  8, 13, 12, 11, -1, 15);
        const U8x32 mid0 = _mm256_shuffle_epi8(dat0, shuffleMask0); // 000111222333444x, 5x666777888999xa
        const auto shuffleMask1 = _mm256_setr_epi8(
            -1, 3, 2, 1, 6, 5, 4, 9, 8,  7, 12, 11, 10, 15, 14, 13,
             2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10,  9, 14, 13, 12, -1);
        const U8x32 mid1 = _mm256_shuffle_epi8(dat1, shuffleMask1); // xbbbcccdddeeefff, 000111222333444x
        const auto shuffleMask2 = _mm256_setr_epi8(
             0, -1, 4, 3, 2, 7, 6, 5, 10, 9,  8, 13, 12, 11, -1, 15,
            -1,  3, 2, 1, 6, 5, 4, 9,  8, 7, 12, 11, 10, 15, 14, 13);
        const U8x32 mid2 = _mm256_shuffle_epi8(dat2, shuffleMask2); // 5x666777888999xa, xbbbcccdddeeefff

        const auto merge01 = dat0.SelectWith<0x80000001u>(dat1); // 20,01,...,0e,0f | 10,11,...,1e,3f
        const auto merge012 = merge01.SelectWith<0x00014002u>(dat2); // 20,41,...,4e,0f | 50,11,...,1e,3f
        const auto mix012 = _mm256_permutevar8x32_epi32(merge012, _mm256_setr_epi32(0, 3, 4, 7, 0, 3, 4, 7)); // 20,41,x,x,x,x,4e,0f,50,11,x,x,x,x,1e,3f

        const auto shuffleMask0f = _mm256_setr_epi8(
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  9,
            -1,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0, -1);
        const U8x32 mid0f = _mm256_shuffle_epi8(mix012, shuffleMask0f); // ~xx11,xx0f~20xx -> xxxxxxxxxxxxxxx5, x5xxxxxxxxxxxxax
        const auto out0 = mid0.Or(mid0f);

        const auto shuffleMask1f = _mm256_setr_epi8(
            14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1);
        const U8x32 mid1f = _mm256_shuffle_epi8(mix012, shuffleMask1f); // 1exx~,~xx41 -> axxxxxxxxxxxxxxx, xxxxxxxxxxxxxxx5
        const auto out1 = mid1.Or(mid1f);

        const auto shuffleMask2f = _mm256_setr_epi8(
            -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  8, -1,
             6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        const U8x32 mid2f = _mm256_shuffle_epi8(mix012, shuffleMask2f); // xx3f~50xx,xx4e~ -> x5xxxxxxxxxxxxax, axxxxxxxxxxxxxxx
        const auto out2 = mid2.Or(mid2f);

        out0.Save(dst);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
struct RGBAToBGRA8_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x8 dat0(src);
        const U32x8 dat1(src + 8);
        const U32x8 dat2(src + 16);
        const U32x8 dat3(src + 24);

        const auto out0 = dat0.As<U8x32>().ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x8>();
        const auto out1 = dat1.As<U8x32>().ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x8>();
        const auto out2 = dat2.As<U8x32>().ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x8>();
        const auto out3 = dat3.As<U8x32>().ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15>().As<U32x8>();

        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct RGBToBGR16_256
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src +  0); // 01
        const U16x16 dat1(src + 16); // 23
        const U16x16 dat2(src + 32); // 45

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // R0 G0 B0 R1 G1 B1 R2 G2 | R8 G8 B8 R9 G9 B9 Ra Ga 
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // B2 R3 G3 B3 R4 G4 B4 R5 | Ba Rb Gb Bb Rc Gc Bc Rd 
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // G5 B5 R6 G6 B6 R7 G7 B7 | Gd Bd Re Ge Be Rf Gf Bf 

        const auto mix01  = dat03.SelectWith<0x8181>(dat14); // B2 G0 B0 R1 G1 B1 R2 R5 | Ba G8 B8 R9 G9 B9 Ra Rd 
        const auto mix012 = mix01.SelectWith<0x0202>(dat25); // B2 B5 B0 R1 G1 B1 R2 R5 | Ba Bd B8 R9 G9 B9 Ra Rd 

        const auto val0 = dat03.ShuffleLane<2, 1, 0, 5, 4, 3, 6, 7>(); // B0 G0 R0 B1 G1 R1 R2 G2 | B8 G8 R8 B9 G9 R9 Ra Ga
        const auto val1 = dat14.ShuffleLane<0, 3, 2, 1, 6, 5, 4, 7>(); // B2 B3 G3 R3 B4 G4 R4 R5 | Ba Bb Gb Rb Bc Gc Rc Rd
        const auto val2 = dat25.ShuffleLane<0, 1, 4, 3, 2, 7, 6, 5>(); // G5 B5 B6 G6 R6 B7 G7 R7 | Gd Bd Be Ge Re Bf Gf Rf
        const auto val012 = mix012.As<U32x8>().ShuffleLane<3, 1, 2, 0>().As<U16x16>(); // R2 R5 B0 R1 G1 B1 B2 B5 | Ra Rd B8 R9 G9 B9 Ba Bd 

        const auto out03 = val0.SelectWith<0x4040>(val012); // B0 G0 R0 B1 G1 R1 B2 G2 | B8 G8 R8 B9 G9 R9 Ba Ga
        const auto out14 = val1.SelectWith<0x8181>(val012); // R2 B3 G3 R3 B4 G4 R4 B5 | Ra Bb Gb Rb Bc Gc Rc Bd
        const auto out25 = val2.SelectWith<0x0202>(val012); // G5 R5 B6 G6 R6 B7 G7 R7 | Gd Rd Be Ge Re Bf Gf Rf

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);

        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct RGBAToBGRA16_256
{
    static constexpr size_t M = 64, N = 64, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src);
        const U16x16 dat1(src + 16);
        const U16x16 dat2(src + 32);
        const U16x16 dat3(src + 48);

        const auto out0 = dat0.ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out1 = dat1.ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out2 = dat2.ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7>();
        const auto out3 = dat3.ShuffleLane<2, 1, 0, 3, 6, 5, 4, 7>();

        out0.Save(dst);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
        out3.Save(dst + 48);
    }
};
struct RGBToBGRf_256
{
    static constexpr size_t M = 24, N = 24, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src +  0); // 01
        const F32x8 dat1(src +  8); // 23
        const F32x8 dat2(src + 16); // 45

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1);
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2);
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2);

        const auto mix01 = _mm256_shuffle_ps(dat03, dat14, 0b01001100); // R0 R1 G1 B1
        const auto mix12 = _mm256_shuffle_ps(dat14, dat25, 0b11001110); // R2 G2 B2 B3

        const F32x8 out03 = _mm256_shuffle_ps(dat03, mix01, 0b11000110); // B0 G0 R0 B1
        const F32x8 out14 = _mm256_shuffle_ps(mix01, mix12, 0b01100110); // G1 R1 B2 G2
        const F32x8 out25 = _mm256_shuffle_ps(mix12, dat25, 0b01101100); // R2 B3 G3 R3

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);
        out0.Save(dst +  0);
        out1.Save(dst +  8);
        out2.Save(dst + 16);
    }
};
struct RGBAToBGRAf_256
{
    static constexpr size_t M = 32, N = 32, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src);
        const F32x8 dat1(src + 8);
        const F32x8 dat2(src + 16);
        const F32x8 dat3(src + 24);

        const auto out0 = dat0.ShuffleLane<2, 1, 0, 3>();
        const auto out1 = dat1.ShuffleLane<2, 1, 0, 3>();
        const auto out2 = dat2.ShuffleLane<2, 1, 0, 3>();
        const auto out3 = dat3.ShuffleLane<2, 1, 0, 3>();

        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
template<uint8_t Idx>
struct KeepChannel8_3_1_256
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        constexpr uint32_t SelectMask[3] = { 0b01001001001001000100100100100100u, 0b10010010010010011001001001001001u, 0b00100100100100100010010010010010u };
        const U8x32 dat0(src);
        const U8x32 dat1(src + 32);
        const U8x32 dat2(src + 64);

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1);
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2);
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2);
        
        const U8x32 mix01  = dat03.SelectWith<SelectMask[(Idx + 0) % 3]>(dat14); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const U8x32 mix012 = mix01.SelectWith<SelectMask[(Idx + 2) % 3]>(dat25); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const auto shuffleMask = _mm256_setr_epi8(
            Idx + 0, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx + 15, Idx + 18, Idx + 21, Idx + 24, Idx + 27, Idx + 30, Idx + 33, Idx + 36, Idx + 39, Idx + 42, Idx + 45,
            Idx + 0, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx + 15, Idx + 18, Idx + 21, Idx + 24, Idx + 27, Idx + 30, Idx + 33, Idx + 36, Idx + 39, Idx + 42, Idx + 45);
        const U8x32 out = _mm256_shuffle_epi8(mix012, shuffleMask);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel16_3_1_256
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        constexpr uint16_t SelectMask[3] = { 0b1001001010010010, 0b0010010000100100, 0b0100100101001001 };
        const U16x16 dat0(src +  0);
        const U16x16 dat1(src + 16);
        const U16x16 dat2(src + 32);

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRG
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // BRGBRGBR, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // GBRGBRGB, R=1, G=2, B=0

        const U16x16 mix01  = dat03.SelectWith<SelectMask[(Idx + 0) % 3]>(dat14);
        const U16x16 mix012 = mix01.SelectWith<SelectMask[(Idx + 1) % 3]>(dat25);

        constexpr uint8_t I = Idx * 2;
        const auto shuffleMask = _mm256_setr_epi8(
            I + 0, I + 1, I + 6, I + 7, I + 12, I + 13, I + 18, I + 19, I + 24, I + 25, I + 30, I + 31, I + 36, I + 37, I + 42, I + 43,
            I + 0, I + 1, I + 6, I + 7, I + 12, I + 13, I + 18, I + 19, I + 24, I + 25, I + 30, I + 31, I + 36, I + 37, I + 42, I + 43);
        const U16x16 out = _mm256_shuffle_epi8(mix012, shuffleMask);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF_4_1_256
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = F32x8(src +  0).As<U32x8>();
        const auto dat1 = F32x8(src +  8).As<U32x8>();
        const auto dat2 = F32x8(src + 16).As<U32x8>();
        const auto dat3 = F32x8(src + 24).As<U32x8>();

        const auto mix01 = Idx >= 2 ? dat0.ZipHiLane(dat1).As<U64x4>() : dat0.ZipLoLane(dat1).As<U64x4>();
        const auto mix23 = Idx >= 2 ? dat2.ZipHiLane(dat3).As<U64x4>() : dat2.ZipLoLane(dat3).As<U64x4>();

        const auto mix = Idx % 2 ? mix01.ZipHiLane(mix23).As<F32x8>() : mix01.ZipLoLane(mix23).As<F32x8>(); // 0246 | 1357
        const auto out = mix.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>();
        out.Save(dst);
    }
};
template<int8_t Idx>
struct KeepChannelF_3_1_256
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src +  0); // RGBR | GBRG
        const F32x8 dat1(src +  8); // BRGB | RGBR
        const F32x8 dat2(src + 16); // GBRG | BRGB

        F32x8 out;
        if constexpr (Idx == 0)
        {
            const auto mix01 = dat0.SelectWith<0b10010010>(dat1); // 03x1 | 4x25
            const auto mix = mix01.SelectWith<0b00100100>(dat2);  // 0361 | 4725
            out = mix.Shuffle<0, 3, 6, 1, 4, 7, 2, 5>();
        }
        else if constexpr (Idx == 1)
        {
            const auto mix01 = dat0.SelectWith<0b00100100>(dat1); // x03x | 14x2
            const auto mix = mix01.SelectWith<0b01001001>(dat2);  // 5036 | 1472
            out = mix.Shuffle<1, 4, 7, 2, 5, 0, 3, 6>();
        }
        else
        {
            const auto mix01 = dat0.SelectWith<0b01001001>(dat1); // 2x03 | x14x
            const auto mix = mix01.SelectWith<0b10010010>(dat2);  // 2503 | 6147
            out = mix.Shuffle<2, 5, 0, 3, 6, 1, 4, 7>();
        }
        out.Save(dst);
    }
};
struct Extract8x2_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src);
        const U16x16 dat1(src + 16);

        const U8x32 mid0 = dat0.As<U8x32>().ShuffleLane<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>();
        const U8x32 mid1 = dat1.As<U8x32>().ShuffleLane<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>();

        const auto outLo = mid0.As<U64x4>().Shuffle<0, 2, 1, 3>(); // 0A0B 1A1B
        const auto outHi = mid1.As<U64x4>().Shuffle<0, 2, 1, 3>(); // 0C0D 1C1D
        const auto out0 = outLo.PermuteLane<0, 2>(outHi).As<U8x32>(); // 0 ABCD
        const auto out1 = outLo.PermuteLane<1, 3>(outHi).As<U8x32>(); // 1 ABCD

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract8x3_256
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        constexpr uint32_t SelectMask[3] = { 0b01001001001001000100100100100100u, 0b10010010010010011001001001001001u, 0b00100100100100100010010010010010u };
        const U8x32 dat0(src);
        const U8x32 dat1(src + 32);
        const U8x32 dat2(src + 64);

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const auto datR = dat03.SelectWith<SelectMask[0]>(dat14).SelectWith<SelectMask[2]>(dat25);
        const auto datG = dat03.SelectWith<SelectMask[1]>(dat14).SelectWith<SelectMask[0]>(dat25);
        const auto datB = dat03.SelectWith<SelectMask[2]>(dat14).SelectWith<SelectMask[1]>(dat25);

        const auto shuffleMaskR = _mm256_setr_epi8(0, 3, 6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 0, 3, 6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45);
        const auto shuffleMaskG = _mm256_setr_epi8(1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46);
        const auto shuffleMaskB = _mm256_setr_epi8(2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47);
        const U8x32 out0 = _mm256_shuffle_epi8(datR, shuffleMaskR);
        const U8x32 out1 = _mm256_shuffle_epi8(datG, shuffleMaskG);
        const U8x32 out2 = _mm256_shuffle_epi8(datB, shuffleMaskB);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract8x4_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x8 dat0(src);
        const U32x8 dat1(src + 8);
        const U32x8 dat2(src + 16);
        const U32x8 dat3(src + 24);

        const U8x32 mid0 = dat0.As<U8x32>().ShuffleLane<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // 0123 | 4567
        const U8x32 mid1 = dat1.As<U8x32>().ShuffleLane<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // 89ab | cdef
        const U8x32 mid2 = dat2.As<U8x32>().ShuffleLane<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // ghij | klmn
        const U8x32 mid3 = dat3.As<U8x32>().ShuffleLane<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(); // opqr | stuv

        const U32x8 val0819 = mid0.As<U32x8>().ZipLoLane(mid1.As<U32x8>()); // 0819 | 4c5d
        const U32x8 val2a3b = mid0.As<U32x8>().ZipHiLane(mid1.As<U32x8>()); // 2a3b | 6e7f
        const U32x8 valgohp = mid2.As<U32x8>().ZipLoLane(mid3.As<U32x8>()); // gohp | kslt
        const U32x8 valiqjr = mid2.As<U32x8>().ZipHiLane(mid3.As<U32x8>()); // iqjr | munv

        const auto val0 = val0819.ZipLoLane(valgohp); // 0g8o | 4kcs
        const auto val1 = val0819.ZipHiLane(valgohp); // 1h9p | 5ldt
        const auto val2 = val2a3b.ZipLoLane(valiqjr); // 2iaq | 6meu
        const auto val3 = val2a3b.ZipHiLane(valiqjr); // 3jbr | 7nfv

        const auto out0 = val0.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U8x32>(); // 048c | gkos
        const auto out1 = val1.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U8x32>(); // 159d | hlpt
        const auto out2 = val2.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U8x32>(); // 26ae | imqu
        const auto out3 = val3.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U8x32>(); // 37bf | jnrv
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract16x2_256
{
    static constexpr size_t M = 32, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src);
        const U16x16 dat1(src + 16);

        const auto mid0 = dat0.ShuffleLane<0, 2, 4, 6, 1, 3, 5, 7>().As<U64x4>(); // 0A1B
        const auto mid1 = dat1.ShuffleLane<0, 2, 4, 6, 1, 3, 5, 7>().As<U64x4>(); // 2C3D

        const auto outLo = mid0.Shuffle<0, 2, 1, 3>(); // 01AB
        const auto outHi = mid1.Shuffle<0, 2, 1, 3>(); // 23CD
        const auto out0 = outLo.PermuteLane<0, 2>(outHi).As<U16x16>(); // 0123
        const auto out1 = outLo.PermuteLane<1, 3>(outHi).As<U16x16>(); // ABCD

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract16x3_256
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        constexpr uint16_t SelectMask[3] = { 0b1001001010010010, 0b0010010000100100, 0b0100100101001001 };
        const U16x16 dat0(src +  0);
        const U16x16 dat1(src + 16);
        const U16x16 dat2(src + 32);

        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRG
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // BRGBRGBR, R=0b10010010, G=0b00100100, B=0b01001001
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // GBRGBRGB, R=0b00100100, G=0b01001001, B=0b10010010

        const auto datR = dat03.SelectWith<SelectMask[0]>(dat14).SelectWith<SelectMask[1]>(dat25);
        const auto datG = dat03.SelectWith<SelectMask[1]>(dat14).SelectWith<SelectMask[2]>(dat25);
        const auto datB = dat03.SelectWith<SelectMask[2]>(dat14).SelectWith<SelectMask[0]>(dat25);

        const auto shuffleMaskR = _mm256_setr_epi8(0, 1,  6,  7, 12, 13, 18, 19, 24, 25, 30, 31, 36, 37, 42, 43, 0, 1,  6,  7, 12, 13, 18, 19, 24, 25, 30, 31, 36, 37, 42, 43);
        const auto shuffleMaskG = _mm256_setr_epi8(2, 3,  8,  9, 14, 15, 20, 21, 26, 27, 32, 33, 38, 39, 44, 45, 2, 3,  8,  9, 14, 15, 20, 21, 26, 27, 32, 33, 38, 39, 44, 45);
        const auto shuffleMaskB = _mm256_setr_epi8(4, 5, 10, 11, 16, 17, 22, 23, 28, 29, 34, 35, 40, 41, 46, 47, 4, 5, 10, 11, 16, 17, 22, 23, 28, 29, 34, 35, 40, 41, 46, 47);
        const U16x16 out0 = _mm256_shuffle_epi8(datR, shuffleMaskR);
        const U16x16 out1 = _mm256_shuffle_epi8(datG, shuffleMaskG);
        const U16x16 out2 = _mm256_shuffle_epi8(datB, shuffleMaskB);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract16x4_256
{
    static constexpr size_t M = 64, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict * __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src);
        const U16x16 dat1(src + 16);
        const U16x16 dat2(src + 32);
        const U16x16 dat3(src + 48);

        const auto mid0 = dat0.ShuffleLane<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x8>(); // 0123 | 4567
        const auto mid1 = dat1.ShuffleLane<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x8>(); // 89ab | cdef
        const auto mid2 = dat2.ShuffleLane<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x8>(); // ghij | klmn
        const auto mid3 = dat3.ShuffleLane<0, 4, 1, 5, 2, 6, 3, 7>().As<U32x8>(); // opqr | stuv

        const auto val0819 = mid0.ZipLoLane(mid1); // 0819 | 4c5d
        const auto val2a3b = mid0.ZipHiLane(mid1); // 2a3b | 6e7f
        const auto valgohp = mid2.ZipLoLane(mid3); // gohp | kslt
        const auto valiqjr = mid2.ZipHiLane(mid3); // iqjr | munv

        const auto val0 = val0819.ZipLoLane(valgohp); // 0g8o | 4kcs
        const auto val1 = val0819.ZipHiLane(valgohp); // 1h9p | 5ldt
        const auto val2 = val2a3b.ZipLoLane(valiqjr); // 2iaq | 6meu
        const auto val3 = val2a3b.ZipHiLane(valiqjr); // 3jbr | 7nfv

        const auto out0 = val0.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U16x16>(); // 048c | gkos
        const auto out1 = val1.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U16x16>(); // 159d | hlpt
        const auto out2 = val2.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U16x16>(); // 26ae | imqu
        const auto out3 = val3.Shuffle<0, 4, 2, 6, 1, 5, 3, 7>().As<U16x16>(); // 37bf | jnrv
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract32x2_256
{
    static constexpr size_t M = 16, N = 8, K = 8;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src + 0); // 0123 | 4567
        const F32x8 dat1(src + 8); // 89ab | cdef

        const F64x4 mid0 = _mm256_castps_pd(_mm256_shuffle_ps(dat0, dat1, 0b10001000)); // 028a | 46ce
        const F64x4 mid1 = _mm256_castps_pd(_mm256_shuffle_ps(dat0, dat1, 0b11011101)); // 139b | 57df

        const F32x8 out0 = mid0.Shuffle<0, 2, 1, 3>().As<F32x8>(); // 0246  | 8ace
        const F32x8 out1 = mid1.Shuffle<0, 2, 1, 3>().As<F32x8>(); // 1357  | 9bdf

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract32x3_256
{
    static constexpr size_t M = 24, N = 8, K = 8;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const F32x8 dat0(src +  0); // 0123 | 4567
        const F32x8 dat1(src +  8); // 89ab | cdef
        const F32x8 dat2(src + 16); // ghij | klmn

        const F32x8 mid0 = _mm256_permutevar8x32_ps(dat0, _mm256_setr_epi32(0, 3, 6, 0, 1, 4, 7, 0)); // 0360 | 1470
        const F32x8 mid1 = _mm256_permutevar8x32_ps(dat1, _mm256_setr_epi32(5, 0, 3, 1, 6, 4, 7, 2)); // d8b9 | ecfa
        const F32x8 mid2 = _mm256_permutevar8x32_ps(dat2, _mm256_setr_epi32(0, 0, 3, 6, 0, 1, 4, 7)); // ggjm | ghkn
        const F32x8 mix02 = _mm256_shuffle_ps(dat0, dat2, 0b10011001); // 12hi | 56lm

        const auto out01Lo = mid0.SelectWith<0b10001000>(mid1); // 0369 | 147a
        const auto out12Hi = mid2.SelectWith<0b00010001>(mid1); // dgjm | ehkn
        const F32x8 mid3 = _mm256_permutevar8x32_ps(mix02, _mm256_setr_epi32(1, 4, 0, 0, 0, 0, 3, 6)); // 2511 | 11il
        const F32x8 mid4 = _mm256_shuffle_ps(mid1, mid1, 0b10011001); // 8b8b | cfcf
        const auto out2Lo0Hi = mid3.SelectWith<0b00111100>(mid4); // 258b | cfil

        const auto out0 = out01Lo.PermuteLane<0, 3>(out2Lo0Hi);
        const auto out1 = out01Lo.PermuteLane<1, 2>(out12Hi);
        const auto out2 = out2Lo0Hi.PermuteLane<0, 3>(out12Hi);

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract32x4_256
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const U32x8 dat0 = F32x8(src +  0).As<U32x8>(); // 0123 | 4567
        const U32x8 dat1 = F32x8(src +  8).As<U32x8>(); // 89ab | cdef
        const U32x8 dat2 = F32x8(src + 16).As<U32x8>(); // ghij | klmn
        const U32x8 dat3 = F32x8(src + 24).As<U32x8>(); // opqr | stuv

        const auto mid0 = dat0.ZipLoLane(dat1); // 0819 | 4c5d
        const auto mid1 = dat2.ZipLoLane(dat3); // gohp | kslt
        const auto mid2 = dat0.ZipHiLane(dat1); // 2a3b | 6e7f
        const auto mid3 = dat2.ZipHiLane(dat3); // iqjr | munv

        const auto mix0 = mid0.PermuteLane<0, 2>(mid1); // 0819 | gohp
        const auto mix1 = mid0.PermuteLane<1, 3>(mid1); // 4c5d | kslt
        const auto mix2 = mid2.PermuteLane<0, 2>(mid3); // 2a3b | iqjr
        const auto mix3 = mid2.PermuteLane<1, 3>(mid3); // 6e7f | munv

        const auto out0 = mix0.ZipLoLane(mix1).As<F32x8>(); // 048c | gkos
        const auto out1 = mix0.ZipHiLane(mix1).As<F32x8>(); // 159d | hlpt
        const auto out2 = mix2.ZipLoLane(mix3).As<F32x8>(); // 26ae | imqu
        const auto out3 = mix2.ZipHiLane(mix3).As<F32x8>(); // 37bf | jnrv

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Combine8x2_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x32 dat0(src[0]);
        const U8x32 dat1(src[1]);

        const auto out = dat0.Zip(dat1).As<U16x16>();
        out[0].Save(dst + 0);
        out[1].Save(dst + 16);
    }
};
struct Combine8x3_256
{
    static constexpr size_t M = 32, N = 96, K = 32;
    static forceinline void DoStore(uint8_t* __restrict dst, const U8x32& dat0, const U8x32& dat1, const U8x32& dat2) noexcept
    {
        // RGBRGBRGBRGBRGBR, R=1001001001001001 G=0100100100100100 B=0010010010010010
        // GBRGBRGBRGBRGBRG, R=0010010010010010 G=1001001001001001 B=0100100100100100
        // BRGBRGBRGBRGBRGB, R=0100100100100100 G=0010010010010010 B=1001001001001001
        const auto datR = dat0.ShuffleLane< 0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15, 10,  5>();
        const auto datG = dat1.ShuffleLane< 5,  0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15, 10>();
        const auto datB = dat2.ShuffleLane<10,  5,  0, 11,  6,  1, 12,  7,  2, 13,  8,  3, 14,  9,  4, 15>();

        const auto out03 = datR.SelectWith<0b00100100100100100010010010010010u>(datG).SelectWith<0b01001001001001000100100100100100u>(datB);
        const auto out14 = datR.SelectWith<0b10010010010010011001001001001001u>(datG).SelectWith<0b00100100100100100010010010010010u>(datB);
        const auto out25 = datR.SelectWith<0b01001001001001000100100100100100u>(datG).SelectWith<0b10010010010010011001001001001001u>(datB);

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);
        out0.Save(dst + 0);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }

    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x32 dat0(src[0]);
        const U8x32 dat1(src[1]);
        const U8x32 dat2(src[2]);
        DoStore(dst, dat0, dat1, dat2);
    }
};
struct Combine8x4_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x32 dat0(src[0]);
        const U8x32 dat1(src[1]);
        const U8x32 dat2(src[2]);
        const U8x32 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<U16x16>();
        const auto mid23 = dat2.Zip(dat3).As<U16x16>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<U32x8>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<U32x8>();
        out01[0].Save(dst +  0);
        out01[1].Save(dst +  8);
        out23[0].Save(dst + 16);
        out23[1].Save(dst + 24);
    }
};
struct Combine16x2_256
{
    static constexpr size_t M = 16, N = 32, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x16 dat0(src[0]);
        const U16x16 dat1(src[1]);

        const auto out = dat0.Zip(dat1);
        out[0].Save(dst + 0);
        out[1].Save(dst + 16);
    }
};
struct Combine16x3_256
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x16 dat0(src[0]);
        const U16x16 dat1(src[1]);
        const U16x16 dat2(src[2]);
        // RGBRGBRG, R=10010010 G=01001001 B=00100100
        // BRGBRGBR, R=01001001 G=00100100 B=10010010
        // GBRGBRGB, R=00100100 G=10010010 B=01001001

        const auto datR = dat0.ShuffleLane<0, 3, 6, 1, 4, 7, 2, 5>();
        const auto datG = dat1.ShuffleLane<5, 0, 3, 6, 1, 4, 7, 2>();
        const auto datB = dat2.ShuffleLane<2, 5, 0, 3, 6, 1, 4, 7>();

        const auto out03 = datR.SelectWith<0b1001001010010010>(datG).SelectWith<0b0010010000100100>(datB);
        const auto out14 = datR.SelectWith<0b0010010000100100>(datG).SelectWith<0b0100100101001001>(datB);
        const auto out25 = datR.SelectWith<0b0100100101001001>(datG).SelectWith<0b1001001010010010>(datB);

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);
        out0.Save(dst + 0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct Combine16x4_256
{
    static constexpr size_t M = 16, N = 64, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const U16x16 dat0(src[0]);
        const U16x16 dat1(src[1]);
        const U16x16 dat2(src[2]);
        const U16x16 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<U32x8>();
        const auto mid23 = dat2.Zip(dat3).As<U32x8>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<U16x16>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<U16x16>();
        out01[0].Save(dst + 0);
        out01[1].Save(dst + 16);
        out23[0].Save(dst + 32);
        out23[1].Save(dst + 48);
    }
};
struct Combine32x2_256
{
    static constexpr size_t M = 8, N = 16, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x8 dat0(src[0]);
        const F32x8 dat1(src[1]);

        const auto out = dat0.Zip(dat1);
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct Combine32x3_256
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x8 dat0(src[0]);
        const F32x8 dat1(src[1]);
        const F32x8 dat2(src[2]);

        const auto valR02G02 = _mm256_shuffle_ps(dat0, dat1, 0b10001000); // R0R2 G0G2
        const auto valR13B02 = _mm256_shuffle_ps(dat0, dat2, 0b10001101); // R1R3 B0B2
        const auto valG13B13 = _mm256_shuffle_ps(dat1, dat2, 0b11011101); // G1G3 B1B3

        const F32x8 out03 = _mm256_shuffle_ps(valR02G02, valR13B02, 0b00101000); // R0G0 B0R1
        const F32x8 out14 = _mm256_shuffle_ps(valG13B13, valR02G02, 0b11011000); // G1B1 R2G2
        const F32x8 out25 = _mm256_shuffle_ps(valR13B02, valG13B13, 0b11010111); // B2R3 G3B3

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);

        out0.Save(dst + 0);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
    }
};
struct Combine32x4_256
{
    static constexpr size_t M = 8, N = 32, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const F32x8 dat0(src[0]);
        const F32x8 dat1(src[1]);
        const F32x8 dat2(src[2]);
        const F32x8 dat3(src[3]);

        const auto mid01 = dat0.Zip(dat1).As<F64x4>();
        const auto mid23 = dat2.Zip(dat3).As<F64x4>();

        const auto out01 = mid01[0].Zip(mid23[0]).As<F32x8>();
        const auto out23 = mid01[1].Zip(mid23[1]).As<F32x8>();
        out01[0].Save(dst +  0);
        out01[1].Save(dst +  8);
        out23[0].Save(dst + 16);
        out23[1].Save(dst + 24);
    }
};
struct G8ToG16_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x32 dat(src);

        const auto out = dat.Zip(dat).As<U16x16>();
        out[0].Save(dst +  0);
        out[1].Save(dst + 16);
    }
};
struct G16ToG8_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat0(src);
        const U16x16 dat1(src + 16);
        const U16x16 muler((uint16_t)0xff01u); // y => x / 0xffff * 0xff => x / 257 => ((x << 24) / 257) >> 24 => (x * (1 << 24) / 257) >> 24 ~> (x * 0xff01) >> 24

        const auto mid0 = dat0.MulHi(muler);
        const auto mid1 = dat1.MulHi(muler);

        const auto val0 = mid0.As<U8x32>().ShuffleLane<1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14>().As<U64x4>().Shuffle<0, 2, 0, 2>(); // 01 01
        const auto val1 = mid1.As<U8x32>().ShuffleLane<1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14>().As<U64x4>().Shuffle<0, 2, 0, 2>(); // 23 23
        const auto out = val0.SelectWith<0b1100>(val1).As<U8x32>();

        out.Save(dst);
    }
};
template<bool Is555>
struct RGB5x5ToRGB8_256Base
{
    forceinline static std::array<U16x16, 3> RGB5x5ToRGB8Hi(const U16x16& dat) noexcept
    {
        const U16x16 maskLo5bit(uint16_t(0x1fu));
        const U16x16 maskLo6bit(uint16_t(0x3fu));
        const auto valR = dat.And(maskLo5bit);
        const auto valG = dat.ShiftRightLogic<5>().And(Is555 ? maskLo5bit : maskLo6bit);
        U16x16 valB = dat.ShiftRightLogic<Is555 ? 10 : 11>();
        if constexpr (Is555) valB = valB.And(maskLo5bit);

        // pre-shift-left by 2
        const U16x16 add5(23 << 2), add6(33 << 2);
        constexpr uint16_t muler5 = 527 << 2, muler6 = 259 << 2;
        const U16x16 mul5(muler5), mul6(muler6);
        const U16x16 midR = valR.MulAddLo(mul5, add5); // at Hi
        const U16x16 midG = valG.MulAddLo(Is555 ? mul5 : mul6, Is555 ? add5 : add6); // at Hi
        const U16x16 midB = valB.MulAddLo(mul5, add5); // at Hi

        return { midR, midG, midB };
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_256 : RGB5x5ToRGB8_256Base<Is555>
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x16(src +  0); // 01234567 | 89abcdef
        const auto dat1 = U16x16(src + 16); // ghijklmn | opqrstuv

        const auto [midR0, midG0, midB0] = this->RGB5x5ToRGB8Hi(dat0);
        const auto [midR1, midG1, midB1] = this->RGB5x5ToRGB8Hi(dat1);

        const U8x32 midRB0 = (IsRGB ? midR0 : midB0).template As<U8x32>().ZipEven((IsRGB ? midB0 : midR0).template As<U8x32>());
        const U8x32 midRB1 = (IsRGB ? midR1 : midB1).template As<U8x32>().ZipEven((IsRGB ? midB1 : midR1).template As<U8x32>());

        const auto shuffle02_RB = _mm256_setr_epi8(0, -1, 1, 2, -1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1, 13, 14, -1, 15);
        const auto shuffle02_G  = _mm256_setr_epi8(-1, 1, -1, -1, 3, -1, -1, 5, -1, -1, 7, -1, -1, 9, -1, -1, -1, -1, 7, -1, -1, 9, -1, -1, 11, -1, -1, 13, -1, -1, 15, -1);
        const U8x32 out02R_B = _mm256_shuffle_epi8(midRB0, shuffle02_RB);
        const U8x32 out02_G_ = _mm256_shuffle_epi8(midG0,  shuffle02_G);
        const auto out02 = out02R_B.Or(out02_G_);
        const U8x32 out35R_B = _mm256_shuffle_epi8(midRB1, shuffle02_RB);
        const U8x32 out35_G_ = _mm256_shuffle_epi8(midG1,  shuffle02_G);
        const auto out35 = out35R_B.Or(out35_G_);

        const auto mixRB01 = F64x4(_mm256_shuffle_pd(midRB0.As<F64x4>(), midRB1.As<F64x4>(), 0b0011)); // 4567klmn | 89abopqr
        const auto mixG01  = F64x4(_mm256_shuffle_pd(midG0.template As<F64x4>(), midG1.template As<F64x4>(), 0b0011)); // 4567klmn | 89abopqr
        const auto midRB01 = mixRB01.Shuffle<0, 2, 1, 3>().As<U8x32>(); // 456789abo | klmnopqr
        const auto midG01  = mixG01 .Shuffle<0, 2, 1, 3>().As<U8x32>(); // 456789abo | klmnopqr
        const auto shuffle1_RB = _mm256_setr_epi8(-1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1, -1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1);
        const auto shuffle1_G  = _mm256_setr_epi8(3, -1, -1, 5, -1, -1, 7, -1, -1, 9, -1, -1, 11, -1, -1, 13, 3, -1, -1, 5, -1, -1, 7, -1, -1, 9, -1, -1, 11, -1, -1, 13);
        const U8x32 out14R_B = _mm256_shuffle_epi8(midRB01, shuffle1_RB);
        const U8x32 out14_G_ = _mm256_shuffle_epi8(midG01,  shuffle1_G);
        const auto out14 = out14R_B.Or(out14_G_);

        const auto out0 = out02.PermuteLane<0, 2>(out14);
        const auto out1 = out02.PermuteLane<1, 2>(out35);
        const auto out2 = out14.PermuteLane<1, 3>(out35);
        out0.Save(dst +  0);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
struct RGB5x5ToRGB8_256_LUT
{
    static constexpr std::array<uint8_t, 64> LUT5 = []()
    {
        std::array<uint8_t, 64> lut = { 0 };
        for (uint32_t i = 0; i < 64; i++)
        {
            const auto j = i / 32;
            const auto k = j * 16 + (i % 16);
            lut[i] = static_cast<uint8_t>((k * 527 + 23) >> 6);
        }
        return lut;
    }();
    static constexpr std::array<uint8_t, 128> LUT6 = []()
    {
        std::array<uint8_t, 128> lut = { 0 };
        for (uint32_t i = 0; i < 128; i++)
        {
            const auto j = i / 32;
            const auto k = j * 16 + (i % 16);
            lut[i] = static_cast<uint8_t>((k * 259 + 33) >> 6);
        }
        return lut;
    }();
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_256LUTBase : RGB5x5ToRGB8_256_LUT
{
    forceinline static std::array<U8x32, 3> RGB5x5ToRGB8(const U16x16& dat0, const U16x16& dat1) noexcept
    {
        const auto maskHi = U16x16(uint16_t(0xff00u)).As<U8x32>();
        const U8x32 mask5bit(uint8_t(0x1fu));
        const U8x32 mask6bit(uint8_t(0x3fu));
        U8x32 val0RB, val1RB;
        if constexpr (IsRGB)
        {
            const U16x16 maskRB(uint16_t(Is555 ? 0x7c1fu : 0xf81fu));
            const auto dat0_ = dat0.And(maskRB), dat1_ = dat1.And(maskRB);
            const auto val0B = dat0_.ShiftRightLogic<Is555 ? 2 : 3>();
            const auto val1B = dat1_.ShiftRightLogic<Is555 ? 2 : 3>();
            val0RB = dat0_.template As<U8x32>().template SelectWith<MaskType::FullEle>(val0B.template As<U8x32>(), maskHi);
            val1RB = dat1_.template As<U8x32>().template SelectWith<MaskType::FullEle>(val1B.template As<U8x32>(), maskHi);
        }
        else
        {
            const auto val0R = dat0.ShiftLeftLogic<8>();
            const auto val1R = dat1.ShiftLeftLogic<8>();
            const auto val0B = dat0.ShiftRightLogic<Is555 ? 10 : 11>();
            const auto val1B = dat1.ShiftRightLogic<Is555 ? 10 : 11>();
            val0RB = val0R.Or(val0B).template As<U8x32>().And(mask5bit);
            val1RB = val1R.Or(val1B).template As<U8x32>().And(mask5bit);
        }
        const auto val0G = dat0.ShiftRightLogic<5>();
        const auto val1G = dat1.ShiftLeftLogic<3>();
        const auto valG = val0G.template As<U8x32>().template SelectWith<MaskType::FullEle>(val1G.template As<U8x32>(), maskHi).And(Is555 ? mask5bit : mask6bit);

        const U8x32 lut5lo(&LUT5[0]), lut5hi(&LUT5[32]);
        const auto midLo0RB = _mm256_shuffle_epi8(lut5lo, val0RB), midHi0RB = _mm256_shuffle_epi8(lut5hi, val0RB);
        const auto midLo1RB = _mm256_shuffle_epi8(lut5lo, val1RB), midHi1RB = _mm256_shuffle_epi8(lut5hi, val1RB);
        const auto mid0RB = _mm256_blendv_epi8(midLo0RB, midHi0RB, _mm256_slli_epi16(val0RB, 3));
        const auto mid1RB = _mm256_blendv_epi8(midLo1RB, midHi1RB, _mm256_slli_epi16(val1RB, 3));
        U8x32 midG;
        if constexpr (Is555)
        {
            const auto midLoG = _mm256_shuffle_epi8(lut5lo, valG), midHiG = _mm256_shuffle_epi8(lut5hi, valG);
            midG = _mm256_blendv_epi8(midLoG, midHiG, _mm256_slli_epi16(valG, 3));
        }
        else
        {
            const U8x32 lut60(&LUT6[0]), lut61(&LUT6[32]), lut62(&LUT6[64]), lut63(&LUT6[96]);
            const auto mid0G = _mm256_shuffle_epi8(lut60, valG), mid1G = _mm256_shuffle_epi8(lut61, valG),
                mid2G = _mm256_shuffle_epi8(lut62, valG), mid3G = _mm256_shuffle_epi8(lut63, valG);
            const auto midLoG = _mm256_blendv_epi8(mid0G, mid1G, _mm256_slli_epi16(valG, 3)),
                midHiG = _mm256_blendv_epi8(mid2G, mid3G, _mm256_slli_epi16(valG, 3));
            midG = _mm256_blendv_epi8(midLoG, midHiG, _mm256_slli_epi16(valG, 2));
        }

        return { mid0RB, mid1RB, midG };
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_LUT_256 : RGB5x5ToRGB8_256LUTBase<IsRGB, Is555>
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x16(src +  0); // 01234567 | 89abcdef
        const auto dat1 = U16x16(src + 16); // ghijklmn | opqrstuv

        const auto [midRB0, midRB1, midG] = this->RGB5x5ToRGB8(dat0, dat1);
        // midG: 0g1h2i3j4k5l6m7n | 8o9paqbrcsdteufv

        const auto shuffle02_RB = _mm256_setr_epi8(0, -1, 1, 2, -1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1, 13, 14, -1, 15);
        const auto shuffle02_G = _mm256_setr_epi8(-1, 0, -1, -1, 2, -1, -1, 4, -1, -1, 6, -1, -1, 8, -1, -1, -1, -1, 6, -1, -1, 8, -1, -1, 10, -1, -1, 12, -1, -1, 14, -1);
        const U8x32 out02R_B = _mm256_shuffle_epi8(midRB0, shuffle02_RB);
        const U8x32 out02_G_ = _mm256_shuffle_epi8(midG, shuffle02_G);
        const auto out02 = out02R_B.Or(out02_G_);
        const U8x32 out35R_B = _mm256_shuffle_epi8(midRB1, shuffle02_RB);
        const U8x32 out35_G_ = _mm256_shuffle_epi8(midG.template As<U16x16>().template ShiftRightLogic<8>(), shuffle02_G);
        const auto out35 = out35R_B.Or(out35_G_);

        const auto mixRB01 = F64x4(_mm256_shuffle_pd(midRB0.template As<F64x4>(), midRB1.template As<F64x4>(), 0b0011)); // 4567klmn | 89abopqr
        const auto midRB01 = mixRB01.Shuffle<0, 2, 1, 3>().As<U8x32>(); // 456789abo | klmnopqr
        const auto midG01 = midG.template As<F32x8>().template Shuffle<2, 3, 4, 5, 2, 3, 4, 5>().template As<U8x32>(); // 4k5l6m7n8o9paqbr | 4k5l6m7n8o9paqbr
        const auto shuffle1_RB = _mm256_setr_epi8(-1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1, -1, 3, 4, -1, 5, 6, -1, 7, 8, -1, 9, 10, -1, 11, 12, -1);
        const auto shuffle1_G = _mm256_setr_epi8(2, -1, -1, 4, -1, -1, 6, -1, -1, 8, -1, -1, 10, -1, -1, 12, 3, -1, -1, 5, -1, -1, 7, -1, -1, 9, -1, -1, 11, -1, -1, 13);
        const U8x32 out14R_B = _mm256_shuffle_epi8(midRB01, shuffle1_RB);
        const U8x32 out14_G_ = _mm256_shuffle_epi8(midG01, shuffle1_G);
        const auto out14 = out14R_B.Or(out14_G_);

        const auto out0 = out02.PermuteLane<0, 2>(out14);
        const auto out1 = out02.PermuteLane<1, 2>(out35);
        const auto out2 = out14.PermuteLane<1, 3>(out35);
        out0.Save(dst + 0);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_256 : RGB5x5ToRGB8_256Base<Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat(src); // 01234567 | 89abcdef

        const auto [midR, midG, midB] = this->RGB5x5ToRGB8Hi(dat);

        const U16x16 valLo = (IsRGB ? midR : midB).template As<U8x32>().ZipEven(midG.template As<U8x32>()).template As<U16x16>();
        U16x16 valHi;
        if constexpr (HasAlpha)
        {
            const auto midA = dat.As<I16x16>().ShiftRightArith<7>().As<U8x32>();
            valHi = (IsRGB ? midB : midR).template As<U8x32>().ZipEven(midA).template As<U16x16>();
        }
        else
        {
            const U16x16 alphaMask(uint16_t(0xff00));
            valHi = (IsRGB ? midB : midR).template ShiftRightLogic<8>().Or(alphaMask);
        }
        const auto out = valLo.Zip(valHi).As<U32x8>();
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_LUT_256 : RGB5x5ToRGB8_256LUTBase<IsRGB, Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x16(src +  0); // 01234567 | 89abcdef
        const auto dat1 = U16x16(src + 16); // ghijklmn | opqrstuv

        const auto [midRB0, midRB1, midG] = this->RGB5x5ToRGB8(dat0, dat1);
        // midG: 0g1h2i3j4k5l6m7n | 8o9paqbrcsdteufv
        U8x32 midGA0, midGA1;
        if constexpr (HasAlpha)
        {
            const auto midA0 = dat0.As<I16x16>().ShiftRightArith<7>().As<U8x32>(),
                midA1 = dat1.As<I16x16>().ShiftRightArith<7>().As<U8x32>();
            midGA0 = midA0.SelectWith<0x55555555>(midG);
            midGA1 = midG.ZipEven(midA1);
        }
        else
        {
            const U16x16 alphaMask(uint16_t(0xff00));
            midGA0 = midG.Or(alphaMask.template As<U8x32>());
            midGA1 = midG.template As<U16x16>().template ShiftRightLogic<8>().Or(alphaMask).template As<U8x32>();
        }
        const auto out0 = midRB0.Zip(midGA0).template As<U32x8>();
        const auto out1 = midRB1.Zip(midGA1).template As<U32x8>();
        out0[0].Save(dst +  0);
        out0[1].Save(dst +  8);
        out1[0].Save(dst + 16);
        out1[1].Save(dst + 24);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_256_FullLUT
{
    static_assert(Is555 || !HasAlpha);
    const int* __restrict LUTPtr = nullptr;
    RGB5x5ToRGBA8_256_FullLUT() noexcept
    {
        static const auto LUT_ = []()
        {
            std::vector<uint32_t> lut(UINT16_MAX + 1, 0u);
            using TGen = RGB5x5ToRGBA8_256<IsRGB, HasAlpha, Is555>;
            TGen gen;
            for (uint32_t i = 0; i <= UINT16_MAX; i += TGen::K)
            {
                std::array<uint16_t, TGen::K> src = { 0 };
                for (uint32_t j = 0; j < TGen::K; ++j)
                    src[j] = static_cast<uint16_t>(i + j);
                gen(&lut[i], src.data());
            }
            return lut;
        }();
        LUTPtr = reinterpret_cast<const int*>(LUT_.data());
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_FullLUT_256 : RGB5x5ToRGBA8_256_FullLUT<IsRGB, HasAlpha, Is555>
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = U16x16(src +  0);
        const auto mid = dat.Cast<U32x8>();
        const U32x8 out0 = _mm256_i32gather_epi32(this->LUTPtr, mid[0], 4);
        const U32x8 out1 = _mm256_i32gather_epi32(this->LUTPtr, mid[1], 4);
        out0.Save(dst + 0);
        out1.Save(dst + 8);
    }
};
template<bool Scale, bool IsRGB>
struct RGB10ToRGBf_256
{
    static constexpr size_t M = 8, N = 24, K = 8;
    F32x8 Scaler;
    RGB10ToRGBf_256(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = U32x8(src).As<I32x8>();

        const I32x8 maskLo10bit(0x3ff);
        const auto dat0 = dat.And(maskLo10bit);
        const auto dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit);
        const auto dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit);

        F32x8 valR = (IsRGB ? dat0 : dat2).Cast<F32x8>();
        F32x8 valG = dat1.Cast<F32x8>();
        F32x8 valB = (IsRGB ? dat2 : dat0).Cast<F32x8>();
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }

        const F32x8 valRRBB = _mm256_shuffle_ps(valR, valB, 0b11001100); // R0 R3 B0 B3 | R4 R7 B4 B7
        const F32x8 valGGRR = _mm256_shuffle_ps(valG, valR, 0b01100001); // G1 G0 R2 R1 | G5 G4 R6 R5
        const F32x8 valBBGG = _mm256_shuffle_ps(valB, valG, 0b10110110); // B2 B1 G3 G2 | B6 B5 G7 G6

        const auto out03 = valRRBB.SelectWith<0b10101010>(valGGRR); // R0 G0 B0 R1 | R4 G4 B4 R5
        const auto out14 = valGGRR.SelectWith<0b10101010>(valBBGG); // G1 B1 R2 G2 | G5 B5 R6 G6
        const auto out25 = valBBGG.SelectWith<0b10101010>(valRRBB); // B2 R3 G3 B3 | B6 R7 G7 B7

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);
        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
    }
};
template<bool Scale, bool IsRGB, bool HasAlpha>
struct RGB10ToRGBAf_256
{
    static constexpr size_t M = 8, N = 32, K = 8;
    F32x8 Scaler;
    RGB10ToRGBAf_256(float mulVal) : Scaler(mulVal) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = U32x8(src).As<I32x8>();

        const I32x8 maskLo10bit(0x3ff);
        const I32x8 dat0 = dat.And(maskLo10bit);
        const I32x8 dat1 = dat.ShiftRightLogic<10>().And(maskLo10bit);
        const I32x8 dat2 = dat.ShiftRightLogic<20>().And(maskLo10bit);

        F32x8 valR = (IsRGB ? dat0 : dat2).Cast<F32x8>(); // R 0123 | 4567
        F32x8 valG = dat1.Cast<F32x8>(); // G 0123 | 4567
        F32x8 valB = (IsRGB ? dat2 : dat0).Cast<F32x8>();
        F32x8 valA;
        if constexpr (Scale)
        {
            valR = valR.Mul(Scaler);
            valG = valG.Mul(Scaler);
            valB = valB.Mul(Scaler);
        }
        if constexpr (HasAlpha)
        {
            const auto alphaLUT = _mm256_setr_ps(0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f);
            valA = _mm256_permutevar_ps(alphaLUT, dat.ShiftRightLogic<30>());
        }
        else
        {
            valA = F32x8(1.0f);
        }

        const auto valRG0145 = valR.ZipLoLane(valG); // R0 G0 R1 G1 | R4 G4 R5 G5
        const auto valRG2367 = valR.ZipHiLane(valG); // R2 G2 R3 G3 | R6 G6 R7 G7
        const auto valBA0145 = valB.ZipLoLane(valA); // B0 A0 B1 A1 | B4 A4 B5 A5
        const auto valBA2367 = valB.ZipHiLane(valA); // B2 A2 B3 A3 | B6 A6 B7 A7

        // integer ymm could have higher throughput on Intel Core
        const auto out02 = valRG0145.As<U64x4>().Zip(valBA0145.As<U64x4>()).As<F32x8>();
        const auto out13 = valRG2367.As<U64x4>().Zip(valBA2367.As<U64x4>()).As<F32x8>();
        
        out02[0].Save(dst);
        out13[0].Save(dst + 8);
        out02[1].Save(dst + 16);
        out13[1].Save(dst + 24);
    }
};

struct RGB8ToYUV8_I16_AVX2_Base
{
    static constexpr size_t M = 96, N = 96, K = 32;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
        alignas(32) static inline constexpr uint8_t ShufRB[] = { R +  0, 255, B +  0, 255, R +  3, 255, B +  3, 255, R +  6, 255, B +  6, 255, R +  9, 255, B +  9, 255, 
            R +  0, 255, B +  0, 255, R +  3, 255, B +  3, 255, R +  6, 255, B +  6, 255, R +  9, 255, B +  9, 255 };
    };
    const __m256i ShufRB;
    __m256i LutYRB, LutYG, LutCbRB, LutCbG, LutCrRB, LutCrG;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I16_AVX2_Base(const I16x8& lut_, RGBOrder<R, G, B>) noexcept :
        ShufRB(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler<R, G, B>::ShufRB)))
    {
        const auto lut = lut_.Shuffle<0, 2, 3, 5, 7, 1, 4, 6>();
        LutYRB  = _mm256_broadcastd_epi32(lut);
        LutYG   = _mm256_broadcastw_epi16(lut.MoveToLo<5>());
        LutCbRB = _mm256_broadcastd_epi32(lut.MoveToLo<2>());
        LutCbG  = _mm256_broadcastw_epi16(lut.MoveToLo<6>());
        LutCrRB = _mm256_broadcastd_epi32(lut.MoveToLo<3>());
        LutCrG  = _mm256_broadcastw_epi16(lut.MoveToLo<7>());
    }
};
struct RGB8ToYUV8_I16_AVX2 : public RGB8ToYUV8_I16_AVX2_Base
{
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler2
    {
        alignas(32) static inline constexpr uint8_t ShufG0[] = { G +  0, 255, G +  3, 255, G +  6, 255, G +  9, 255, G + 12, 255, G + 15, 255, G + 18, 255, G + 21, 255,
            G +  0, 255, G +  3, 255, G +  6, 255, G +  9, 255, G + 12, 255, G + 15, 255, G + 18, 255, G + 21, 255 };
        alignas(32) static inline constexpr uint8_t ShufG1[] = { G + 24, 255, G + 27, 255, G + 30, 255, G + 33, 255, G + 36, 255, G + 39, 255, G + 42, 255, G + 45, 255,
            G + 24, 255, G + 27, 255, G + 30, 255, G + 33, 255, G + 36, 255, G + 39, 255, G + 42, 255, G + 45, 255 };
    };
    const __m256i ShufG0, ShufG1;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I16_AVX2(const I16x8& lut, RGBOrder<R, G, B> order) noexcept : RGB8ToYUV8_I16_AVX2_Base(lut, order),
        ShufG0(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG0))),
        ShufG1(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG1))) 
    { }
    static forceinline auto DP3(const I16x16& rbLo, const I16x16& rbHi, const I16x16& g, const __m256i& lutRB, const __m256i& lutG) noexcept
    {
        const auto ShufMid16 = _mm256_setr_epi8(1, 2, 5, 6, 9, 10, 13, 14, 1, 2, 5, 6, 9, 10, 13, 14, 1, 2, 5, 6, 9, 10, 13, 14, 1, 2, 5, 6, 9, 10, 13, 14);
        const auto midRB0 = _mm256_madd_epi16(rbLo, lutRB);
        const auto midRB1 = _mm256_madd_epi16(rbHi, lutRB);
        const I16x16 midRB = _mm256_blend_epi32(_mm256_shuffle_epi8(midRB0, ShufMid16), _mm256_shuffle_epi8(midRB1, ShufMid16), 0b11001100); // << (14 - 8)=6
        const I16x16 midG  = _mm256_mulhrs_epi16(g, lutG); // << (7 + 14 - 15)=6
        const auto sum = midRB.Add(midG.Add(32)).ShiftRightArith<6>(); // sra to keep negative
        return sum;
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x32 dat0(src), dat1(src + 32), dat2(src + 64);
        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const I16x16 midRB04 = _mm256_shuffle_epi8(dat03, ShufRB);
        const I16x16 midRB15 = _mm256_shuffle_epi8(dat03.MoveLaneToLoWith<12>(dat14), ShufRB);
        const I16x16 midRB26 = _mm256_shuffle_epi8(dat14.MoveLaneToLoWith< 8>(dat25), ShufRB);
        const I16x16 midRB37 = _mm256_shuffle_epi8(dat25.MoveLaneToLo<4>(), ShufRB);
        const U8x32  datG = dat03.SelectWith<0b10010010010010011001001001001001>(dat14).SelectWith<0b01001001001001000100100100100100>(dat25);
        const I16x16 midG02 = _mm256_slli_epi16(_mm256_shuffle_epi8(datG, ShufG0), 7);
        const I16x16 midG13 = _mm256_slli_epi16(_mm256_shuffle_epi8(datG, ShufG1), 7);

#define DP3x4(out, lut) const auto out##02 = DP3(midRB04, midRB15, midG02, lut##RB, lut##G), out##13 = DP3(midRB26, midRB37, midG13, lut##RB, lut##G)
        DP3x4(lumi, LutY);
        DP3x4(cb_, LutCb);
        DP3x4(cr_, LutCr);
#undef DP3x4
        U8x32 outY = _mm256_packus_epi16(lumi02, lumi13);
        if (needAddY)
            outY = outY.Add(16);
        const U8x32 cb = _mm256_packs_epi16(cb_02, cb_13), cr = _mm256_packs_epi16(cr_02, cr_13);
        const auto outCb = cb.Add(128), outCr = cr.Add(128);

        Combine8x3_256::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I16DP2A_AVX2 : public RGB8ToYUV8_I16_AVX2_Base
{
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler2
    {
#define G4(x) G+x, 255, 255, 255, G+x+3, 255, 255, 255, G+x+6, 255, 255, 255, G+x+9, 255, 255, 255
        alignas(32) static inline constexpr uint8_t ShufG[] = { G4(0), G4(0), G4(12), G4(12), G4(24), G4(24), G4(36), G4(36) };
#undef G4
    };
    const __m256i ShufG0, ShufG1, ShufG2, ShufG3;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I16DP2A_AVX2(const I16x8& lut, RGBOrder<R, G, B> order) noexcept : RGB8ToYUV8_I16_AVX2_Base(lut, order),
        ShufG0(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG + 0))),
        ShufG1(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG + 32))),
        ShufG2(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG + 64))),
        ShufG3(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler2<R, G, B>::ShufG + 96)))
    { }
    static forceinline auto DP3(const I16x16& rbLo, const I16x16& rbHi, const I16x16& gLo, const I16x16& gHi, const __m256i& lutRB, const __m256i& lutG, const uint8_t base) noexcept
    {
        const auto base_ = (base << 14) + (1 << 13); // pre-add rounding
        const auto baseVal = _mm256_set1_epi32(base_);
        const auto midG0 = _mm256_dpwssd_avx_epi32(baseVal, gLo, lutG);
        const auto midG1 = _mm256_dpwssd_avx_epi32(baseVal, gHi, lutG);
        const auto mid0  = _mm256_dpwssd_avx_epi32(midG0,  rbLo, lutRB);
        const auto mid1  = _mm256_dpwssd_avx_epi32(midG1,  rbHi, lutRB);
        return _mm256_packus_epi32(_mm256_srai_epi32(mid0, 14), _mm256_srai_epi32(mid1, 14));
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x32 dat0(src), dat1(src + 32), dat2(src + 64);
        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        const I16x16 midRB04 = _mm256_shuffle_epi8(dat03, ShufRB);
        const I16x16 midRB15 = _mm256_shuffle_epi8(dat03.MoveLaneToLoWith<12>(dat14), ShufRB);
        const I16x16 midRB26 = _mm256_shuffle_epi8(dat14.MoveLaneToLoWith< 8>(dat25), ShufRB);
        const I16x16 midRB37 = _mm256_shuffle_epi8(dat25.MoveLaneToLo<4>(), ShufRB);
        const U8x32  datG = dat03.SelectWith<0b10010010010010011001001001001001>(dat14).SelectWith<0b01001001001001000100100100100100>(dat25);
        const I16x16 midG04 = _mm256_shuffle_epi8(datG, ShufG0);
        const I16x16 midG15 = _mm256_shuffle_epi8(datG, ShufG1);
        const I16x16 midG26 = _mm256_shuffle_epi8(datG, ShufG2);
        const I16x16 midG37 = _mm256_shuffle_epi8(datG, ShufG3);

#define DP3x4(out, lut, base) const auto out##02 = DP3(midRB04, midRB15, midG04, midG15, lut##RB, lut##G, base), out##13 = DP3(midRB26, midRB37, midG26, midG37, lut##RB, lut##G, base)
        DP3x4(lumi, LutY, needAddY ? 16 : 0);
        DP3x4(cb, LutCb, 128);
        DP3x4(cr, LutCr, 128);
#undef DP3x4
        const U8x32 outY = _mm256_packus_epi16(lumi02, lumi13);
        const U8x32 outCb = _mm256_packus_epi16(cb02, cb13), outCr = _mm256_packus_epi16(cr02, cr13);
        Combine8x3_256::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I8_AVX2_Base
{
    static constexpr size_t M = 96, N = 96, K = 32;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
#define PUT4(a,b,c,d,i) a + i, b + i, c + i, d + i 
#define SHUF32(a,b,c,d)alignas(32) static inline const uint8_t Shuf##a##b##c##d[] = { PUT4(a,b,c,d,0), PUT4(a,b,c,d,3), PUT4(a,b,c,d,6), PUT4(a,b,c,d,9), PUT4(a,b,c,d,0), PUT4(a,b,c,d,3), PUT4(a,b,c,d,6), PUT4(a,b,c,d,9) }
        SHUF32(R, G, G, B);
        SHUF32(R, G, R, B);
        SHUF32(R, B, G, B);
#undef SHUF32
#undef PUT4
    };
    __m256i ShufRGGB, ShufRGRB, ShufRBGB;
    __m256i LutY, LutCb, LutCr;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I8_AVX2_Base(const int8_t* lut, RGBOrder<R, G, B>) noexcept :
        ShufRGGB(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler<R, G, B>::ShufRGGB))),
        ShufRGRB(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler<R, G, B>::ShufRGRB))),
        ShufRBGB(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Shuffler<R, G, B>::ShufRBGB))),
        LutY (_mm256_set1_epi32(reinterpret_cast<const int32_t*>(lut)[0])),
        LutCb(_mm256_set1_epi32(reinterpret_cast<const int32_t*>(lut)[1])),
        LutCr(_mm256_set1_epi32(reinterpret_cast<const int32_t*>(lut)[2]))
    { }
};
struct RGB8ToYUV8_I8_AVX2 : public RGB8ToYUV8_I8_AVX2_Base
{
    using RGB8ToYUV8_I8_AVX2_Base::RGB8ToYUV8_I8_AVX2_Base;
    template<bool IsY>
    static forceinline auto DP4(const U8x32& rgb04, const U8x32& rgb15, const __m256i& lut) noexcept
    {
        const auto mid04 = _mm256_maddubs_epi16(rgb04, lut), mid15 = _mm256_maddubs_epi16(rgb15, lut);
        const I16x16 sum0145 = _mm256_hadd_epi16(mid04, mid15);
        const __m256i add128 = _mm256_set1_epi16(128);
        if constexpr (IsY)
            return sum0145.As<U16x16>().Add(add128).ShiftRightLogic<8>();
        else
            return sum0145.SatAdd(add128).ShiftRightArith<8>();
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x32 dat0(src), dat1(src + 32), dat2(src + 64);
        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1
        const U8x32 part04 = dat03, part15 = dat03.MoveLaneToLoWith<12>(dat14), part26 = dat14.MoveLaneToLoWith< 8>(dat25), part37 = dat25.MoveLaneToLo<4>();

#define DP4x4(out, isy, sm, lut) const auto out##02 = DP4<isy>(_mm256_shuffle_epi8(part04, Shuf##sm), _mm256_shuffle_epi8(part15, Shuf##sm), lut), out##13 = DP4<isy>(_mm256_shuffle_epi8(part26, Shuf##sm), _mm256_shuffle_epi8(part37, Shuf##sm), lut)
        DP4x4(lumi, true, RGGB, LutY);
        DP4x4(cb_, false, RBGB, LutCb);
        DP4x4(cr_, false, RGRB, LutCr);
#undef DP4x4
        U8x32 outY = _mm256_packus_epi16(lumi02, lumi13);
        if (needAddY)
            outY = outY.Add(16);
        const U8x32 cb = _mm256_packs_epi16(cb_02, cb_13), cr = _mm256_packs_epi16(cr_02, cr_13);
        const auto outCb = cb.Add(128), outCr = cr.Add(128);

        Combine8x3_256::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I8DP4A_AVX2 : public RGB8ToYUV8_I8_AVX2_Base
{
    using RGB8ToYUV8_I8_AVX2_Base::RGB8ToYUV8_I8_AVX2_Base;
    static forceinline auto DP4(const U8x32& rgb04, const U8x32& rgb15, const __m256i& lut, const int32_t base) noexcept
    {
        const auto adder = _mm256_set1_epi32((base << 8) + 128); // pre-add round
        const auto mid04 = _mm256_dpbusd_avx_epi32(adder, rgb04, lut), mid15 = _mm256_dpbusd_avx_epi32(adder, rgb15, lut);
        return _mm256_srli_epi16(_mm256_packus_epi32(mid04, mid15), 8);
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const U8x32 dat0(src), dat1(src + 32), dat2(src + 64);
        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1
        const U8x32 part04 = dat03, part15 = dat03.MoveLaneToLoWith<12>(dat14), part26 = dat14.MoveLaneToLoWith< 8>(dat25), part37 = dat25.MoveLaneToLo<4>();

#define DP4x4(out, sm, lut, base) const auto out##02 = DP4(_mm256_shuffle_epi8(part04, Shuf##sm), _mm256_shuffle_epi8(part15, Shuf##sm), lut, base), out##13 = DP4(_mm256_shuffle_epi8(part26, Shuf##sm), _mm256_shuffle_epi8(part37, Shuf##sm), lut, base)
        DP4x4(lumi, RGGB, LutY, needAddY ? 16 : 0);
        DP4x4(cb,   RBGB, LutCb, 128);
        DP4x4(cr,   RGRB, LutCr, 128);
#undef DP4x4
        const U8x32 outY  = _mm256_packus_epi16(lumi02, lumi13);
        const U8x32 outCb = _mm256_packus_epi16(  cb02,   cb13);
        const U8x32 outCr = _mm256_packus_epi16(  cr02,   cr13);

        Combine8x3_256::DoStore(dst, outY, outCb, outCr);
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, AVX2_I16)
{
    if (count >= RGB8ToYUV8_I16_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16_AVX2 proc(I16x8(RGB8ToYCC8LUTI16[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<SSE41_I16_2>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, AVX2VNNI_I16)
{
    if (count >= RGB8ToYUV8_I16DP2A_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16DP2A_AVX2 proc(I16x8(RGB8ToYCC8LUTI16[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<SSE41_I16_2>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, AVX2_I8)
{
    if (count >= RGB8ToYUV8_I8_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8_AVX2 proc(RGB8ToYCC8LUTI8x4[mval].data(), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<SSE41_I8>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, AVX2VNNI_I8)
{
    if (count >= RGB8ToYUV8_I8DP4A_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8DP4A_AVX2 proc(RGB8ToYCC8LUTI8x4[mval].data(), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<SSE41_I8>(dest, src, count, mval);
}

template<typename T>
struct YUV8ToRGB8_I16_AVX2_Base
{
    static constexpr size_t M = 96, N = 96, K = 32;
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY, bool isRGBFull, bool isRGB) const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        const U8x32 dat0(src), dat1(src + 32), dat2(src + 64);
        const auto dat03 = dat0.PermuteLane<0, 3>(dat1); // RGBRGBRGBRGBRGBR
        const auto dat14 = dat0.PermuteLane<1, 2>(dat2); // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat25 = dat1.PermuteLane<0, 3>(dat2); // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        // 0b61c72d83e94fa5
        U8x32 datY  = dat03.SelectWith<0b01001001001001000100100100100100>(dat14).SelectWith<0b00100100100100100010010010010010>(dat25);
        U8x32 datCb = dat03.SelectWith<0b10010010010010011001001001001001>(dat14).SelectWith<0b01001001001001000100100100100100>(dat25);
        U8x32 datCr = dat03.SelectWith<0b00100100100100100010010010010010>(dat14).SelectWith<0b10010010010010011001001001001001>(dat25);
        if (needAddY)
            datY = datY.Sub(16);
        datCb = datCb.Sub(128), datCr = datCr.Sub(128);
#define Shuf82(n,x,a) const auto n = _mm256_shuffle_epi8(dat##x, _mm256_setr_epi8(a, -1, a+3, -1, a+6, -1, a+9, -1, a+12, -1, a+15, -1, a+18, -1, a+21, -1, a, -1, a+3, -1, a+6, -1, a+9, -1, a+12, -1, a+15, -1, a+18, -1, a+21, -1))
        Shuf82(y_0, Y,  0);
        Shuf82(y_1, Y, 24);
#undef  Shuf82
        // (7 + 14) - 15 = 6
        const I16x16 y0 = _mm256_mulhrs_epi16(_mm256_slli_epi16(y_0, 7), self.MulY);
        const I16x16 y1 = _mm256_mulhrs_epi16(_mm256_slli_epi16(y_1, 7), self.MulY);
#define Shuf82(n,x,a) const auto n = _mm256_shuffle_epi8(dat##x, _mm256_setr_epi8(-1, a, -1, a+6, -1, a+12, -1, a+18, -1, a+24, -1, a+30, -1, a+36, -1, a+42, -1, a, -1, a+6, -1, a+12, -1, a+18, -1, a+24, -1, a+30, -1, a+36, -1, a+42))
        Shuf82(cb_e, Cb, 1);
        Shuf82(cb_o, Cb, 4);
        Shuf82(cr_e, Cr, 2);
        Shuf82(cr_o, Cr, 5);
#undef  Shuf82
        const auto cbr0e = _mm256_unpacklo_epi16(cb_e, cr_e); // 0246
        const auto cbr0o = _mm256_unpacklo_epi16(cb_o, cr_o); // 1357
        const auto cbr1e = _mm256_unpackhi_epi16(cb_e, cr_e); // 8ace
        const auto cbr1o = _mm256_unpackhi_epi16(cb_o, cr_o); // 9bdf

#define CalcRGB(x) const auto [r##x, g##x, b##x] = self.Calc(y##x, cbr##x##e, cbr##x##o)
        CalcRGB(0);
        CalcRGB(1);
#undef  CalcRGB

#define Combine4(x) auto x = _mm256_packus_epi16(x##0, x##1)
        Combine4(r);
        Combine4(g);
        Combine4(b);
#undef  Combine4
        if (!isRGBFull)
        {
            const auto rgbMin = _mm256_set1_epi8(16), rgbMax = _mm256_set1_epi8(static_cast<char>(235));
#define ClampRGB(x) x = _mm256_min_epu8(_mm256_max_epu8(x, rgbMin), rgbMax)
            ClampRGB(r);
            ClampRGB(g);
            ClampRGB(b);
#undef  ClampRGB
        }
        if (isRGB)
            Combine8x3_256::DoStore(dst, r, g, b);
        else
            Combine8x3_256::DoStore(dst, b, g, r);
    }
};
struct YUV8ToRGB8_I16_AVX2 : public YUV8ToRGB8_I16_AVX2_Base<YUV8ToRGB8_I16_AVX2>
{
    I16x16 MulY, MulR, MulB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_AVX2(const uint32_t* lut) noexcept : MulY(static_cast<int16_t>(lut[0] >> 1)),
        MulR(U32x8(lut[1] & 0xffff0000u).As<I16x16>()),
        MulB(U32x8(lut[1] & 0x0000ffffu).As<I16x16>()),
        MulG(U32x8(lut[2]).As<I16x16>())
    { }
    forceinline std::tuple<__m256i, __m256i, __m256i> Calc(const I16x16& y5, const __m256i& cbrE, const __m256i& cbrO) const noexcept
    {
        const I16x16 rE = _mm256_madd_epi16(cbrE, MulR), rO = _mm256_madd_epi16(cbrO, MulR);
        const I16x16 gE = _mm256_madd_epi16(cbrE, MulG), gO = _mm256_madd_epi16(cbrO, MulG);
        const I16x16 bE = _mm256_madd_epi16(cbrE, MulB), bO = _mm256_madd_epi16(cbrO, MulB);
        // (8 + 13) - 16 = 5
        const auto r5 = rE.ZipEven(rO), g5 = gE.ZipEven(gO), b5 = bE.ZipEven(bO);
        const auto y = y5.Add(16); // add rounding
        const auto r = r5.Add(y).ShiftRightArith<5>(), b = b5.Add(y).ShiftRightArith<5>();
        const auto g = y.Sub(g5).ShiftRightArith<5>();
        return { r, g, b };
    }
};
struct YUV8ToRGB8_I16_2_AVX2 : public YUV8ToRGB8_I16_AVX2_Base<YUV8ToRGB8_I16_2_AVX2>
{
    I16x16 MulY, MulRB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_2_AVX2(const uint32_t* lut) noexcept : MulY(static_cast<int16_t>(lut[0])),
        MulRB(U32x8(lut[1]).As<I16x16>()),
        MulG(U32x8(lut[2]).As<I16x16>())
    { }
    forceinline std::tuple<__m256i, __m256i, __m256i> Calc(const I16x16& y6, const __m256i& cbrE, const __m256i& cbrO) const noexcept
    {
        // (8 + 13) - 16 = 5
        const I16x16 gE = _mm256_madd_epi16(cbrE, MulG), gO = _mm256_madd_epi16(cbrO, MulG);
        // (8 + 13) - 15 = 6
        const I16x16 rbE = _mm256_mulhrs_epi16(cbrE, MulRB), rbO = _mm256_mulhrs_epi16(cbrO, MulRB);
        const auto r6 = rbE.ZipEven(rbO), b6 = rbE.ZipOdd(rbO);
        const auto g5 = gE.ZipEven(gO);
        const auto yrb = y6.Add(32); // add rounding
        const auto r = r6.Add(yrb).ShiftRightArith<6>(), b = b6.Add(yrb).ShiftRightArith<6>();
        const auto yg = yrb.ShiftRightArith<1>();
        const auto g = yg.Sub(g5).ShiftRightArith<5>();
        return { r, g, b };
    }
};
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX2_I16)
{
    if (count >= YUV8ToRGB8_I16_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_AVX2 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
} 
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX2_I16_2)
{
    if (count >= YUV8ToRGB8_I16_2_AVX2::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_2_AVX2 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<LOOP_F32>(dest, src, count, mval);
}

DEFINE_FASTPATH_METHOD(G8ToGA8, AVX2)
{
    ProcessLOOP4<G2GA8_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX22)
{
    ProcessLOOP4<G2GA8_256_2, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX2)
{
    ProcessLOOP4<G2RGB8_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX2)
{
    ProcessLOOP4<G2RGBA8_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX22)
{
    ProcessLOOP4<G2RGBA8_256_2, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, AVX2)
{
    ProcessLOOP4<GA2RGB8_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX2)
{
    ProcessLOOP4<GA2RGBA8_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToGA16, AVX2)
{
    ProcessLOOP4<G2GA16_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G16ToRGB16, AVX2)
{
    ProcessLOOP4<G2RGB16_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToRGBA16, AVX2)
{
    ProcessLOOP4<G2RGBA16_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA16ToRGB16, AVX2)
{
    ProcessLOOP4<GA2RGB16_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA16ToRGBA16, AVX2)
{
    ProcessLOOP4<GA2RGBA16_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, AVX)
{
    ProcessLOOP4<G2GAf_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, AVX)
{
    ProcessLOOP4<G2RGBf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, AVX)
{
    ProcessLOOP4<G2RGBAf_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, AVX)
{
    ProcessLOOP4<GA2RGBf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, AVX)
{
    ProcessLOOP4<GA2RGBAf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB8_3To4_256<0, 1, 2>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB8_3To4_256<2, 1, 0>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX2)
{
    ProcessLOOP4<RGB8_4To3_256<0, 1, 2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, AVX2)
{
    ProcessLOOP4<RGB8_4To3_256<2, 1, 0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToRGBA16, AVX2)
{
    ProcessLOOP4<RGB16_3To4_256<true>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR16ToRGBA16, AVX2)
{
    ProcessLOOP4<RGB16_3To4_256<false>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA16ToRGB16, AVX2)
{
    ProcessLOOP4<RGB16_4To3_256<true>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGR16, AVX2)
{
    ProcessLOOP4<RGB16_4To3_256<false>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, AVX)
{
    ProcessLOOP4<RGBf_3To4_256<true>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, AVX)
{
    ProcessLOOP4<RGBf_3To4_256<false>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, AVX2)
{
    ProcessLOOP4<RGBf_4To3_256<true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, AVX2)
{
    ProcessLOOP4<RGBf_4To3_256<false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX2)
{
    ProcessLOOP4<RGBToBGR8_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX22)
{
    ProcessLOOP4<RGBToBGR8_256_2, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, AVX2)
{
    ProcessLOOP4<RGBAToBGRA8_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToBGR16, AVX2)
{
    ProcessLOOP4<RGBToBGR16_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGRA16, AVX2)
{
    ProcessLOOP4<RGBAToBGRA16_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, AVX)
{
    ProcessLOOP4<RGBToBGRf_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, AVX)
{
    ProcessLOOP4<RGBAToBGRAf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, AVX2)
{
    ProcessLOOP4<KeepChannel8_3_1_256<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, AVX2)
{
    ProcessLOOP4<KeepChannel8_3_1_256<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, AVX2)
{
    ProcessLOOP4<KeepChannel8_3_1_256<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToR16, AVX2)
{
    ProcessLOOP4<KeepChannel16_3_1_256<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToG16, AVX2)
{
    ProcessLOOP4<KeepChannel16_3_1_256<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToB16, AVX2)
{
    ProcessLOOP4<KeepChannel16_3_1_256<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, AVX2)
{
    ProcessLOOP4<KeepChannelF_4_1_256<0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, AVX2)
{
    ProcessLOOP4<KeepChannelF_4_1_256<1>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, AVX2)
{
    ProcessLOOP4<KeepChannelF_4_1_256<2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, AVX2)
{
    ProcessLOOP4<KeepChannelF_4_1_256<3>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, AVX2)
{
    ProcessLOOP4<KeepChannelF_3_1_256<0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, AVX2)
{
    ProcessLOOP4<KeepChannelF_3_1_256<1>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, AVX2)
{
    ProcessLOOP4<KeepChannelF_3_1_256<2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX2)
{
    ExtractLOOP4<2, Extract8x2_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, AVX2)
{
    ExtractLOOP4<3, Extract8x3_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX2)
{
    ExtractLOOP4<4, Extract8x4_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x2, AVX2)
{
    ExtractLOOP4<2, Extract16x2_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x3, AVX2)
{
    ExtractLOOP4<3, Extract16x3_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x4, AVX2)
{
    ExtractLOOP4<4, Extract16x4_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x2, AVX2)
{
    ExtractLOOP4<2, Extract32x2_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x3, AVX2)
{
    ExtractLOOP4<3, Extract32x3_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x4, AVX2)
{
    ExtractLOOP4<4, Extract32x4_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x2, AVX2)
{
    CombineLOOP4<2, Combine8x2_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, AVX2)
{
    CombineLOOP4<3, Combine8x3_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, AVX2)
{
    CombineLOOP4<4, Combine8x4_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x2, AVX2)
{
    CombineLOOP4<2, Combine16x2_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x3, AVX2)
{
    CombineLOOP4<3, Combine16x3_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x4, AVX2)
{
    CombineLOOP4<4, Combine16x4_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, AVX2)
{
    CombineLOOP4<2, Combine32x2_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, AVX2)
{
    CombineLOOP4<3, Combine32x3_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, AVX2)
{
    CombineLOOP4<4, Combine32x4_256, &Func<SIMD128>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(G8ToG16, AVX2)
{
    ProcessLOOP4<G8ToG16_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToG8, AVX2)
{
    ProcessLOOP4<G16ToG8_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<true, true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<false, true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<true, false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<false, false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_256<true, true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_256<false, true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_256<true, false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_256<false, false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<true, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<false, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<true, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<false, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<true, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_256<false, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<true, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<false, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<true, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<false, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<true, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_256<false, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<true, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<false, false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<true, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<false, true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<true, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, AVX23)
{
    ProcessLOOP4<RGB5x5ToRGBA8_FullLUT_256<false, false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, AVX)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, true, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, AVX)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_SSE41<false, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_SSE41<true, false, true>, &Func<LOOP>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_256<false, true>, &Func<SSE41>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_256<true, true>, &Func<SSE41>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_256<false, false>, &Func<SSE41>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_256<true, false>, &Func<SSE41>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBAf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_256<false, true, false>, &Func<SSE41>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_256<true, true, false>, &Func<SSE41>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBAf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_256<false, false, false>, &Func<SSE41>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_256<true, false, false>, &Func<SSE41>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_256<false, true, true>, &Func<AVX>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_256<true, true, true>, &Func<AVX>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, AVX2)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_256<false, false, true>, &Func<AVX>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_256<true, false, true>, &Func<AVX>>(dest, src, count, mulVal);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320
#define mm512_set_lane(n, ...) _mm512_set_##n(__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__)
struct G2GA8_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2GA8_512(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src); // 0~3f
        const auto outLoLane = _mm512_unpacklo_epi8(dat, Alpha); // [00~07][10~17][20~27][30~37] 
        const auto outHiLane = _mm512_unpackhi_epi8(dat, Alpha); // [08~0f][18~1f][28~2f][38~3f]
        const auto mix0 = _mm512_shuffle_i64x2(outLoLane, outHiLane, 0b10001000); // [00~07][20~27][08~0f][28~2f]
        const auto mix1 = _mm512_shuffle_i64x2(outLoLane, outHiLane, 0b11011101); // [10~17][30~37][18~1f][38~3f]
        const auto mix2 = _mm512_shuffle_i64x2(mix0, mix1, 0b10001000); // [00~07][08~0f][10~17][18~1f]
        const auto mix3 = _mm512_shuffle_i64x2(mix0, mix1, 0b11011101); // [20~27][28~2f][30~37][38~3f]
        _mm512_storeu_si512(dst, mix2);
        _mm512_storeu_si512(dst + 32, mix3);
    }
};
struct G2RGB8_512
{
    static constexpr size_t M = 64, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src); // 0~3f
        const auto shuffleMask0 = _mm512_set_epi8( 5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0,  5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0,  5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0,  5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0);
        const auto shuffleMask1 = _mm512_set_epi8(10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5, 10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5, 10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5, 10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5);
        const auto shuffleMask2 = _mm512_set_epi8(15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10);
        const auto mid0 = _mm512_shuffle_epi8(dat, shuffleMask0);
        const auto mid1 = _mm512_shuffle_epi8(dat, shuffleMask1);
        const auto mid2 = _mm512_shuffle_epi8(dat, shuffleMask2);
        const auto mix0 = _mm512_shuffle_i64x2(mid0, mid1, 0b10001000); // [00,20,01,21]
        const auto mix1 = _mm512_shuffle_i64x2(mid0, mid1, 0b11011101); // [10,30,11,31]
        const auto mix2 = _mm512_shuffle_i64x2(mid2, mid2, 0b11000110); // [22,12,02,32]
        const auto out0 = _mm512_mask_shuffle_i64x2(mix2, 0b11001111, mix0, mix1, 0b00001000); // [00,01,02,10]
        const auto out1 = _mm512_mask_shuffle_i64x2(mix2, 0b11110011, mix1, mix0, 0b11010010); // [11,12,20,21]
        const auto out2 = _mm512_mask_shuffle_i64x2(mix2, 0b00111100, mix1, mix1, 0b00110100); // [22,30,31,32]
        _mm512_storeu_si512(dst +   0, out0);
        _mm512_storeu_si512(dst +  64, out1);
        _mm512_storeu_si512(dst + 128, out2);
    }
};
struct G2RGBA8_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2RGBA8_512(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src); // 0~3f
        const auto rgLo = _mm512_unpacklo_epi8(dat, dat); // [*0~*7]
        const auto rgHi = _mm512_unpackhi_epi8(dat, dat); // [*8~*f]
        const auto baLo = _mm512_unpacklo_epi8(dat, Alpha); // [*0~*7]
        const auto baHi = _mm512_unpackhi_epi8(dat, Alpha); // [*8~*f]
        const auto rgba0123 = _mm512_unpacklo_epi16(rgLo, baLo); // [*0~*3]
        const auto rgba4567 = _mm512_unpackhi_epi16(rgLo, baLo); // [*4~*7]
        const auto rgba89ab = _mm512_unpacklo_epi16(rgHi, baHi); // [*8~*b]
        const auto rgbacdef = _mm512_unpackhi_epi16(rgHi, baHi); // [*c~*f]
        const auto mix0 = _mm512_shuffle_i64x2(rgba0123, rgba4567, 0b10001000); // [00~03][20~23][04~07][24~27]
        const auto mix1 = _mm512_shuffle_i64x2(rgba0123, rgba4567, 0b11011101); // [10~13][30~33][14~17][34~37]
        const auto mix2 = _mm512_shuffle_i64x2(rgba89ab, rgbacdef, 0b10001000); // [08~0b][28~2b][0c~0f][2c~2f]
        const auto mix3 = _mm512_shuffle_i64x2(rgba89ab, rgbacdef, 0b11011101); // [18~1b][38~3b][1c~1f][3c~3f]
        const auto out0 = _mm512_shuffle_i64x2(mix0, mix2, 0b10001000); // [00~03][04~07][08~0b][0c~0f]
        const auto out1 = _mm512_shuffle_i64x2(mix1, mix3, 0b10001000); // [10~13][14~17][18~1b][1c~1f]
        const auto out2 = _mm512_shuffle_i64x2(mix0, mix2, 0b11011101); // [20~23][24~27][28~2b][2c~2f]
        const auto out3 = _mm512_shuffle_i64x2(mix1, mix3, 0b11011101); // [30~33][34~37][38~3b][3c~3f]
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct GA2RGBA8_512
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~1f
        const auto shuffleMaskLo = mm512_set_lane(epi8, 7, 6, 6, 6, 5, 4, 4, 4, 3, 2, 2, 2, 1, 0, 0, 0);
        const auto shuffleMaskHi = mm512_set_lane(epi8, 15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10, 9, 8, 8, 8);
        const auto midLo = _mm512_shuffle_epi8(dat, shuffleMaskLo); // [00~03][08~0b][10~13][18~1b]
        const auto midHi = _mm512_shuffle_epi8(dat, shuffleMaskHi); // [04~07][0c~0f][14~17][1c~1f]
        const auto permuteMask0 = _mm512_set_epi64(11, 10, 3, 2,  9,  8, 1, 0);
        const auto permuteMask1 = _mm512_set_epi64(15, 14, 7, 6, 13, 12, 5, 4);
        const auto out0 = _mm512_permutex2var_epi64(midLo, permuteMask0, midHi); // [00~03][04~07][08~0b][0c~0f]
        const auto out1 = _mm512_permutex2var_epi64(midLo, permuteMask1, midHi); // [10~13][14~17][18~1b][1c~1f]
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct G2GA16_512
{
    static constexpr size_t M = 32, N = 64, K = 32;
    __m512i Alpha;
    forceinline G2GA16_512(uint16_t alpha) noexcept : Alpha(_mm512_set1_epi16(static_cast<short>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~1f

        const auto shuffleMask0 = _mm512_set_epi32(
            0x000f000f, 0x000e000e, 0x000d000d, 0x000c000c, 
            0x000b000b, 0x000a000a, 0x00090009, 0x00080008, 
            0x00070007, 0x00060006, 0x00050005, 0x00040004, 
            0x00030003, 0x00020002, 0x00010001, 0x00000000);
        const auto out0 = _mm512_mask_permutexvar_epi16(Alpha, 0x55555555, shuffleMask0, dat);

        const auto shuffleMask1 = _mm512_set_epi32(
            0x001f001f, 0x001e001e, 0x001d001d, 0x001c001c,
            0x001b001b, 0x001a001a, 0x00190019, 0x00180018,
            0x00170017, 0x00160016, 0x00150015, 0x00140014,
            0x00130013, 0x00120012, 0x00110011, 0x00100010);
        const auto out1 = _mm512_mask_permutexvar_epi16(Alpha, 0x55555555, shuffleMask1, dat);

        _mm512_storeu_si512(dst, out0);
        _mm512_storeu_si512(dst + 32, out1);
    }
};
struct G2RGB16_512
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~1f

        const auto shuffleMask0 = _mm512_set_epi64(0x000a000a00090009, 0x0009000800080008, 0x0007000700070006, 0x0006000600050005, 0x0005000400040004, 0x0003000300030002, 0x0002000200010001, 0x0001000000000000);
        const auto out0 = _mm512_permutexvar_epi16(shuffleMask0, dat);
        const auto shuffleMask1 = _mm512_set_epi64(0x0015001400140014, 0x0013001300130012, 0x0012001200110011, 0x0011001000100010, 0x000f000f000f000e, 0x000e000e000d000d, 0x000d000c000c000c, 0x000b000b000b000a);
        const auto out1 = _mm512_permutexvar_epi16(shuffleMask1, dat);
        const auto shuffleMask2 = _mm512_set_epi64(0x001f001f001f001e, 0x001e001e001d001d, 0x001d001c001c001c, 0x001b001b001b001a, 0x001a001a00190019, 0x0019001800180018, 0x0017001700170016, 0x0016001600150015);
        const auto out2 = _mm512_permutexvar_epi16(shuffleMask2, dat);

        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 32, out1);
        _mm512_storeu_si512(dst + 64, out2);
    }
};
struct G2RGBA16_512
{
    static constexpr size_t M = 32, N = 128, K = 32;
    __m512i Alpha;
    forceinline G2RGBA16_512(uint16_t alpha) noexcept : Alpha(_mm512_set1_epi16(static_cast<short>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~1f

        const auto shuffleMask0 = _mm512_set_epi64(0x0007000700070007, 0x0006000600060006, 0x0005000500050005, 0x0004000400040004, 0x0003000300030003, 0x0002000200020002, 0x0001000100010001, 0x0000000000000000);
        const auto out0 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, shuffleMask0, dat);
        const auto shuffleMask1 = _mm512_set_epi64(0x000f000f000f000f, 0x000e000e000e000e, 0x000d000d000d000d, 0x000c000c000c000c, 0x000b000b000b000b, 0x000a000a000a000a, 0x0009000900090009, 0x0008000800080008);
        const auto out1 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, shuffleMask1, dat);
        const auto shuffleMask2 = _mm512_set_epi64(0x0017001700170017, 0x0016001600160016, 0x0015001500150015, 0x0014001400140014, 0x0013001300130013, 0x0012001200120012, 0x0011001100110011, 0x0010001000100010);
        const auto out2 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, shuffleMask2, dat);
        const auto shuffleMask3 = _mm512_set_epi64(0x001f001f001f001f, 0x001e001e001e001e, 0x001d001d001d001d, 0x001c001c001c001c, 0x001b001b001b001b, 0x001a001a001a001a, 0x0019001900190019, 0x0018001800180018);
        const auto out3 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, shuffleMask3, dat);

        _mm512_storeu_si512(dst, out0);
        _mm512_storeu_si512(dst + 32, out1);
        _mm512_storeu_si512(dst + 64, out2);
        _mm512_storeu_si512(dst + 96, out3);
    }
};
struct GA2RGB16_512
{
    static constexpr size_t M = 64, N = 96, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0); // 0~f
        const auto dat1 = _mm512_loadu_epi16(src + 32); // 10~1f

        const auto shuffleMask0 = _mm512_set_epi64(0x0014001400120012, 0x0012001000100010, 0x000e000e000e000c, 0x000c000c000a000a, 0x000a000800080008, 0x0006000600060004, 0x0004000400020002, 0x0002000000000000);
        const auto out0 = _mm512_permutexvar_epi16(shuffleMask0, dat0);
        const auto shuffleMask1 = _mm512_set_epi64(0x002a002800280028, 0x0026002600260024, 0x0024002400220022, 0x0022002000200020, 0x001e001e001e001c, 0x001c001c001a001a, 0x001a001800180018, 0x0016001600160014);
        const auto out1 = _mm512_permutex2var_epi16(dat0, shuffleMask1, dat1);
        const auto shuffleMask2 = _mm512_set_epi64(0x001e001e001e001c, 0x001c001c001a001a, 0x001a001800180018, 0x0016001600160014, 0x0014001400120012, 0x0012001000100010, 0x000e000e000e000c, 0x000c000c000a000a);
        const auto out2 = _mm512_permutexvar_epi16(shuffleMask2, dat1);

        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 32, out1);
        _mm512_storeu_si512(dst + 64, out2);
    }
};
struct GA2RGBA16_512
{
    static constexpr size_t M = 32, N = 64, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~f
        const auto shuffleMask0 = _mm512_set_epi64(0x000f000e000e000e, 0x000d000c000c000c, 0x000b000a000a000a, 0x0009000800080008, 0x0007000600060006, 0x0005000400040004, 0x0003000200020002, 0x0001000000000000);
        const auto out0 = _mm512_permutexvar_epi16(shuffleMask0, dat);
        const auto shuffleMask1 = _mm512_set_epi64(0x001f001e001e001e, 0x001d001c001c001c, 0x001b001a001a001a, 0x0019001800180018, 0x0017001600160016, 0x0015001400140014, 0x0013001200120012, 0x0011001000100010);
        const auto out1 = _mm512_permutexvar_epi16(shuffleMask1, dat);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 32, out1);
    }
};
struct G2GAf_512
{
    static constexpr size_t M = 16, N = 32, K = 16;
    __m512 Alpha;
    forceinline G2GAf_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_ps(src);
        const auto out0 = _mm512_mask_permutexvar_ps(Alpha, 0x5555, _mm512_set_epi32( 7,  7,  6,  6,  5,  5,  4,  4,  3,  3,  2,  2,  1,  1,  0,  0), dat);
        const auto out1 = _mm512_mask_permutexvar_ps(Alpha, 0x5555, _mm512_set_epi32(15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10,  9,  9,  8,  8), dat);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
    }
};
struct G2RGBf_512
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_ps(src);
        const auto out0 = _mm512_permutexvar_ps(_mm512_set_epi32( 5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0), dat);
        const auto out1 = _mm512_permutexvar_ps(_mm512_set_epi32(10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5), dat);
        const auto out2 = _mm512_permutexvar_ps(_mm512_set_epi32(15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10), dat);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct G2RGBAf_512
{
    static constexpr size_t M = 16, N = 64, K = 16;
    __m512 Alpha;
    forceinline G2RGBAf_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_ps(src);
        const auto out0 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32( 3,  3,  3,  3,  2,  2,  2,  2,  1,  1,  1,  1,  0,  0,  0,  0), dat);
        const auto out1 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32( 7,  7,  7,  7,  6,  6,  6,  6,  5,  5,  5,  5,  4,  4,  4,  4), dat);
        const auto out2 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(11, 11, 11, 11, 10, 10, 10, 10,  9,  9,  9,  9,  8,  8,  8,  8), dat);
        const auto out3 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(15, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 12), dat);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
struct GA2RGBf_512
{
    static constexpr size_t M = 32, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto out0 = _mm512_permutexvar_ps(_mm512_set_epi32(10,  8,  8,  8,  6,  6,  6,  4,  4,  4,  2,  2,  2,  0,  0,  0), dat0);
        const auto out1 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(20, 20, 18, 18, 18, 16, 16, 16, 14, 14, 14, 12, 12, 12, 10, 10), dat1);
        const auto out2 = _mm512_permutexvar_ps(_mm512_set_epi32(14, 14, 14, 12, 12, 12, 10, 10, 10,  8,  8,  8,  6,  6,  6,  4), dat1);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct GA2RGBAf_512
{
    static constexpr size_t M = 16, N = 32, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_ps(src);
        const auto out0 = _mm512_permutexvar_ps(_mm512_set_epi32( 7,  6,  6,  6,  5,  4,  4,  4,  3,  2,  2,  2,  1,  0,  0,  0), dat);
        const auto out1 = _mm512_permutexvar_ps(_mm512_set_epi32(15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10,  9,  8,  8,  8), dat);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB16_3To4_512
{
    static constexpr size_t M = 96, N = 128, K = 32;
    __m512i Alpha;
    forceinline RGB16_3To4_512(uint16_t alpha) noexcept : Alpha(_mm512_set1_epi16(static_cast<short>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);
#define BGR4(i) 0, B + i, G + i, R + i
        const auto out0 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, _mm512_set_epi16(BGR4(21), BGR4(18), BGR4(15), BGR4(12), BGR4( 9), BGR4( 6), BGR4( 3), BGR4( 0)), dat0);
        const auto mid01 = _mm512_mask_blend_epi16(0x0000ffff, dat0, dat1);
        const auto out1 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, _mm512_set_epi16(BGR4(45), BGR4(42), BGR4(39), BGR4(36), BGR4(33), BGR4(30), BGR4(27), BGR4(24)), mid01);
        const auto mid12 = _mm512_mask_blend_epi16(0x0000ffff, dat1, dat2);
        const auto out2 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, _mm512_set_epi16(BGR4(37), BGR4(34), BGR4(31), BGR4(28), BGR4(25), BGR4(22), BGR4(19), BGR4(16)), mid12);
        const auto out3 = _mm512_mask_permutexvar_epi16(Alpha, 0x77777777, _mm512_set_epi16(BGR4(29), BGR4(26), BGR4(23), BGR4(20), BGR4(17), BGR4(14), BGR4(11), BGR4( 8)), dat2);
#undef BGR4
        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
        _mm512_storeu_epi16(dst + 96, out3);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB16_4To3_512
{
    static constexpr size_t M = 128, N = 96, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);
        const auto dat3 = _mm512_loadu_epi16(src + 96);
#define BGR3(i) B + i, G + i, R + i
        const auto out0 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(G + 40, R + 40, BGR3(36), BGR3(32), BGR3(28), BGR3(24), BGR3(20), BGR3(16), BGR3(12), BGR3( 8), BGR3( 4), BGR3( 0)), dat1);
        const auto out1 = _mm512_permutex2var_epi16(dat1, _mm512_set_epi16(R + 52, BGR3(48), BGR3(44), BGR3(40), BGR3(36), BGR3(32), BGR3(28), BGR3(24), BGR3(20), BGR3(16), BGR3(12), B +  8), dat2);
        const auto out2 = _mm512_permutex2var_epi16(dat2, _mm512_set_epi16(BGR3(60), BGR3(56), BGR3(52), BGR3(48), BGR3(44), BGR3(40), BGR3(36), BGR3(32), BGR3(28), BGR3(24), B + 20, G + 20), dat3);
#undef BGR3
        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGBf_3To4_512
{
    static constexpr size_t M = 48, N = 64, K = 16;
    __m512 Alpha;
    forceinline RGBf_3To4_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);
#define BGR4(i) 0, B + i, G + i, R + i
        const auto out0 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(BGR4( 9), BGR4( 6), BGR4( 3), BGR4( 0)), dat0);
        const auto mid01 = _mm512_mask_blend_ps(0x00ff, dat0, dat1);
        const auto out1 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(BGR4(21), BGR4(18), BGR4(15), BGR4(12)), mid01);
        const auto mid12 = _mm512_mask_blend_ps(0x00ff, dat1, dat2);
        const auto out2 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(BGR4(17), BGR4(14), BGR4(11), BGR4( 8)), mid12);
        const auto out3 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(BGR4(13), BGR4(10), BGR4( 7), BGR4( 4)), dat2);
#undef BGR4
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGBf_4To3_512
{
    static constexpr size_t M = 64, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);
        const auto dat3 = _mm512_loadu_ps(src + 48);

        const auto out0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(R + 20, B + 16, G + 16, R + 16, B + 12, G + 12, R + 12, B +  8, G +  8, R +  8, B +  4, G +  4, R +  4, B +  0, G +  0, R +  0), dat1);
        const auto out1 = _mm512_permutex2var_ps(dat1, _mm512_set_epi32(G + 24, R + 24, B + 20, G + 20, R + 20, B + 16, G + 16, R + 16, B + 12, G + 12, R + 12, B +  8, G +  8, R +  8, B +  4, G +  4), dat2);
        const auto out2 = _mm512_permutex2var_ps(dat2, _mm512_set_epi32(B + 28, G + 28, R + 28, B + 24, G + 24, R + 24, B + 20, G + 20, R + 20, B + 16, G + 16, R + 16, B + 12, G + 12, R + 12, B +  8), dat3);
        
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct RGBToBGR16_512
{
    static constexpr size_t M = 96, N = 96, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);
        const auto mix02 = _mm512_mask_blend_epi16(0xffff0000, dat2, dat0);
        std::atomic_signal_fence(std::memory_order_acquire);

        const auto out0 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(31, 32, 27, 28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2), dat1);
        const auto out2 = _mm512_permutex2var_epi16(dat2, _mm512_set_epi16(29, 30, 31, 26, 27, 28, 23, 24, 25, 20, 21, 22, 17, 18, 19, 14, 15, 16, 11, 12, 13,  8,  9, 10,  5,  6,  7,  2,  3,  4, 63,  0), dat1);
        const auto out1 = _mm512_permutex2var_epi16(dat1, _mm512_set_epi16(33, 28, 29, 30, 25, 26, 27, 22, 23, 24, 19, 20, 21, 16, 17, 18, 13, 14, 15, 10, 11, 12,  7,  8,  9,  4,  5,  6,  1,  2,  3, 62), mix02);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
    }
};
struct RGBAToBGRA16_512
{
    static constexpr size_t M = 128, N = 128, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src + 0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);
        const auto dat3 = _mm512_loadu_epi16(src + 96);

        const auto shuffleMask = _mm512_set_epi64(0x001f001c001d001e, 0x001b00180019001a, 0x0017001400150016, 0x0013001000110012, 0x000f000c000d000e, 0x000b00080009000a, 0x0007000400050006, 0x0003000000010002);
        const auto out0 = _mm512_permutexvar_epi16(shuffleMask, dat0);
        const auto out1 = _mm512_permutexvar_epi16(shuffleMask, dat1);
        const auto out2 = _mm512_permutexvar_epi16(shuffleMask, dat2);
        const auto out3 = _mm512_permutexvar_epi16(shuffleMask, dat3);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
        _mm512_storeu_epi16(dst + 96, out3);
    }
};
struct RGBToBGRf_512
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);
        const auto mix02 = _mm512_mask_blend_ps(0xff00, dat2, dat0);
        std::atomic_signal_fence(std::memory_order_acquire);

        const auto out0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2), dat1);
        const auto out2 = _mm512_permutex2var_ps(dat2, _mm512_set_epi32(13, 14, 15, 10, 11, 12,  7,  8,  9,  4,  5,  6,  1,  2,  3, 30), dat1);
        const auto out1 = _mm512_permutex2var_ps(dat1, _mm512_set_epi32(15, 16, 11, 12, 13,  8,  9, 10,  5,  6,  7,  2,  3,  4, 31,  0), mix02);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct RGBAToBGRAf_512
{
    static constexpr size_t M = 64, N = 64, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);
        const auto dat3 = _mm512_loadu_ps(src + 48);

        const auto out0 = _mm512_permute_ps(dat0, 0b11000110);
        const auto out1 = _mm512_permute_ps(dat1, 0b11000110);
        const auto out2 = _mm512_permute_ps(dat2, 0b11000110);
        const auto out3 = _mm512_permute_ps(dat3, 0b11000110);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
template<uint8_t Idx>
struct KeepChannel16_3_1_512
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);

        __m512i shuffleMask;
             if constexpr (Idx == 0) shuffleMask = _mm512_set_epi16(29, 26, 23, 20, 17, 14, 11,  8, 5, 2, 63, 60, 57, 54, 51, 48, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12,  9, 6, 3, 0);
        else if constexpr (Idx == 1) shuffleMask = _mm512_set_epi16(30, 27, 24, 21, 18, 15, 12,  9, 6, 3,  0, 61, 58, 55, 52, 49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 13, 10, 7, 4, 1);
        else if constexpr (Idx == 2) shuffleMask = _mm512_set_epi16(31, 28, 25, 22, 19, 16, 13, 10, 7, 4,  1, 62, 59, 56, 53, 50, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17, 14, 11, 8, 5, 2);

        const auto mid = _mm512_permutex2var_epi16(dat0, shuffleMask, dat1);
        const auto out0 = _mm512_mask_permutexvar_epi16(mid, Idx == 0 ? 0xffc00000u : 0xffe00000u, shuffleMask, dat2);

        _mm512_storeu_epi16(dst, out0);
    }
};
template<uint8_t Idx>
struct KeepChannelF_4_1_512
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);

        const auto shuffleMask = _mm512_set_epi32(-1, -1, -1, -1, -1, -1, -1, -1, Idx + 28, Idx + 24, Idx + 20, Idx + 16, Idx + 12, Idx + 8, Idx + 4, Idx + 0);
        const F32x8 out = _mm512_castps512_ps256(_mm512_permutex2var_ps(dat0, shuffleMask, dat1));
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF_3_1_512
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src + 0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);

        const auto shuffleMask0 = _mm512_set_epi32(-1, -1, -1, -1, -1, Idx + 30, Idx + 27, Idx + 24, Idx + 21, Idx + 18, Idx + 15, Idx + 12, Idx + 9, Idx + 6, Idx + 3, Idx + 0);
        const auto mid01 = _mm512_permutex2var_ps(dat0, shuffleMask0, dat1);
        const auto shuffleMask1 = _mm512_set_epi32(Idx + 13, Idx + 10, Idx + 7, Idx + 4, Idx + 1, Idx - 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto insMask = Idx == 2 ? 0b1111110000000000u : 0b1111100000000000u;

        const auto out = _mm512_mask_permutexvar_ps(mid01, insMask, shuffleMask1, dat2);
        _mm512_storeu_ps(dst, out);
    }
};
struct Extract8x2_512
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);

        const U8x32 out0 = _mm512_cvtepi16_epi8(dat0);
        const U8x32 out1 = _mm512_cvtepi16_epi8(_mm512_srli_epi16(dat0, 8));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract8x2_512_2
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto shuffleMask = mm512_set_lane(epi8, 15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);
        const auto mid0 = _mm512_shuffle_epi8(dat0, shuffleMask); // 0A 1B 2C 3D
        const auto mid1 = _mm512_shuffle_epi8(dat1, shuffleMask); // 4E 5F 6G 7H

        const auto permMask0 = _mm512_set_epi64(14, 12, 10, 8, 6, 4, 2, 0);
        const auto permMask1 = _mm512_set_epi64(15, 13, 11, 9, 7, 5, 3, 1);
        const auto out0 = _mm512_permutex2var_epi64(mid0, permMask0, mid1);
        const auto out1 = _mm512_permutex2var_epi64(mid0, permMask1, mid1);

        _mm512_storeu_epi8(dst[0], out0);
        _mm512_storeu_epi8(dst[1], out1);
    }
};
struct Extract8x3_512
{
    static constexpr size_t M = 192, N = 64, K = 64;
    static forceinline std::tuple<__m512i, __m512i, __m512i> DoLoadMixed(const uint8_t* __restrict src) noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        const auto dat0693 = _mm512_mask_shuffle_i64x2(dat0, 0x3c, dat1, dat2, 0b01011010); // 1 1 2 2
        const auto dat41a7 = _mm512_mask_shuffle_i64x2(dat1, 0x3c, dat0, dat2, 0b10100101); // 2 2 1 1
        const auto dat825b = _mm512_mask_shuffle_i64x2(dat2, 0x3c, dat0, dat1, 0b01011010); // 1 1 2 2

        const auto dat0369 = _mm512_shuffle_i64x2(dat0693, dat0693, 0b10011100); // 2 1 3 0 // RGBRGBRGBRGBRGBR
        const auto dat147a = _mm512_shuffle_i64x2(dat41a7, dat41a7, 0b10110001); // 2 3 0 1 // GBRGBRGBRGBRGBRG, R=0, G=1, B=2
        const auto dat258b = _mm512_shuffle_i64x2(dat825b, dat825b, 0b11001001); // 3 0 2 1 // BRGBRGBRGBRGBRGB, R=2, G=0, B=1

        constexpr uint64_t BlendMask[3] = {
            0b0100100100100100u * 0x0001000100010001u,
            0b1001001001001001u * 0x0001000100010001u,
            0b0010010010010010u * 0x0001000100010001u };
        const auto blendMask0 = _cvtu64_mask64(BlendMask[0]), blendMask1 = _cvtu64_mask64(BlendMask[1]), blendMask2 = _cvtu64_mask64(BlendMask[2]);

        const auto datR = _mm512_mask_blend_epi8(blendMask2, _mm512_mask_blend_epi8(blendMask0, dat0369, dat147a), dat258b);
        const auto datG = _mm512_mask_blend_epi8(blendMask0, _mm512_mask_blend_epi8(blendMask1, dat0369, dat147a), dat258b);
        const auto datB = _mm512_mask_blend_epi8(blendMask1, _mm512_mask_blend_epi8(blendMask2, dat0369, dat147a), dat258b);

        return { datR, datG, datB };
    }
    static forceinline std::tuple<__m512i, __m512i, __m512i> DoLoad(const uint8_t* __restrict src) noexcept
    {
        const auto [datR, datG, datB] = DoLoadMixed(src);
        const auto shuffleMaskR = mm512_set_lane(epi8, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12,  9,  6,  3,  0);
        const auto shuffleMaskG = mm512_set_lane(epi8, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 13, 10,  7,  4,  1);
        const auto shuffleMaskB = mm512_set_lane(epi8, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17, 14, 11,  8,  5,  2);

        const auto out0 = _mm512_shuffle_epi8(datR, shuffleMaskR);
        const auto out1 = _mm512_shuffle_epi8(datG, shuffleMaskG);
        const auto out2 = _mm512_shuffle_epi8(datB, shuffleMaskB);
        return { out0, out1, out2 };
    }
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto [out0, out1, out2] = DoLoad(src);
        _mm512_storeu_epi8(dst[0], out0);
        _mm512_storeu_epi8(dst[1], out1);
        _mm512_storeu_epi8(dst[2], out2);
    }
};
struct Extract8x4_512
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);

        const U8x16 out0 = _mm512_cvtepi32_epi8(dat0);
        const U8x16 out1 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(dat0,  8));
        const U8x16 out2 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(dat0, 16));
        const U8x16 out3 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(dat0, 24));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract8x4_512_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        const auto shuffleMask = mm512_set_lane(epi8, 15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
        const auto mid0 = _mm512_shuffle_epi8(dat0, shuffleMask); // 0123 4567 89ab cdef
        const auto mid1 = _mm512_shuffle_epi8(dat1, shuffleMask); // ghij klmn opqr stuv

        const auto permMask0 = _mm512_set_epi32(29, 25, 21, 17, 13,  9, 5, 1, 28, 24, 20, 16, 12,  8, 4, 0);
        const auto permMask1 = _mm512_set_epi32(31, 27, 23, 19, 15, 11, 7, 3, 30, 26, 22, 18, 14, 10, 6, 2);
        const auto val0 = _mm512_permutex2var_epi32(mid0, permMask0, mid1); // 048c gkos 159d hlpt
        const auto val1 = _mm512_permutex2var_epi32(mid0, permMask1, mid1); // 26ae imqu 37bf jnrv

        const U8x32 out0 = _mm512_castsi512_si256(val0);
        const U8x32 out1 = _mm512_extracti64x4_epi64(val0, 1);
        const U8x32 out2 = _mm512_castsi512_si256(val1);
        const U8x32 out3 = _mm512_extracti64x4_epi64(val1, 1);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract16x2_512
{
    static constexpr size_t M = 64, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto out0 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0), dat1);
        const auto out1 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(63, 61, 59, 57, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1), dat1);

        _mm512_storeu_epi16(dst[0], out0);
        _mm512_storeu_epi16(dst[1], out1);
    }
};
struct Extract16x3_512
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto dat2 = _mm512_loadu_epi16(src + 64);

        const auto shuffleMask0 = _mm512_set_epi16(29, 26, 23, 20, 17, 14, 11,  8, 5, 2, 63, 60, 57, 54, 51, 48, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12,  9, 6, 3, 0);
        const auto shuffleMask1 = _mm512_set_epi16(30, 27, 24, 21, 18, 15, 12,  9, 6, 3,  0, 61, 58, 55, 52, 49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 13, 10, 7, 4, 1);
        const auto shuffleMask2 = _mm512_set_epi16(31, 28, 25, 22, 19, 16, 13, 10, 7, 4,  1, 62, 59, 56, 53, 50, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17, 14, 11, 8, 5, 2);

        const auto mid0 = _mm512_permutex2var_epi16(dat0, shuffleMask0, dat1);
        const auto mid1 = _mm512_permutex2var_epi16(dat0, shuffleMask1, dat1);
        const auto mid2 = _mm512_permutex2var_epi16(dat0, shuffleMask2, dat1);

        const auto out0 = _mm512_mask_permutexvar_epi16(mid0, 0xffc00000u, shuffleMask0, dat2);
        const auto out1 = _mm512_mask_permutexvar_epi16(mid1, 0xffe00000u, shuffleMask1, dat2);
        const auto out2 = _mm512_mask_permutexvar_epi16(mid2, 0xffe00000u, shuffleMask2, dat2);

        _mm512_storeu_epi16(dst[0], out0);
        _mm512_storeu_epi16(dst[1], out1);
        _mm512_storeu_epi16(dst[2], out2);
    }
};
struct Extract16x4_512
{
    static constexpr size_t M = 64, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const U16x16 out0 = _mm512_castsi512_si256(_mm512_permutex2var_epi16(dat0, _mm512_castsi256_si512(_mm256_set_epi16(60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12,  8, 4, 0)), dat1));
        const U16x16 out1 = _mm512_castsi512_si256(_mm512_permutex2var_epi16(dat0, _mm512_castsi256_si512(_mm256_set_epi16(61, 57, 53, 49, 45, 41, 37, 33, 29, 25, 21, 17, 13,  9, 5, 1)), dat1));
        const U16x16 out2 = _mm512_castsi512_si256(_mm512_permutex2var_epi16(dat0, _mm512_castsi256_si512(_mm256_set_epi16(62, 58, 54, 50, 46, 42, 38, 34, 30, 26, 22, 18, 14, 10, 6, 2)), dat1));
        const U16x16 out3 = _mm512_castsi512_si256(_mm512_permutex2var_epi16(dat0, _mm512_castsi256_si512(_mm256_set_epi16(63, 59, 55, 51, 47, 43, 39, 35, 31, 27, 23, 19, 15, 11, 7, 3)), dat1));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Extract32x2_512
{
    static constexpr size_t M = 32, N = 16, K = 16;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src);
        const auto dat1 = _mm512_loadu_ps(src + 16);

        const auto out0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0), dat1);
        const auto out1 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1), dat1);

        _mm512_storeu_ps(dst[0], out0);
        _mm512_storeu_ps(dst[1], out1);
    }
};
struct Extract32x3_512
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);

        const auto shuffleMask0 = _mm512_set_epi32(13, 10, 7, 4, 1, 30, 27, 24, 21, 18, 15, 12,  9,  6,  3,  0);
        const auto shuffleMask1 = _mm512_set_epi32(14, 11, 8, 5, 2, 31, 28, 25, 22, 19, 16, 13, 10,  7,  4,  1);
        const auto shuffleMask2 = _mm512_set_epi32(15, 12, 9, 6, 3,  0, 29, 26, 23, 20, 17, 14, 11,  8,  5,  2);

        const auto mid0 = _mm512_permutex2var_ps(dat0, shuffleMask0, dat1);
        const auto mid1 = _mm512_permutex2var_ps(dat0, shuffleMask1, dat1);
        const auto mid2 = _mm512_permutex2var_ps(dat0, shuffleMask2, dat1);

        const auto out0 = _mm512_mask_permutexvar_ps(mid0, 0b1111100000000000, shuffleMask0, dat2);
        const auto out1 = _mm512_mask_permutexvar_ps(mid1, 0b1111100000000000, shuffleMask1, dat2);
        const auto out2 = _mm512_mask_permutexvar_ps(mid2, 0b1111110000000000, shuffleMask2, dat2);

        _mm512_storeu_ps(dst[0], out0);
        _mm512_storeu_ps(dst[1], out1);
        _mm512_storeu_ps(dst[2], out2);
    }
};
struct Extract32x4_512
{
    static constexpr size_t M = 32, N = 8, K = 8;
    forceinline void operator()(float* __restrict* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src);
        const auto dat1 = _mm512_loadu_ps(src + 16);

        const F32x8 out0 = _mm512_castps512_ps256(_mm512_permutex2var_ps(dat0, _mm512_castsi256_si512(_mm256_set_epi32(28, 24, 20, 16, 12,  8, 4, 0)), dat1));
        const F32x8 out1 = _mm512_castps512_ps256(_mm512_permutex2var_ps(dat0, _mm512_castsi256_si512(_mm256_set_epi32(29, 25, 21, 17, 13,  9, 5, 1)), dat1));
        const F32x8 out2 = _mm512_castps512_ps256(_mm512_permutex2var_ps(dat0, _mm512_castsi256_si512(_mm256_set_epi32(30, 26, 22, 18, 14, 10, 6, 2)), dat1));
        const F32x8 out3 = _mm512_castps512_ps256(_mm512_permutex2var_ps(dat0, _mm512_castsi256_si512(_mm256_set_epi32(31, 27, 23, 19, 15, 11, 7, 3)), dat1));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Combine8x2_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src[0]);
        const auto dat1 = _mm512_loadu_epi8(src[1]);

        const auto mixLo = _mm512_unpacklo_epi8(dat0, dat1); // 0246
        const auto mixHi = _mm512_unpackhi_epi8(dat0, dat1); // 1357

        const auto out0 = _mm512_permutex2var_epi64(mixLo, _mm512_set_epi64(11, 10, 3, 2,  9,  8, 1, 0), mixHi);
        const auto out1 = _mm512_permutex2var_epi64(mixLo, _mm512_set_epi64(15, 14, 7, 6, 13, 12, 5, 4), mixHi);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
    }
};
struct Combine8x3_512
{
    static constexpr size_t M = 64, N = 192, K = 64;
    static forceinline void DoStore(uint8_t* __restrict dst, const __m512i& dat0, const __m512i& dat1, const __m512i& dat2) noexcept
    {
        // RGBRGBRGBRGBRGBR, R=1001001001001001 G=0100100100100100 B=0010010010010010
        // GBRGBRGBRGBRGBRG, R=0010010010010010 G=1001001001001001 B=0100100100100100
        // BRGBRGBRGBRGBRGB, R=0100100100100100 G=0010010010010010 B=1001001001001001
        const auto shuffleMaskR = mm512_set_lane(epi8,  5, 10, 15,  4,  9, 14,  3,  8, 13,  2,  7, 12,  1,  6, 11,  0);
        const auto shuffleMaskG = mm512_set_lane(epi8, 10, 15,  4,  9, 14,  3,  8, 13,  2,  7, 12,  1,  6, 11,  0,  5);
        const auto shuffleMaskB = mm512_set_lane(epi8, 15,  4,  9, 14,  3,  8, 13,  2,  7, 12,  1,  6, 11,  0,  5, 10);
        const auto datR = _mm512_shuffle_epi8(dat0, shuffleMaskR);
        const auto datG = _mm512_shuffle_epi8(dat1, shuffleMaskG);
        const auto datB = _mm512_shuffle_epi8(dat2, shuffleMaskB);

        constexpr uint64_t BlendMask[3] = {
            0b0010010010010010u * 0x0001000100010001u,
            0b0100100100100100u * 0x0001000100010001u,
            0b1001001001001001u * 0x0001000100010001u };
        const auto blendMask0 = _cvtu64_mask64(BlendMask[0]), blendMask1 = _cvtu64_mask64(BlendMask[1]), blendMask2 = _cvtu64_mask64(BlendMask[2]);

        const auto out0369 = _mm512_mask_blend_epi8(blendMask1, _mm512_mask_blend_epi8(blendMask0, datR, datG), datB);
        const auto out147a = _mm512_mask_blend_epi8(blendMask0, _mm512_mask_blend_epi8(blendMask2, datR, datG), datB);
        const auto out258b = _mm512_mask_blend_epi8(blendMask2, _mm512_mask_blend_epi8(blendMask1, datR, datG), datB);

        const auto out0617 = _mm512_shuffle_i64x2(out0369, out147a, 0b10001000);
        const auto out2839 = _mm512_shuffle_i64x2(out258b, out0369, 0b11011000);
        const auto out4a5b = _mm512_shuffle_i64x2(out147a, out258b, 0b11011101);

        const auto out0123 = _mm512_shuffle_i64x2(out0617, out2839, 0b10001000);
        const auto out4567 = _mm512_shuffle_i64x2(out4a5b, out0617, 0b11011000);
        const auto out89ab = _mm512_shuffle_i64x2(out2839, out4a5b, 0b11011101);

        _mm512_storeu_epi8(dst +   0, out0123);
        _mm512_storeu_epi8(dst +  64, out4567);
        _mm512_storeu_epi8(dst + 128, out89ab);
    }

    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src[0]);
        const auto dat1 = _mm512_loadu_epi8(src[1]);
        const auto dat2 = _mm512_loadu_epi8(src[2]);
        DoStore(dst, dat0, dat1, dat2);
    }
};
struct Combine8x4_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src[0]);
        const auto dat1 = _mm512_loadu_epi8(src[1]);
        const auto dat2 = _mm512_loadu_epi8(src[2]);
        const auto dat3 = _mm512_loadu_epi8(src[3]);

        const auto dat02Lo = _mm512_shuffle_i64x2(dat0, dat2, 0b01000100);
        const auto dat02Hi = _mm512_shuffle_i64x2(dat0, dat2, 0b11101110);
        const auto dat13Lo = _mm512_shuffle_i64x2(dat1, dat3, 0b01000100);
        const auto dat13Hi = _mm512_shuffle_i64x2(dat1, dat3, 0b11101110);

        const auto shuffleLo = _mm512_set_epi16(
            55, 39, 54, 38, 53, 37, 52, 36, 51, 35, 50, 34, 49, 33, 48, 32,
            23,  7, 22,  6, 21,  5, 20,  4, 19,  3, 18,  2, 17,  1, 16,  0);
        const auto shuffleHi = _mm512_set_epi16(
            63, 47, 62, 46, 61, 45, 60, 44, 59, 43, 58, 42, 57, 41, 56, 40,
            31, 15, 30, 14, 29, 13, 28, 12, 27, 11, 26, 10, 25,  9, 24,  8);

        const auto mix0213LoLo = _mm512_unpacklo_epi8(dat02Lo, dat13Lo); // RG0Lo RG1Lo BA0Lo BA1Lo
        const auto mix0213LoHi = _mm512_unpackhi_epi8(dat02Lo, dat13Lo); // RG0Hi RG1Hi BA0Hi BA1Hi
        const auto out0 = _mm512_permutex2var_epi16(mix0213LoLo, shuffleLo, mix0213LoHi);
        const auto out1 = _mm512_permutex2var_epi16(mix0213LoLo, shuffleHi, mix0213LoHi);

        const auto mix0213HiLo = _mm512_unpacklo_epi8(dat02Hi, dat13Hi); // RG2Lo RG3Lo BA2Lo BA3Lo
        const auto mix0213HiHi = _mm512_unpackhi_epi8(dat02Hi, dat13Hi); // RG2Hi RG3Hi BA2Hi BA3Hi
        const auto out2 = _mm512_permutex2var_epi16(mix0213HiLo, shuffleLo, mix0213HiHi);
        const auto out3 = _mm512_permutex2var_epi16(mix0213HiLo, shuffleHi, mix0213HiHi);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 16, out1);
        _mm512_storeu_epi16(dst + 32, out2);
        _mm512_storeu_epi16(dst + 48, out3);
    }
};
struct Combine16x2_512
{
    static constexpr size_t M = 32, N = 64, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src[0]);
        const auto dat1 = _mm512_loadu_epi16(src[1]);
        const auto out0 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(47, 15, 46, 14, 45, 13, 44, 12, 43, 11, 42, 10, 41,  9, 40,  8, 39,  7, 38,  6, 37,  5, 36,  4, 35,  3, 34,  2, 33,  1, 32,  0), dat1);
        const auto out1 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(63, 31, 62, 30, 61, 29, 60, 28, 59, 27, 58, 26, 57, 25, 56, 24, 55, 23, 54, 22, 53, 21, 52, 20, 51, 19, 50, 18, 49, 17, 48, 16), dat1);
        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
    }
};
struct Combine16x3_512
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src[0]);
        const auto dat1 = _mm512_loadu_epi16(src[1]);
        const auto dat2 = _mm512_loadu_epi16(src[2]);

#define RG_(i) -1, i+32, i
        const auto valRG_0 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(42, 10, RG_(9), RG_(8), RG_(7), RG_(6), RG_(5), RG_(4), RG_(3), RG_(2), RG_(1), RG_(0)), dat1);
        const auto valRG_1 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(21, RG_(20), RG_(19), RG_(18), RG_(17), RG_(16), RG_(15), RG_(14), RG_(13), RG_(12), RG_(11), -1), dat1);
        const auto valRG_2 = _mm512_permutex2var_epi16(dat0, _mm512_set_epi16(RG_(31), RG_(30), RG_(29), RG_(28), RG_(27), RG_(26), RG_(25), RG_(24), RG_(23), RG_(22), -1, 53), dat1);
#undef RG_
#define B3(i) i, i, i
        const auto out0 = _mm512_mask_permutexvar_epi16(valRG_0, 0x24924924u, _mm512_set_epi16(10, 10, B3(9), B3(8), B3(7), B3(6), B3(5), B3(4), B3(3), B3(2), B3(1), B3(0)), dat2);
        const auto out1 = _mm512_mask_permutexvar_epi16(valRG_1, 0x49249249u, _mm512_set_epi16(21, B3(20), B3(19), B3(18), B3(17), B3(16), B3(15), B3(14), B3(13), B3(12), B3(11), 10), dat2);
        const auto out2 = _mm512_mask_permutexvar_epi16(valRG_2, 0x92492492u, _mm512_set_epi16(B3(31), B3(30), B3(29), B3(28), B3(27), B3(26), B3(25), B3(24), B3(23), B3(22), 21, 21), dat2);
#undef B3

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
    }
};
struct Combine16x4_512
{
    static constexpr size_t M = 32, N = 128, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint16_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src[0]);
        const auto dat1 = _mm512_loadu_epi16(src[1]);
        const auto dat2 = _mm512_loadu_epi16(src[2]);
        const auto dat3 = _mm512_loadu_epi16(src[3]);

        const auto shuffleLo = _mm512_set_epi16(
            47, 15, 46, 14, 39,  7, 38,  6, 
            45, 13, 44, 12, 37,  5, 36,  4, 
            43, 11, 42, 10, 35,  3, 34,  2, 
            41,  9, 40,  8, 33,  1, 32,  0);
        const auto shuffleHi = _mm512_set_epi16(
            63, 31, 62, 30, 55, 23, 54, 22, 
            61, 29, 60, 28, 53, 21, 52, 20, 
            59, 27, 58, 26, 51, 19, 50, 18, 
            57, 25, 56, 24, 49, 17, 48, 16);

        const auto mix01Lo = _mm512_permutex2var_epi16(dat0, shuffleLo, dat1);
        const auto mix01Hi = _mm512_permutex2var_epi16(dat0, shuffleHi, dat1);
        const auto mix23Lo = _mm512_permutex2var_epi16(dat2, shuffleLo, dat3);
        const auto mix23Hi = _mm512_permutex2var_epi16(dat2, shuffleHi, dat3);

        const auto out0 = _mm512_unpacklo_epi32(mix01Lo, mix23Lo);
        const auto out1 = _mm512_unpackhi_epi32(mix01Lo, mix23Lo);
        const auto out2 = _mm512_unpacklo_epi32(mix01Hi, mix23Hi);
        const auto out3 = _mm512_unpackhi_epi32(mix01Hi, mix23Hi);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
        _mm512_storeu_epi16(dst + 64, out2);
        _mm512_storeu_epi16(dst + 96, out3);
    }
};
struct Combine32x2_512
{
    static constexpr size_t M = 16, N = 32, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src[0]);
        const auto dat1 = _mm512_loadu_ps(src[1]);
        const auto out0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(23,  7, 22,  6, 21,  5, 20,  4, 19,  3, 18,  2, 17,  1, 16,  0), dat1);
        const auto out1 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(31, 15, 30, 14, 29, 13, 28, 12, 27, 11, 26, 10, 25,  9, 24,  8), dat1);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
    }
};
struct Combine32x3_512
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src[0]);
        const auto dat1 = _mm512_loadu_ps(src[1]);
        const auto dat2 = _mm512_loadu_ps(src[2]);

        const auto valRGR0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32( 5,  4, 20,  4,  3, 19,  3,  2, 18,  2,  1, 17,  1,  0, 16,  0), dat1);
        const auto valRGR1 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(26, 10,  9, 25,  9,  8, 24,  8,  7, 23,  7,  6, 22,  6,  5, 21), dat1);
        const auto valRGR2 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(15, 31, 15, 14, 30, 14, 13, 29, 13, 12, 28, 12, 11, 27, 11, 10), dat1);

        const auto out0 = _mm512_mask_permutexvar_ps(valRGR0, 0b0100100100100100, _mm512_set_epi32( 5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0), dat2);
        const auto out1 = _mm512_mask_permutexvar_ps(valRGR1, 0b0010010010010010, _mm512_set_epi32(10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5), dat2);
        const auto out2 = _mm512_mask_permutexvar_ps(valRGR2, 0b1001001001001001, _mm512_set_epi32(15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10), dat2);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct Combine32x4_512
{
    static constexpr size_t M = 16, N = 64, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src[0]);
        const auto dat1 = _mm512_loadu_ps(src[1]);
        const auto dat2 = _mm512_loadu_ps(src[2]);
        const auto dat3 = _mm512_loadu_ps(src[3]);
        
        const auto shuffleLo = _mm512_set_epi32(23,  7, 19,  3, 22,  6, 18,  2, 21,  5, 17, 1, 20,  4, 16, 0);
        const auto shuffleHi = _mm512_set_epi32(31, 15, 27, 11, 30, 14, 26, 10, 29, 13, 25, 9, 28, 12, 24, 8);

        const auto mix01Lo = _mm512_permutex2var_ps(dat0, shuffleLo, dat1);
        const auto mix01Hi = _mm512_permutex2var_ps(dat0, shuffleHi, dat1);
        const auto mix23Lo = _mm512_permutex2var_ps(dat2, shuffleLo, dat3);
        const auto mix23Hi = _mm512_permutex2var_ps(dat2, shuffleHi, dat3);

        const auto out0 = _mm512_shuffle_ps(mix01Lo, mix23Lo, 0b01000100);
        const auto out1 = _mm512_shuffle_ps(mix01Lo, mix23Lo, 0b11101110);
        const auto out2 = _mm512_shuffle_ps(mix01Hi, mix23Hi, 0b01000100);
        const auto out3 = _mm512_shuffle_ps(mix01Hi, mix23Hi, 0b11101110);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
struct G8ToG16_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src);

        const auto mixLo = _mm512_unpacklo_epi8(dat, dat); // 0246
        const auto mixHi = _mm512_unpackhi_epi8(dat, dat); // 1357

        const auto out0 = _mm512_permutex2var_epi64(mixLo, _mm512_set_epi64(11, 10, 3, 2,  9,  8, 1, 0), mixHi);
        const auto out1 = _mm512_permutex2var_epi64(mixLo, _mm512_set_epi64(15, 14, 7, 6, 13, 12, 5, 4), mixHi);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
    }
};
struct G16ToG8_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 32);
        const auto muler = _mm512_set1_epi16(static_cast<short>(0xff01));

        const auto mid0 = _mm512_srli_epi16(_mm512_mulhi_epu16(dat0, muler), 8);
        const auto mid1 = _mm512_srli_epi16(_mm512_mulhi_epu16(dat1, muler), 8);
        const U8x32 out0 = _mm512_cvtepi16_epi8(mid0);
        const U8x32 out1 = _mm512_cvtepi16_epi8(mid1);

        out0.Save(dst +  0);
        out1.Save(dst + 32);
    }
};
template<bool Is555>
struct RGB5x5ToRGB8_512Base
{
    forceinline static std::array<__m512i, 3> RGB5x5ToRGB8Hi(const __m512i& dat) noexcept
    {
        const auto maskLo5bit = _mm512_set1_epi16(0x1f);
        const auto maskLo6bit = _mm512_set1_epi16(0x3f);

        const auto valR = _mm512_and_si512(dat, maskLo5bit);
        const auto valG = _mm512_and_si512(_mm512_srli_epi16(dat, 5), Is555 ? maskLo5bit : maskLo6bit);
        auto valB = _mm512_srli_epi16(dat, Is555 ? 10 : 11);
        if constexpr (Is555) valB = _mm512_and_si512(valB, maskLo5bit);

        // pre-shift-left by 2
        const auto mul5 = _mm512_set1_epi16(527 << 2), mul6 = _mm512_set1_epi16(259 << 2);
        const auto add5 = _mm512_set1_epi16(23 << 2), add6 = _mm512_set1_epi16(33 << 2);
        const auto midR = _mm512_add_epi16(_mm512_mullo_epi16_force(valR, mul5), add5); // at Hi
        const auto midG = _mm512_add_epi16(_mm512_mullo_epi16_force(valG, Is555 ? mul5 : mul6), Is555 ? add5 : add6); // at Hi
        const auto midB = _mm512_add_epi16(_mm512_mullo_epi16_force(valB, mul5), add5); // at Hi

        return { midR, midG, midB };
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_512 : RGB5x5ToRGB8_512Base<Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src + 0);

        const auto [midR, midG, midB] = this->RGB5x5ToRGB8Hi(dat);

        const auto shuffleToLo = mm512_set_lane(epi8, 15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1);
        const auto insertMask = _cvtu64_mask64(0x5555555555555555u);
        const auto valLo = _mm512_mask_shuffle_epi8(midG, insertMask, IsRGB ? midR : midB, shuffleToLo);
        __m512i midA;
        if constexpr (HasAlpha)
            midA = _mm512_srai_epi16(dat, 7); // at Hi
        else
            midA = _mm512_set1_epi8((int8_t)-1);
        const auto valHi = _mm512_mask_shuffle_epi8(midA, insertMask, IsRGB ? midB : midR, shuffleToLo);

        const auto packLo = _mm512_set_epi16(47, 15, 46, 14, 45, 13, 44, 12, 43, 11, 42, 10, 41, 9, 40, 8, 39, 7, 38, 6, 37, 5, 36, 4, 35, 3, 34, 2, 33, 1, 32, 0);
        const auto packHi = _mm512_set_epi16(63, 31, 62, 30, 61, 29, 60, 28, 59, 27, 58, 26, 57, 25, 56, 24, 55, 23, 54, 22, 53, 21, 52, 20, 51, 19, 50, 18, 49, 17, 48, 16);
        const auto out0 = _mm512_permutex2var_epi16(valLo, packLo, valHi);
        const auto out1 = _mm512_permutex2var_epi16(valLo, packHi, valHi);
        _mm512_storeu_epi32(dst +  0, out0);
        _mm512_storeu_epi32(dst + 16, out1);
    }
};
template<bool Scale, bool IsRGB>
struct RGB10ToRGBf_512
{
    static constexpr size_t M = 16, N = 48, K = 16;
    __m512 Scaler;
    RGB10ToRGBf_512(float mulVal) : Scaler(_mm512_set1_ps(mulVal)) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi32(src);

        const auto maskLo10bit = _mm512_set1_epi32(0x3ff);
        const auto dat0 = _mm512_and_si512(maskLo10bit, dat);
        const auto dat1 = _mm512_and_si512(maskLo10bit, _mm512_srli_epi32(dat, 10));
        const auto dat2 = _mm512_and_si512(maskLo10bit, _mm512_srli_epi32(dat, 20));

        auto valR = _mm512_cvtepi32_ps(IsRGB ? dat0 : dat2);
        auto valG = _mm512_cvtepi32_ps(dat1);
        auto valB = _mm512_cvtepi32_ps(IsRGB ? dat2 : dat0);
        if constexpr (Scale)
        {
            valR = _mm512_mul_ps(valR, Scaler);
            valG = _mm512_mul_ps(valG, Scaler);
            valB = _mm512_mul_ps(valB, Scaler);
        }

        const auto shuffle0 = _mm512_set_epi32( 5,  4, 20,  4,  3, 19,  3,  2, 18,  2,  1, 17,  1,  0, 16,  0);
        const auto shuffle1 = _mm512_set_epi32(26, 10,  9, 25,  9,  8, 24,  8,  7, 23,  7,  6, 22,  6,  5, 21);
        const auto shuffle2 = _mm512_set_epi32(15, 31, 15, 14, 30, 14, 13, 29, 13, 12, 28, 12, 11, 27, 11, 10);
        const auto valRG_0 = _mm512_permutex2var_ps(valR, shuffle0, valG);
        const auto valRG_1 = _mm512_permutex2var_ps(valR, shuffle1, valG);
        const auto valRG_2 = _mm512_permutex2var_ps(valR, shuffle2, valG);
        const auto out0 = _mm512_mask_permutexvar_ps(valRG_0, 0b0100100100100100, shuffle0, valB);
        const auto out1 = _mm512_mask_permutexvar_ps(valRG_1, 0b0010010010010010, shuffle1, valB);
        const auto out2 = _mm512_mask_permutexvar_ps(valRG_2, 0b1001001001001001, shuffle2, valB);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
template<bool Scale, bool IsRGB, bool HasAlpha>
struct RGB10ToRGBAf_512
{
    static constexpr size_t M = 16, N = 64, K = 16;
    __m512 Scaler;
    RGB10ToRGBAf_512(float mulVal) : Scaler(_mm512_set1_ps(mulVal)) {}
    forceinline void operator()(float* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi32(src);

        const auto maskLo10bit = _mm512_set1_epi32(0x3ff);
        const auto dat0 = _mm512_and_si512(maskLo10bit, dat);
        const auto dat1 = _mm512_and_si512(maskLo10bit, _mm512_srli_epi32(dat, 10));
        const auto dat2 = _mm512_and_si512(maskLo10bit, _mm512_srli_epi32(dat, 20));

        auto valR = _mm512_cvtepi32_ps(IsRGB ? dat0 : dat2);
        auto valG = _mm512_cvtepi32_ps(dat1);
        auto valB = _mm512_cvtepi32_ps(IsRGB ? dat2 : dat0);
        __m512 valA;
        if constexpr (Scale)
        {
            valR = _mm512_mul_ps(valR, Scaler);
            valG = _mm512_mul_ps(valG, Scaler);
            valB = _mm512_mul_ps(valB, Scaler);
        }
        if constexpr (HasAlpha)
        {
            const auto alphaLUT = _mm512_setr_ps(0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 0.f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f);
            valA = _mm512_permutevar_ps(alphaLUT, _mm512_srli_epi32(dat, 30));
        }
        else
        {
            valA = _mm512_set1_ps(1.0f);
        }

        const auto shuffle0 = _mm512_set_epi32(19,  3, 19,  3, 18,  2, 18,  2, 17,  1, 17,  1, 16,  0, 16,  0);
        const auto shuffle1 = _mm512_set_epi32(23,  7, 23,  7, 22,  6, 22,  6, 21,  5, 21,  5, 20,  4, 20,  4);
        const auto shuffle2 = _mm512_set_epi32(27, 11, 27, 11, 26, 10, 26, 10, 25,  9, 25,  9, 24,  8, 24,  8);
        const auto shuffle3 = _mm512_set_epi32(31, 15, 31, 15, 30, 14, 30, 14, 29, 13, 29, 13, 28, 12, 28, 12);

        const auto valRG__0 = _mm512_maskz_permutex2var_ps(0x3333, valR, shuffle0, valG);
        const auto valRG__1 = _mm512_maskz_permutex2var_ps(0x3333, valR, shuffle1, valG);
        const auto valRG__2 = _mm512_maskz_permutex2var_ps(0x3333, valR, shuffle2, valG);
        const auto valRG__3 = _mm512_maskz_permutex2var_ps(0x3333, valR, shuffle3, valG);

        const auto val__BA0 = _mm512_maskz_permutex2var_ps(0xcccc, valB, shuffle0, valA);
        const auto val__BA1 = _mm512_maskz_permutex2var_ps(0xcccc, valB, shuffle1, valA);
        const auto val__BA2 = _mm512_maskz_permutex2var_ps(0xcccc, valB, shuffle2, valA);
        const auto val__BA3 = _mm512_maskz_permutex2var_ps(0xcccc, valB, shuffle3, valA);
        
        const auto out0 = _mm512_or_ps(valRG__0, val__BA0);
        const auto out1 = _mm512_or_ps(valRG__1, val__BA1);
        const auto out2 = _mm512_or_ps(valRG__2, val__BA2);
        const auto out3 = _mm512_or_ps(valRG__3, val__BA3);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
struct RGB8ToYUV8_I8_AVX512_Base
{
    static constexpr size_t M = 192, N = 192, K = 64;
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGB8ToYUV8_I8_AVX512 : public RGB8ToYUV8_I8_AVX512_Base
{
    const __m512i LutYRG, LutYGB, LutCbRB, LutCbGB, LutCrRG, LutCrRB;
    RGB8ToYUV8_I8_AVX512(const int16_t* lut) noexcept :
#define LLUT(x,i) Lut##x(_mm512_set1_epi16(lut[i]))
        LLUT(YRG, 0), LLUT(YGB, 1), LLUT(CbRB, 2), LLUT(CbGB, 3), LLUT(CrRG, 4), LLUT(CrRB, 5)
#undef LLUT
    { }
    template<bool IsY>
    static forceinline auto DP4(const __m512i& part0, const __m512i& part1, const __m512i& lut0, const __m512i& lut1) noexcept
    {
        const auto mid0 = _mm512_maddubs_epi16(part0, lut0), mid1 = _mm512_maddubs_epi16(part1, lut1);
        const auto sum = _mm512_add_epi16(mid0, mid1);
        const auto add128 = _mm512_set1_epi16(128);
        if constexpr (IsY)
            return _mm512_srli_epi16(_mm512_add_epi16 (sum, add128), 8);
        else
            return _mm512_srli_epi16(_mm512_adds_epi16(sum, add128), 8);
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto datRGB = Extract8x3_512::DoLoad(src);

        const auto midRG0 = _mm512_unpacklo_epi8(std::get<R>(datRGB), std::get<G>(datRGB));
        const auto midRG1 = _mm512_unpackhi_epi8(std::get<R>(datRGB), std::get<G>(datRGB));
        const auto midGB0 = _mm512_unpacklo_epi8(std::get<G>(datRGB), std::get<B>(datRGB));
        const auto midGB1 = _mm512_unpackhi_epi8(std::get<G>(datRGB), std::get<B>(datRGB));
        const auto midRB0 = _mm512_unpacklo_epi8(std::get<R>(datRGB), std::get<B>(datRGB));
        const auto midRB1 = _mm512_unpackhi_epi8(std::get<R>(datRGB), std::get<B>(datRGB));

#define DP4x4(out, isy, a, b, lut) const auto out##0 = DP4<isy>(mid##a##0, mid##b##0, lut##a, lut##b), out##1 = DP4<isy>(mid##a##1, mid##b##1, lut##a, lut##b)
        DP4x4(lumi, true, RG, GB, LutY);
        DP4x4(cb,  false, RB, GB, LutCb);
        DP4x4(cr,  false, RG, RB, LutCr);
#undef DP4x4

        auto outY = _mm512_packus_epi16(lumi0, lumi1);
        if (needAddY)
            outY = _mm512_add_epi8(outY, _mm512_set1_epi8(16));
        auto outCb = _mm512_packus_epi16(cb0, cb1);
        auto outCr = _mm512_packus_epi16(cr0, cr1);
        const auto addToU8 = _mm512_set1_epi8(static_cast<int8_t>(128));
        outCb = _mm512_add_epi8(outCb, addToU8);
        outCr = _mm512_add_epi8(outCr, addToU8);

        Combine8x3_512::DoStore(dst, outY, outCb, outCr);
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, AVX512_I8)
{
    if (count >= RGB8ToYUV8_I8_AVX512_Base::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8_AVX512<0, 1, 2> proc(reinterpret_cast<const int16_t*>(RGB8ToYCC8LUTI8x4[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I8>(dest, src, count, mval);
}

template<typename T>
struct YUV8ToRGB8_I16_512_Base
{
    static constexpr size_t M = 192, N = 192, K = 64;
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY, bool isRGBFull, bool isRGB) const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        auto [datY, datCb, datCr] = Extract8x3_512::DoLoadMixed(src);
        const auto u8_16 = _mm512_set1_epi8(16), u8_128 = _mm512_set1_epi8(static_cast<char>(128));
        
        if (needAddY)
            datY = _mm512_sub_epi8(datY, u8_16);
        datCb = _mm512_sub_epi8(datCb, u8_128);
        datCr = _mm512_sub_epi8(datCr, u8_128);

#define Shuf82(n,x,a) const auto n = _mm512_shuffle_epi8(dat##x, mm512_set_lane(epi8, -1, a+21, -1, a+18, -1, a+15, -1, a+12, -1, a+9, -1, a+6, -1, a+3, -1, a))
        Shuf82(y_0, Y,  0);
        Shuf82(y_1, Y, 24);
#undef  Shuf82
        // (7 + x) - 15
        const auto y0 = _mm512_mulhrs_epi16(_mm512_slli_epi16(y_0, 7), self.MulY);
        const auto y1 = _mm512_mulhrs_epi16(_mm512_slli_epi16(y_1, 7), self.MulY);
#define Shuf82(n,x,a) const auto n = _mm512_shuffle_epi8(dat##x, mm512_set_lane(epi8, a+42, -1, a+36, -1, a+30, -1, a+24, -1, a+18, -1, a+12, -1, a+6, -1, a, -1))
        Shuf82(cb_e, Cb, 1);
        Shuf82(cb_o, Cb, 4);
        Shuf82(cr_e, Cr, 2);
        Shuf82(cr_o, Cr, 5);
#undef  Shuf82
        const auto cbr0e = _mm512_unpacklo_epi16(cb_e, cr_e); // 0246
        const auto cbr0o = _mm512_unpacklo_epi16(cb_o, cr_o); // 1357
        const auto cbr1e = _mm512_unpackhi_epi16(cb_e, cr_e); // 8ace
        const auto cbr1o = _mm512_unpackhi_epi16(cb_o, cr_o); // 9bdf

#define CalcRGB(x) const auto [r##x, g##x, b##x] = self.Calc(y##x, cbr##x##e, cbr##x##o)
        CalcRGB(0);
        CalcRGB(1);
#undef  CalcRGB

#define Combine4(x) auto x = _mm512_packus_epi16(x##0, x##1)
        Combine4(r);
        Combine4(g);
        Combine4(b);
#undef  Combine4
        if (!isRGBFull)
        {
            const auto rgbMin = u8_16, rgbMax = _mm512_set1_epi8(static_cast<char>(235));
#define ClampRGB(x) x = _mm512_min_epu8(_mm512_max_epu8(x, rgbMin), rgbMax)
            ClampRGB(r);
            ClampRGB(g);
            ClampRGB(b);
#undef  ClampRGB
        }
        if (isRGB)
            Combine8x3_512::DoStore(dst, r, g, b);
        else
            Combine8x3_512::DoStore(dst, b, g, r);
    }
};
struct YUV8ToRGB8_I16_512 : public YUV8ToRGB8_I16_512_Base<YUV8ToRGB8_I16_512>
{
    // Y,0,Bcb,Rcr,Gcb,Gcr
    __m512i MulY, MulR, MulB, MulG;
    YUV8ToRGB8_I16_512(const uint32_t* lut) noexcept : 
        MulY(_mm512_set1_epi16(static_cast<int16_t>(lut[0] >> 1))),
        MulR(_mm512_set1_epi32(static_cast<int32_t>(lut[1] & 0xffff0000u))),
        MulB(_mm512_set1_epi32(static_cast<int32_t>(lut[1] & 0x0000ffffu))),
        MulG(_mm512_set1_epi32(static_cast<int32_t>(lut[2])))
    { }
    forceinline std::tuple<__m512i, __m512i, __m512i> Calc(const __m512i& y5, const __m512i& cbrE, const __m512i& cbrO) const noexcept
    {
        const auto rE = _mm512_madd_epi16(cbrE, MulR), rO = _mm512_madd_epi16(cbrO, MulR);
        const auto gE = _mm512_madd_epi16(cbrE, MulG), gO = _mm512_madd_epi16(cbrO, MulG);
        const auto bE = _mm512_madd_epi16(cbrE, MulB), bO = _mm512_madd_epi16(cbrO, MulB);
        // (8 + 13) - 16 = 5
        const auto r5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(rE, 16), rO);
        const auto g5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(gE, 16), gO);
        const auto b5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(bE, 16), bO);
        const auto y = _mm512_add_epi16(y5, _mm512_set1_epi16(16)); // add rounding
        const auto r = _mm512_srai_epi16(_mm512_add_epi16(r5, y), 5), b = _mm512_srai_epi16(_mm512_add_epi16(b5, y), 5);
        const auto g = _mm512_srai_epi16(_mm512_sub_epi16(y, g5), 5);
        return { r, g, b };
    }
};
struct YUV8ToRGB8_I16_2_512 : public YUV8ToRGB8_I16_512_Base<YUV8ToRGB8_I16_2_512>
{
    // Y,0,Bcb,Rcr,Gcb,Gcr
    __m512i MulY, MulRB, MulG;
    YUV8ToRGB8_I16_2_512(const uint32_t* lut) noexcept : 
        MulY (_mm512_set1_epi16(static_cast<int16_t>(lut[0]))),
        MulRB(_mm512_set1_epi32(static_cast<int32_t>(lut[1]))),
        MulG (_mm512_set1_epi32(static_cast<int32_t>(lut[2])))
    { }
    forceinline std::tuple<__m512i, __m512i, __m512i> Calc(const __m512i& y6, const __m512i& cbrE, const __m512i& cbrO) const noexcept
    {
        // (8 + 13) - 16 = 5
        const auto gE = _mm512_madd_epi16(cbrE, MulG), gO = _mm512_madd_epi16(cbrO, MulG);
        // (8 + 13) - 15 = 6
        const auto rbE = _mm512_mulhrs_epi16(cbrE, MulRB), rbO = _mm512_mulhrs_epi16(cbrO, MulRB);
        const auto r6 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(rbE, 16), rbO);
        const auto b6 = _mm512_mask_blend_epi16(0x55555555u, _mm512_slli_epi32(rbO, 16), rbE);
        const auto g5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(gE,  16), gO);
        const auto yrb = _mm512_add_epi16(y6, _mm512_set1_epi16(32)); // add rounding
        const auto r = _mm512_srai_epi16(_mm512_add_epi16(r6, yrb), 6), b = _mm512_srai_epi16(_mm512_add_epi16(b6, yrb), 6);
        const auto g = _mm512_srai_epi16(_mm512_sub_epi16(_mm512_srai_epi16(yrb, 1), g5), 5);
        return { r, g, b };
    }
};
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX512_I16)
{
    if (count >= YUV8ToRGB8_I16_512::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_512 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX512_I16_2)
{
    if (count >= YUV8ToRGB8_I16_2_512::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_2_512 proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I16_2>(dest, src, count, mval);
}

DEFINE_FASTPATH_METHOD(G8ToGA8, AVX512BW)
{
    ProcessLOOP4<G2GA8_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX512BW)
{
    ProcessLOOP4<G2RGB8_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512BW)
{
    ProcessLOOP4<G2RGBA8_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX512BW)
{
    ProcessLOOP4<GA2RGBA8_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToGA16, AVX512BW)
{
    ProcessLOOP4<G2GA16_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G16ToRGB16, AVX512BW)
{
    ProcessLOOP4<G2RGB16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToRGBA16, AVX512BW)
{
    ProcessLOOP4<G2RGBA16_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA16ToRGB16, AVX512BW)
{
    ProcessLOOP4<GA2RGB16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA16ToRGBA16, AVX512BW)
{
    ProcessLOOP4<GA2RGBA16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, AVX512BW)
{
    ProcessLOOP4<G2GAf_512, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, AVX512BW)
{
    ProcessLOOP4<G2RGBf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, AVX512BW)
{
    ProcessLOOP4<G2RGBAf_512, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, AVX512BW)
{
    ProcessLOOP4<GA2RGBf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, AVX512BW)
{
    ProcessLOOP4<GA2RGBAf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToRGBA16, AVX512BW)
{
    ProcessLOOP4<RGB16_3To4_512<0, 1, 2>, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR16ToRGBA16, AVX512BW)
{
    ProcessLOOP4<RGB16_3To4_512<2, 1, 0>, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA16ToRGB16, AVX512BW)
{
    ProcessLOOP4<RGB16_4To3_512<0, 1, 2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGR16, AVX512BW)
{
    ProcessLOOP4<RGB16_4To3_512<2, 1, 0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, AVX512BW)
{
    ProcessLOOP4<RGBf_3To4_512<0, 1, 2>, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, AVX512BW)
{
    ProcessLOOP4<RGBf_3To4_512<2, 1, 0>, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, AVX512BW)
{
    ProcessLOOP4<RGBf_4To3_512<0, 1, 2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, AVX512BW)
{
    ProcessLOOP4<RGBf_4To3_512<2, 1, 0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToBGR16, AVX512BW)
{
    ProcessLOOP4<RGBToBGR16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA16ToBGRA16, AVX512BW)
{
    ProcessLOOP4<RGBAToBGRA16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, AVX512BW)
{
    ProcessLOOP4<RGBToBGRf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, AVX512BW)
{
    ProcessLOOP4<RGBAToBGRAf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToR16, AVX512BW)
{
    ProcessLOOP4<KeepChannel16_3_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToG16, AVX512BW)
{
    ProcessLOOP4<KeepChannel16_3_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB16ToB16, AVX512BW)
{
    ProcessLOOP4<KeepChannel16_3_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_4_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_4_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_4_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_4_1_512<3>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_3_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_3_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF_3_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512BW)
{
    ExtractLOOP4<2, Extract8x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512BW2)
{
    ExtractLOOP4<2, Extract8x2_512_2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, AVX512BW)
{
    ExtractLOOP4<3, Extract8x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512BW)
{
    ExtractLOOP4<4, Extract8x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512BW2)
{
    ExtractLOOP4<4, Extract8x4_512_2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x2, AVX512BW)
{
    ExtractLOOP4<2, Extract16x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x3, AVX512BW)
{
    ExtractLOOP4<3, Extract16x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract16x4, AVX512BW)
{
    ExtractLOOP4<4, Extract16x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x2, AVX512BW)
{
    ExtractLOOP4<2, Extract32x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x3, AVX512BW)
{
    ExtractLOOP4<3, Extract32x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract32x4, AVX512BW)
{
    ExtractLOOP4<4, Extract32x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x2, AVX512BW)
{
    CombineLOOP4<2, Combine8x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, AVX512BW)
{
    CombineLOOP4<3, Combine8x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, AVX512BW)
{
    CombineLOOP4<4, Combine8x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x2, AVX512BW)
{
    CombineLOOP4<2, Combine16x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x3, AVX512BW)
{
    CombineLOOP4<3, Combine16x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine16x4, AVX512BW)
{
    CombineLOOP4<4, Combine16x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, AVX512BW)
{
    CombineLOOP4<2, Combine32x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, AVX512BW)
{
    CombineLOOP4<3, Combine32x3_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, AVX512BW)
{
    CombineLOOP4<4, Combine32x4_512, &Func<AVX2>>(dest, src, count);
}

DEFINE_FASTPATH_METHOD(G8ToG16, AVX512BW)
{
    ProcessLOOP4<G8ToG16_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToG8, AVX512BW)
{
    ProcessLOOP4<G16ToG8_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<false, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGB8_256<false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<true, false, true>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<false, false, true>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<true, true, true>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<false, true, true>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<true, false, false>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, AVX512BW)
{
    ProcessLOOP4<RGB5x5ToRGBA8_512<false, false, false>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_512<false, true>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_512<true, true>, &Func<AVX2>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBf_512<false, false>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBf_512<true, false>, &Func<AVX2>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10ToRGBAf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_512<false, true, false>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_512<true, true, false>, &Func<AVX2>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10ToRGBAf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_512<false, false, false>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_512<true, false, false>, &Func<AVX2>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(RGB10A2ToRGBAf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_512<false, true, true>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_512<true, true, true>, &Func<AVX2>>(dest, src, count, mulVal);
}
DEFINE_FASTPATH_METHOD(BGR10A2ToRGBAf, AVX512BW)
{
    if (mulVal == 0)
        ProcessLOOP4<RGB10ToRGBAf_512<false, false, true>, &Func<AVX2>>(dest, src, count, mulVal);
    else
        ProcessLOOP4<RGB10ToRGBAf_512<true, false, true>, &Func<AVX2>>(dest, src, count, mulVal);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (defined(__AVX512VBMI__) || COMMON_COMPILER_MSVC)
#   pragma message("Compiling ColorConvert with AVX512-VBMI")
struct G2RGB8_512VBMI
{
    static constexpr size_t M = 64, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src);
        const auto shuffleMask0 = _mm512_set_epi8(
            21, 20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16,
            15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10,
            10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5,
             5,  4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,  0);
        const auto shuffleMask1 = _mm512_set_epi8(
            42, 42, 41, 41, 41, 40, 40, 40, 39, 39, 39, 38, 38, 38, 37, 37,
            37, 36, 36, 36, 35, 35, 35, 34, 34, 34, 33, 33, 33, 32, 32, 32,
            31, 31, 31, 30, 30, 30, 29, 29, 29, 28, 28, 28, 27, 27, 27, 26,
            26, 26, 25, 25, 25, 24, 24, 24, 23, 23, 23, 22, 22, 22, 21, 21);
        const auto shuffleMask2 = _mm512_set_epi8(
            63, 63, 63, 62, 62, 62, 61, 61, 61, 60, 60, 60, 59, 59, 59, 58,
            58, 58, 57, 57, 57, 56, 56, 56, 55, 55, 55, 54, 54, 54, 53, 53,
            53, 52, 52, 52, 51, 51, 51, 50, 50, 50, 49, 49, 49, 48, 48, 48,
            47, 47, 47, 46, 46, 46, 45, 45, 45, 44, 44, 44, 43, 43, 43, 42);
        const auto out0 = _mm512_permutexvar_epi8(shuffleMask0, dat);
        const auto out1 = _mm512_permutexvar_epi8(shuffleMask1, dat);
        const auto out2 = _mm512_permutexvar_epi8(shuffleMask2, dat);
        _mm512_storeu_si512(dst +   0, out0);
        _mm512_storeu_si512(dst +  64, out1);
        _mm512_storeu_si512(dst + 128, out2);
    }
};
struct G2RGBA8_512VBMI
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2RGBA8_512VBMI(std::byte alpha) noexcept : Alpha(_mm512_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(alpha) << 24))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto dat = _mm512_loadu_epi8(src);

        const auto shuffleMask0 = _mm512_set_epi8(
            15, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 12,
            11, 11, 11, 11, 10, 10, 10, 10,  9,  9,  9,  9,  8,  8,  8,  8,
             7,  7,  7,  7,  6,  6,  6,  6,  5,  5,  5,  5,  4,  4,  4,  4,
             3,  3,  3,  3,  2,  2,  2,  2,  1,  1,  1,  1,  0,  0,  0,  0);
        const auto shuffleMask1 = _mm512_set_epi8(
            31, 31, 31, 31, 30, 30, 30, 30, 29, 29, 29, 29, 28, 28, 28, 28,
            27, 27, 27, 27, 26, 26, 26, 26, 25, 25, 25, 25, 24, 24, 24, 24,
            23, 23, 23, 23, 22, 22, 22, 22, 21, 21, 21, 21, 20, 20, 20, 20,
            19, 19, 19, 19, 18, 18, 18, 18, 17, 17, 17, 17, 16, 16, 16, 16);
        const auto shuffleMask2 = _mm512_set_epi8(
            47, 47, 47, 47, 46, 46, 46, 46, 45, 45, 45, 45, 44, 44, 44, 44,
            43, 43, 43, 43, 42, 42, 42, 42, 41, 41, 41, 41, 40, 40, 40, 40,
            39, 39, 39, 39, 38, 38, 38, 38, 37, 37, 37, 37, 36, 36, 36, 36,
            35, 35, 35, 35, 34, 34, 34, 34, 33, 33, 33, 33, 32, 32, 32, 32);
        const auto shuffleMask3 = _mm512_set_epi8(
            63, 63, 63, 63, 62, 62, 62, 62, 61, 61, 61, 61, 60, 60, 60, 60,
            59, 59, 59, 59, 58, 58, 58, 58, 57, 57, 57, 57, 56, 56, 56, 56,
            55, 55, 55, 55, 54, 54, 54, 54, 53, 53, 53, 53, 52, 52, 52, 52,
            51, 51, 51, 51, 50, 50, 50, 50, 49, 49, 49, 49, 48, 48, 48, 48);
        const auto out0 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask0, dat);
        const auto out1 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask1, dat);
        const auto out2 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask2, dat);
        const auto out3 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask3, dat);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct GA2RGBA8_512VBMI
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src);
        const auto shuffleMaskLo = _mm512_set_epi8(
            31, 30, 30, 30, 29, 28, 28, 28, 27, 26, 26, 26, 25, 24, 24, 24,
            23, 22, 22, 22, 21, 20, 20, 20, 19, 18, 18, 18, 17, 16, 16, 16,
            15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10,  9,  8,  8,  8,
             7,  6,  6,  6,  5,  4,  4,  4,  3,  2,  2,  2,  1,  0,  0,  0);
        const auto shuffleMaskHi = _mm512_set_epi8(
            63, 62, 62, 62, 61, 60, 60, 60, 59, 58, 58, 58, 57, 56, 56, 56,
            55, 54, 54, 54, 53, 52, 52, 52, 51, 50, 50, 50, 49, 48, 48, 48,
            47, 46, 46, 46, 45, 44, 44, 44, 43, 42, 42, 42, 41, 40, 40, 40,
            39, 38, 38, 38, 37, 36, 36, 36, 35, 34, 34, 34, 33, 32, 32, 32);
        const auto out0 = _mm512_permutexvar_epi8(shuffleMaskLo, dat);
        const auto out1 = _mm512_permutexvar_epi8(shuffleMaskHi, dat);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct RGB2RGBA8_512VBMI
{
    static constexpr size_t M = 192, N = 64, K = 64;
    __m512i Alpha;
    forceinline RGB2RGBA8_512VBMI(std::byte alpha) noexcept : Alpha(_mm512_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(alpha) << 24))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto dat0 = _mm512_loadu_epi8(src +   0);
        const auto dat1 = _mm512_loadu_epi8(src +  64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        const auto shuffleMask0 = _mm512_set_epi8(
            47, 47, 46, 45, 44, 44, 43, 42, 41, 41, 40, 39, 38, 38, 37, 36,
            35, 35, 34, 33, 32, 32, 31, 30, 29, 29, 28, 27, 26, 26, 25, 24,
            23, 23, 22, 21, 20, 20, 19, 18, 17, 17, 16, 15, 14, 14, 13, 12,
            11, 11, 10,  9,  8,  8,  7,  6,  5,  5,  4,  3,  2,  2,  1,  0);
        const auto shuffleMask1 = _mm512_set_epi8(
            31, 31, 30, 29, 28, 28, 27, 26, 25, 25, 24, 23, 22, 22, 21, 20,
            19, 19, 18, 17, 16, 16, 15, 14, 13, 13, 12, 11, 10, 10,  9,  8,
             7,  7,  6,  5,  4,  4,  3,  2,  1,  1,  0, 63, 62, 62, 61, 60,
            59, 59, 58, 57, 56, 56, 55, 54, 53, 53, 52, 51, 50, 50, 49, 48);
        const auto shuffleMask2 = _mm512_set_epi8(
            15, 15, 14, 13, 12, 12, 11, 10,  9,  9,  8,  7,  6,  6,  5,  4,
             3,  3,  2,  1,  0,  0, 63, 62, 61, 61, 60, 59, 58, 58, 57, 56,
            55, 55, 54, 53, 52, 52, 51, 50, 49, 49, 48, 47, 46, 46, 45, 44,
            43, 43, 42, 41, 40, 40, 39, 38, 37, 37, 36, 35, 34, 34, 33, 32);
        const auto shuffleMask3 = _mm512_set_epi8(
            63, 63, 62, 61, 60, 60, 59, 58, 57, 57, 56, 55, 54, 54, 53, 52,
            51, 51, 50, 49, 48, 48, 47, 46, 45, 45, 44, 43, 42, 42, 41, 40,
            39, 39, 38, 37, 36, 36, 35, 34, 33, 33, 32, 31, 30, 30, 29, 28,
            27, 27, 26, 25, 24, 24, 23, 22, 21, 21, 20, 19, 18, 18, 17, 16);
        
        const auto out0 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask0, dat0);
        const auto dat01 = _mm512_mask_blend_epi64(_cvtu32_mask8(0b00111111), dat0, dat1);
        const auto out1 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask1, dat01);
        const auto dat12 = _mm512_mask_blend_epi64(_cvtu32_mask8(0b00000011), dat1, dat2);
        const auto out2 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask2, dat12);
        const auto out3 = _mm512_mask_permutexvar_epi8(Alpha, mask, shuffleMask3, dat2);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
struct RGBA2RGB8_512VBMI
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto shuffleMask = _mm512_set_epi8(
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            62, 61, 60, 58, 57, 56, 54, 53, 52, 50, 49, 48, 46, 45, 44, 42,
            41, 40, 38, 37, 36, 34, 33, 32, 30, 29, 28, 26, 25, 24, 22, 21,
            20, 18, 17, 16, 14, 13, 12, 10,  9,  8,  6,  5,  4,  2,  1,  0);
        const auto dat = _mm512_loadu_epi8(src);
        const auto val = _mm512_permutexvar_epi8(shuffleMask, dat);
        const U8x32 out0 = _mm512_castsi512_si256(val);
        out0.Save(dst);
        const U8x16 out1 = _mm512_extracti32x4_epi32(val, 2);
        out1.Save(dst + 32);
    }
};
struct RGBToBGR8_512VBMI
{
    static constexpr size_t M = 192, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src +   0);
        const auto dat1 = _mm512_loadu_epi8(src +  64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);
        const auto mix02 = _mm512_mask_blend_epi64(0xf0, dat2, dat0);
        std::atomic_signal_fence(std::memory_order_acquire);

        const auto shuffleMask0 = _mm512_set_epi8(
            65, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 
            45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 
            31, 32, 27, 28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 
            17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2);
        const auto out0 = _mm512_permutex2var_epi8(dat0, shuffleMask0, dat1);

        const auto shuffleMask1 = _mm512_set_epi8(
            63, 64, 59, 60, 61, 56, 57, 58, 53, 54, 55, 50, 51, 52,  47, 48, 
            49, 44, 45, 46, 41, 42, 43, 38, 39, 40, 35, 36, 37, 32,  33, 34, 
            29, 30, 31, 26, 27, 28, 23, 24, 25, 20, 21, 22, 17, 18,  19, 14, 
            15, 16, 11, 12, 13,  8,  9, 10,  5,  6,  7,  2,  3,  4, 127,  0);
        const auto out1 = _mm512_permutex2var_epi8(dat1, shuffleMask1, mix02);

        const auto shuffleMask2 = _mm512_set_epi8(
            61, 62, 63, 58, 59, 60, 55, 56, 57, 52, 53, 54, 49, 50, 51,  46, 
            47, 48, 43, 44, 45, 40, 41, 42, 37, 38, 39, 34, 35, 36, 31,  32, 
            33, 28, 29, 30, 25, 26, 27, 22, 23, 24, 19, 20, 21, 16, 17,  18, 
            13, 14, 15, 10, 11, 12,  7,  8,  9,  4,  5,  6,  1,  2,  3, 126);
        const auto out2 = _mm512_permutex2var_epi8(dat2, shuffleMask2, dat1);

        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
};
struct RGBAToBGRA8_512VBMI
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src +  0);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        const auto shuffleMask = _mm512_set_epi8(
            63, 60, 61, 62, 59, 56, 57, 58, 55, 52, 53, 54, 51, 48, 49, 50,
            47, 44, 45, 46, 43, 40, 41, 42, 39, 36, 37, 38, 35, 32, 33, 34,
            31, 28, 29, 30, 27, 24, 25, 26, 23, 20, 21, 22, 19, 16, 17, 18,
            15, 12, 13, 14, 11,  8,  9, 10,  7,  4,  5,  6,  3,  0,  1,  2);

        const auto out0 = _mm512_permutexvar_epi8(shuffleMask, dat0);
        const auto out1 = _mm512_permutexvar_epi8(shuffleMask, dat1);

        _mm512_storeu_epi32(dst +  0, out0);
        _mm512_storeu_epi32(dst + 16, out1);
    }
};
template<uint8_t Idx>
struct KeepChannel8_3_1_512VBMI
{
    static constexpr size_t M = 192, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        const auto shufMask01 = _mm512_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,                    static_cast<int8_t>(Idx + 126), Idx + 123, Idx + 120, Idx + 117, Idx + 114, Idx + 111, Idx + 108, Idx + 105, Idx + 102, Idx +  99, Idx +  96,
            Idx +  93, Idx +  90, Idx +  87, Idx +  84, Idx +  81, Idx +  78, Idx +  75, Idx +  72, Idx +  69, Idx +  66, Idx +  63, Idx +  60, Idx +  57, Idx +  54, Idx +  51, Idx +  48,
            Idx +  45, Idx +  42, Idx +  39, Idx +  36, Idx +  33, Idx +  30, Idx +  27, Idx +  24, Idx +  21, Idx +  18, Idx +  15, Idx +  12, Idx +   9, Idx +   6, Idx +   3, Idx +   0);
        const auto shufMask2 = _mm512_set_epi8(
            Idx + 125, Idx + 122, Idx + 119, Idx + 116, Idx + 113, Idx + 110, Idx + 107, Idx + 104, Idx + 101, Idx +  98, Idx +  95, Idx +  92, Idx +  89, Idx +  86, Idx +  83, Idx +  80,
            Idx +  77, Idx +  74, Idx +  71, Idx +  68, Idx +  65, Idx +  62,         0,         0,         0,         0,         0,         0,         0,         0,         0,         0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        constexpr uint64_t Mask2 = uint64_t(-1) << (Idx == 2 ? 42 : 43);
        const auto insertMask2 = _cvtu64_mask64(Mask2);

        const auto mid01 = _mm512_permutex2var_epi8(dat0, shufMask01, dat1);
        const auto out = _mm512_mask_permutex2var_epi8(mid01, insertMask2, shufMask2, dat2);
        _mm512_storeu_epi8(dst, out);
    }
};
struct Extract8x2_512VBMI
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto shuffle0Mask = _mm512_set_epi8(
            126, 124, 122, 120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96,
             94,  92,  90,  88,  86,  84,  82,  80,  78,  76,  74,  72,  70,  68, 66, 64,
             62,  60,  58,  56,  54,  52,  50,  48,  46,  44,  42,  40,  38,  36, 34, 32,
             30,  28,  26,  24,  22,  20,  18,  16,  14,  12,  10,   8,   6,   4,  2,  0);
        const auto out0 = _mm512_permutex2var_epi8(dat0, shuffle0Mask, dat1);

        const auto shuffle1Mask = _mm512_set_epi8(
            127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 107, 105, 103, 101, 99, 97,
             95,  93,  91,  89,  87,  85,  83,  81,  79,  77,  75,  73,  71,  69, 67, 65,
             63,  61,  59,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37, 35, 33,
             31,  29,  27,  25,  23,  21,  19,  17,  15,  13,  11,   9,   7,   5,  3,  1);
        const auto out1 = _mm512_permutex2var_epi8(dat0, shuffle1Mask, dat1);

        _mm512_storeu_epi8(dst[0], out0);
        _mm512_storeu_epi8(dst[1], out1);
    }
};
struct Extract8x3_512VBMI
{
    static constexpr size_t M = 192, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        const auto maskRG = _cvtu64_mask64(UINT64_MAX << 43);
        const auto maskB  = _cvtu64_mask64(UINT64_MAX << 42);

        const auto shuffle0Mask = _mm512_set_epi8(
            61, 58, 55, 52, 49,  46,  43,  40,  37,  34,  31,  28,  25,  22, 19, 16,
            13, 10,  7,  4,  1, 126, 123, 120, 117, 114, 111, 108, 105, 102, 99, 96,
            93, 90, 87, 84, 81,  78,  75,  72,  69,  66,  63,  60,  57,  54, 51, 48,
            45, 42, 39, 36, 33,  30,  27,  24,  21,  18,  15,  12,   9,   6,  3,  0);
        const auto val0 = _mm512_permutex2var_epi8(dat0, shuffle0Mask, dat1);
        const auto out0 = _mm512_mask_permutexvar_epi8(val0, maskRG, shuffle0Mask, dat2);

        const auto shuffle1Mask = _mm512_set_epi8(
            62, 59, 56, 53, 50,  47,  44,  41,  38,  35,  32,  29,  26,  23,  20, 17,
            14, 11,  8,  5,  2, 127, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97,
            94, 91, 88, 85, 82,  79,  76,  73,  70,  67,  64,  61,  58,  55,  52, 49,
            46, 43, 40, 37, 34,  31,  28,  25,  22,  19,  16,  13,  10,   7,   4,  1);
        const auto val1 = _mm512_permutex2var_epi8(dat0, shuffle1Mask, dat1);
        const auto out1 = _mm512_mask_permutexvar_epi8(val1, maskRG, shuffle1Mask, dat2);

        const auto shuffle2Mask = _mm512_set_epi8(
            63, 60, 57, 54, 51, 48,  45,  42,  39,  36,  33,  30,  27,  24,  21, 18,
            15, 12,  9,  6,  3,  0, 125, 122, 119, 116, 113, 110, 107, 104, 101, 98,
            95, 92, 89, 86, 83, 80,  77,  74,  71,  68,  65,  62,  59,  56,  53, 50,
            47, 44, 41, 38, 35, 32,  29,  26,  23,  20,  17,  14,  11,   8,   5,  2);
        const auto val2 = _mm512_permutex2var_epi8(dat0, shuffle2Mask, dat1);
        const auto out2 = _mm512_mask_permutexvar_epi8(val2, maskB, shuffle2Mask, dat2);

        _mm512_storeu_epi8(dst[0], out0);
        _mm512_storeu_epi8(dst[1], out1);
        _mm512_storeu_epi8(dst[2], out2);
    }
};
struct Extract8x4_512VBMI
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        const auto shuffle0Mask = _mm512_set_epi8(
            125, 121, 117, 113, 109, 105, 101, 97, 93, 89, 85, 81, 77, 73, 69, 65,
             61,  57,  53,  49,  45,  41,  37, 33, 29, 25, 21, 17, 13,  9,  5,  1,
            124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64,
             60,  56,  52,  48,  44,  40,  36, 32, 28, 24, 20, 16, 12,  8,  4,  0);
        const auto val0 = _mm512_permutex2var_epi8(dat0, shuffle0Mask, dat1);

        const auto shuffle1Mask = _mm512_set_epi8(
            127, 123, 119, 115, 111, 107, 103, 99, 95, 91, 87, 83, 79, 75, 71, 67,
             63,  59,  55,  51,  47,  43,  39, 35, 31, 27, 23, 19, 15, 11,  7,  3,
            126, 122, 118, 114, 110, 106, 102, 98, 94, 90, 86, 82, 78, 74, 70, 66,
             62,  58,  54,  50,  46,  42,  38, 34, 30, 26, 22, 18, 14, 10,  6,  2);
        const auto val1 = _mm512_permutex2var_epi8(dat0, shuffle1Mask, dat1);

        const U8x32 out0 = _mm512_castsi512_si256(val0);
        const U8x32 out1 = _mm512_extracti64x4_epi64(val0, 1);
        const U8x32 out2 = _mm512_castsi512_si256(val1);
        const U8x32 out3 = _mm512_extracti64x4_epi64(val1, 1);
        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
struct Combine8x3_512VBMI
{
    static constexpr size_t M = 64, N = 192, K = 64;
    static forceinline void DoStore(uint8_t* __restrict dst, const __m512i& dat0, const __m512i& dat1, const __m512i& dat2) noexcept
    {
        const auto shuffleRGR0 = _mm512_set_epi8(
             21,  20,  84,  20,  19,  83,  19,  18,  82,  18,  17,  81,  17,  16,  80,  16,
             15,  79,  15,  14,  78,  14,  13,  77,  13,  12,  76,  12,  11,  75,  11,  10,
             74,  10,   9,  73,   9,   8,  72,   8,   7,  71,   7,   6,  70,   6,   5,  69,
              5,   4,  68,   4,   3,  67,   3,   2,  66,   2,   1,  65,   1,   0,  64,   0);
        const auto valRGR0 = _mm512_permutex2var_epi8(dat0, shuffleRGR0, dat1);
        const auto shuffleRGR1 = _mm512_set_epi8(
            106,  42,  41, 105,  41,  40, 104,  40,  39, 103,  39,  38, 102,  38,  37, 101,
             37,  36, 100,  36,  35,  99,  35,  34,  98,  34,  33,  97,  33,  32,  96,  32,
             31,  95,  31,  30,  94,  30,  29,  93,  29,  28,  92,  28,  27,  91,  27,  26,
             90,  26,  25,  89,  25,  24,  88,  24,  23,  87,  23,  22,  86,  22,  21,  85);
        const auto valRGR1 = _mm512_permutex2var_epi8(dat0, shuffleRGR1, dat1);
        const auto shuffleRGR2 = _mm512_set_epi8(
             63, 127,  63,  62, 126,  62,  61, 125,  61,  60, 124,  60,  59, 123,  59,  58,
            122,  58,  57, 121,  57,  56, 120,  56,  55, 119,  55,  54, 118,  54,  53, 117,
             53,  52, 116,  52,  51, 115,  51,  50, 114,  50,  49, 113,  49,  48, 112,  48,
             47, 111,  47,  46, 110,  46,  45, 109,  45,  44, 108,  44,  43, 107,  43,  42);
        const auto valRGR2 = _mm512_permutex2var_epi8(dat0, shuffleRGR2, dat1);

        const auto insertB0 = _mm512_set_epi8(
            63, 84, 61, 60, 83, 58, 57, 82, 55, 54, 81, 52, 51, 80, 49, 48,
            79, 46, 45, 78, 43, 42, 77, 40, 39, 76, 37, 36, 75, 34, 33, 74,
            31, 30, 73, 28, 27, 72, 25, 24, 71, 22, 21, 70, 19, 18, 69, 16,
            15, 68, 13, 12, 67, 10,  9, 66,  7,  6, 65,  4,  3, 64,  1,  0);
        const auto out0 = _mm512_permutex2var_epi8(valRGR0, insertB0, dat2);
        const auto insertB1 = _mm512_set_epi8(
             63,  62, 105,  60,  59, 104,  57,  56, 103,  54,  53, 102,  51,  50, 101,  48,
             47, 100,  45,  44,  99,  42,  41,  98,  39,  38,  97,  36,  35,  96,  33,  32,
             95,  30,  29,  94,  27,  26,  93,  24,  23,  92,  21,  20,  91,  18,  17,  90,
             15,  14,  89,  12,  11,  88,   9,   8,  87,   6,   5,  86,   3,   2,  85,   0);
        const auto out1 = _mm512_permutex2var_epi8(valRGR1, insertB1, dat2);
        const auto insertB2 = _mm512_set_epi8(
            127,  62,  61, 126,  59,  58, 125,  56,  55, 124,  53,  52, 123,  50,  49, 122,
             47,  46, 121,  44,  43, 120,  41,  40, 119,  38,  37, 118,  35,  34, 117,  32,
             31, 116,  29,  28, 115,  26,  25, 114,  23,  22, 113,  20,  19, 112,  17,  16,
            111,  14,  13, 110,  11,  10, 109,   8,   7, 108,   5,   4, 107,   2,   1, 106);
        const auto out2 = _mm512_permutex2var_epi8(valRGR2, insertB2, dat2);
        
        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src[0]);
        const auto dat1 = _mm512_loadu_epi8(src[1]);
        const auto dat2 = _mm512_loadu_epi8(src[2]);
        DoStore(dst, dat0, dat1, dat2);
    }
};
struct G8ToG16_512VBMI
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src);

        const auto out0 = _mm512_permutexvar_epi8(_mm512_set_epi8(31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
            15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0), dat);
        const auto out1 = _mm512_permutexvar_epi8(_mm512_set_epi8(63, 63, 62, 62, 61, 61, 60, 60, 59, 59, 58, 58, 57, 57, 56, 56, 55, 55, 54, 54, 53, 53, 52, 52, 51, 51, 50, 50, 49, 49, 48, 48,
            47, 47, 46, 46, 45, 45, 44, 44, 43, 43, 42, 42, 41, 41, 40, 40, 39, 39, 38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 33, 33, 32, 32), dat);

        _mm512_storeu_epi16(dst +  0, out0);
        _mm512_storeu_epi16(dst + 32, out1);
    }
};
struct G16ToG8_512VBMI
{
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src);
        const auto dat1 = _mm512_loadu_epi16(src + 32);
        const auto muler = _mm512_set1_epi16(static_cast<short>(0xff01));

        const auto mid0 = _mm512_mulhi_epu16(dat0, muler);
        const auto mid1 = _mm512_mulhi_epu16(dat1, muler);
        const auto out = _mm512_permutex2var_epi8(mid0, _mm512_set_epi8(127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 107, 105, 103, 101, 99, 97, 95, 93, 91, 89, 87, 85, 83, 81, 79, 77, 75, 73, 71, 69, 67, 65,
            63, 61, 59, 57, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1), mid1);
        
        _mm512_storeu_epi8(dst + 0, out);
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_512VBMI : RGB5x5ToRGB8_512Base<Is555>
{
    static constexpr size_t M = 64, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src +  0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto [midR0, midG0, midB0] = this->RGB5x5ToRGB8Hi(dat0);
        const auto [midR1, midG1, midB1] = this->RGB5x5ToRGB8Hi(dat1);

        const auto shuffleToLo = mm512_set_lane(epi8, 15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1);
        const auto insertMask = _cvtu64_mask64(0x5555555555555555u);
        const auto midRB0 = _mm512_mask_shuffle_epi8(IsRGB ? midB0 : midR0, insertMask, IsRGB ? midR0 : midB0, shuffleToLo);
        const auto midRB1 = _mm512_mask_shuffle_epi8(IsRGB ? midB1 : midR1, insertMask, IsRGB ? midR1 : midB1, shuffleToLo);

        const auto shuffle0RGB = _mm512_set_epi8(
            42,  41, 105,  40,  39, 103,  38,  37, 101,  36,  35,  99,  34,  33,  97,  32,
            31,  95,  30,  29,  93,  28,  27,  91,  26,  25,  89,  24,  23,  87,  22,  21,
            85,  20,  19,  83,  18,  17,  81,  16,  15,  79,  14,  13,  77,  12,  11,  75,
            10,   9,  73,   8,   7,  71,   6,   5,  69,   4,   3,  67,   2,   1,  65,   0);
        const auto out0 = _mm512_permutex2var_epi8(midRB0, shuffle0RGB, midG0);

        const auto shuffle2RGB = _mm512_set_epi8(
             63, 127,  62,  61, 125,  60,  59, 123,  58,  57, 121,  56,  55, 119,  54,  53,
            117,  52,  51, 115,  50,  49, 113,  48,  47, 111,  46,  45, 109,  44,  43, 107,
             42,  41, 105,  40,  39, 103,  38,  37, 101,  36,  35,  99,  34,  33,  97,  32,
             31,  95,  30,  29,  93,  28,  27,  91,  26,  25,  89,  24,  23,  87,  22,  21);
        const auto out2 = _mm512_permutex2var_epi8(midRB1, shuffle2RGB, midG1);

        const auto midRB01 = _mm512_mask_blend_epi64(0b00001111, midRB0, midRB1);
        const auto midG01  = _mm512_mask_blend_epi64(0b00001111, midG0,  midG1);
        const auto shuffle1RGB = _mm512_set_epi8(
             85,  20,  19,  83,  18,  17,  81,  16,  15,  79,  14,  13,  77,  12,  11,  75,
             10,   9,  73,   8,   7,  71,   6,   5,  69,   4,   3,  67,   2,   1,  65,   0,
             63, 127,  62,  61, 125,  60,  59, 123,  58,  57, 121,  56,  55, 119,  54,  53,
            117,  52,  51, 115,  50,  49, 113,  48,  47, 111,  46,  45, 109,  44,  43, 107);
        const auto out1 = _mm512_permutex2var_epi8(midRB01, shuffle1RGB, midG01);

        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
};

template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_512LUTBase
{
    forceinline static std::array<__m512i, 3> RGB5x5ToRGB8(const __m512i& dat0, const __m512i& dat1) noexcept
    {
        const auto maskHi = _cvtu64_mask64(0xaaaaaaaaaaaaaaaau);
        const auto mask5bit = _mm512_set1_epi8(0x1f);
        const auto mask6bit = _mm512_set1_epi8(0x3f);

        __m512i val0RB, val1RB;
        if constexpr (IsRGB)
        {
            const auto maskRB = _mm512_set1_epi16(short(Is555 ? 0x7c1fu : 0xf81fu));
            const auto dat0_ = _mm512_and_si512(dat0, maskRB), dat1_ = _mm512_and_si512(dat1, maskRB);
            const auto val0B = _mm512_srli_epi16(dat0_, Is555 ? 2 : 3);
            const auto val1B = _mm512_srli_epi16(dat1_, Is555 ? 2 : 3);
            val0RB = _mm512_mask_blend_epi8(maskHi, dat0_, val0B);
            val1RB = _mm512_mask_blend_epi8(maskHi, dat1_, val1B);
        }
        else
        {
            const auto val0R = _mm512_slli_epi16(dat0, 8);
            const auto val1R = _mm512_slli_epi16(dat1, 8);
            const auto val0B = _mm512_srli_epi16(dat0, Is555 ? 10 : 11);
            const auto val1B = _mm512_srli_epi16(dat1, Is555 ? 10 : 11);
            val0RB = _mm512_ternarylogic_epi32(mask5bit, val0R, val0B, 0b11100000); // mask5bit & (val0R | val0B)
            val1RB = _mm512_ternarylogic_epi32(mask5bit, val1R, val1B, 0b11100000); // mask5bit & (val0R | val0B)
            //val0RB = _mm512_and_si512(_mm512_or_si512(val0R, val0B), mask5bit);
            //val1RB = _mm512_and_si512(_mm512_or_si512(val1R, val1B), mask5bit);
        }
        const auto val0G = _mm512_srli_epi16(dat0, 5);
        const auto val1G = _mm512_slli_epi16(dat1, 3);
        const auto valG = _mm512_and_si512(_mm512_mask_blend_epi8(maskHi, val0G, val1G), Is555 ? mask5bit : mask6bit);

        const auto lut5 = _mm512_zextsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(LUT5Bit.data())));
        const auto mid0RB = _mm512_permutexvar_epi8(val0RB, lut5);
        const auto mid1RB = _mm512_permutexvar_epi8(val1RB, lut5);

        __m512i midG;
        if constexpr (Is555)
        {
            midG = _mm512_permutexvar_epi8(valG, lut5);
        }
        else
        {
            const auto lut6 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(LUT6Bit.data()));
            midG = _mm512_permutexvar_epi8(valG, lut6);
        }

        return { mid0RB, mid1RB, midG };
    }
};
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_LUT_512VBMI : RGB5x5ToRGB8_512LUTBase<IsRGB, Is555>
{
    static constexpr size_t M = 64, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src + 0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto [midRB0, midRB1, midG] = this->RGB5x5ToRGB8(dat0, dat1);

        const auto shuffle0RGB = _mm512_set_epi8(
            42,  41, 104,  40,  39, 102,  38,  37, 100,  36,  35,  98,  34,  33,  96,  32,
            31,  94,  30,  29,  92,  28,  27,  90,  26,  25,  88,  24,  23,  86,  22,  21,
            84,  20,  19,  82,  18,  17,  80,  16,  15,  78,  14,  13,  76,  12,  11,  74,
            10,   9,  72,   8,   7,  70,   6,   5,  68,   4,   3,  66,   2,   1,  64,   0);
        const auto out0 = _mm512_permutex2var_epi8(midRB0, shuffle0RGB, midG);

        const auto midRB01 = _mm512_mask_blend_epi64(0b00001111, midRB0, midRB1);
        const auto shuffle1RGB = _mm512_set_epi8(
             85,  20,  19,  83,  18,  17,  81,  16,  15,  79,  14,  13,  77,  12,  11,  75,
             10,   9,  73,   8,   7,  71,   6,   5,  69,   4,   3,  67,   2,   1,  65,   0,
             63, 126,  62,  61, 124,  60,  59, 122,  58,  57, 120,  56,  55, 118,  54,  53,
            116,  52,  51, 114,  50,  49, 112,  48,  47, 110,  46,  45, 108,  44,  43, 106);
        const auto out1 = _mm512_permutex2var_epi8(midRB01, shuffle1RGB, midG);

        const auto shuffle2RGB = _mm512_set_epi8(
             63, 127,  62,  61, 125,  60,  59, 123,  58,  57, 121,  56,  55, 119,  54,  53,
            117,  52,  51, 115,  50,  49, 113,  48,  47, 111,  46,  45, 109,  44,  43, 107,
             42,  41, 105,  40,  39, 103,  38,  37, 101,  36,  35,  99,  34,  33,  97,  32,
             31,  95,  30,  29,  93,  28,  27,  91,  26,  25,  89,  24,  23,  87,  22,  21);
        const auto out2 = _mm512_permutex2var_epi8(midRB1, shuffle2RGB, midG);

        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
};
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_LUT_512VBMI : RGB5x5ToRGB8_512LUTBase<IsRGB, Is555>
{
    static_assert(Is555 || !HasAlpha);
    static constexpr size_t M = 64, N = 64, K = 64;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi16(src + 0);
        const auto dat1 = _mm512_loadu_epi16(src + 32);

        const auto [midRB0, midRB1, midG] = this->RGB5x5ToRGB8(dat0, dat1);

        __m512i out[4];
        if constexpr (HasAlpha)
        {
            const auto midA0 = _mm512_srai_epi16(dat0, 7), midA1 = _mm512_srai_epi16(dat1, 7);
            const auto maskHi = _cvtu64_mask64(0xaaaaaaaaaaaaaaaau);
            const auto midGA0 = _mm512_mask_blend_epi8(maskHi, midG, midA0);
            const auto midGA1 = _mm512_mask_blend_epi8(maskHi, _mm512_srli_epi16(midG, 8), midA1);
            const auto shuffleLo = _mm512_set_epi8(
                95, 31, 94, 30, 93, 29, 92, 28, 91, 27, 90, 26, 89, 25, 88, 24,
                87, 23, 86, 22, 85, 21, 84, 20, 83, 19, 82, 18, 81, 17, 80, 16,
                79, 15, 78, 14, 77, 13, 76, 12, 75, 11, 74, 10, 73,  9, 72,  8,
                71,  7, 70,  6, 69,  5, 68,  4, 67,  3, 66,  2, 65,  1, 64,  0);
            const auto shuffleHi = _mm512_set_epi8(
                127, 63, 126, 62, 125, 61, 124, 60, 123, 59, 122, 58, 121, 57, 120, 56,
                119, 55, 118, 54, 117, 53, 116, 52, 115, 51, 114, 50, 113, 49, 112, 48,
                111, 47, 110, 46, 109, 45, 108, 44, 107, 43, 106, 42, 105, 41, 104, 40,
                103, 39, 102, 38, 101, 37, 100, 36,  99, 35,  66, 34,  97, 33,  96, 32);
            out[0] = _mm512_permutex2var_epi8(midRB0, shuffleLo, midGA0);
            out[1] = _mm512_permutex2var_epi8(midRB0, shuffleHi, midGA0);
            out[2] = _mm512_permutex2var_epi8(midRB1, shuffleLo, midGA1);
            out[3] = _mm512_permutex2var_epi8(midRB1, shuffleHi, midGA1);
        }
        else
        {
            const auto maskAlpha = _cvtu64_mask64(0x7777777777777777u);
            const auto shuffleLo = _mm512_set_epi8(
                -1, 31, 94, 30, -1, 29, 92, 28, -1, 27, 90, 26, -1, 25, 88, 24,
                -1, 23, 86, 22, -1, 21, 84, 20, -1, 19, 82, 18, -1, 17, 80, 16,
                -1, 15, 78, 14, -1, 13, 76, 12, -1, 11, 74, 10, -1,  9, 72,  8,
                -1,  7, 70,  6, -1,  5, 68,  4, -1,  3, 66,  2, -1,  1, 64,  0);
            const auto shuffleHi = _mm512_set_epi8(
                -1, 63, 126, 62, -1, 61, 124, 60, -1, 59, 122, 58, -1, 57, 120, 56,
                -1, 55, 118, 54, -1, 53, 116, 52, -1, 51, 114, 50, -1, 49, 112, 48,
                -1, 47, 110, 46, -1, 45, 108, 44, -1, 43, 106, 42, -1, 41, 104, 40,
                -1, 39, 102, 38, -1, 37, 100, 36, -1, 35,  66, 34, -1, 33,  96, 32);
            out[0] = _mm512_mask2_permutex2var_epi8(midRB0, shuffleLo, maskAlpha, midG);
            out[1] = _mm512_mask2_permutex2var_epi8(midRB0, shuffleHi, maskAlpha, midG);
            const auto midG1 = _mm512_srli_epi16(midG, 8);
            out[2] = _mm512_mask2_permutex2var_epi8(midRB1, shuffleLo, maskAlpha, midG1);
            out[3] = _mm512_mask2_permutex2var_epi8(midRB1, shuffleHi, maskAlpha, midG1);
        }
        _mm512_storeu_epi32(dst +  0, out[0]);
        _mm512_storeu_epi32(dst + 16, out[1]);
        _mm512_storeu_epi32(dst + 32, out[2]);
        _mm512_storeu_epi32(dst + 48, out[3]);
    }
};

struct RGB8ToYUV8_I16_AVX512VBMI_Base
{
    static constexpr size_t M = 192, N = 192, K = 64;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
#define RB(i) R + i, 0, B + i, 0
        alignas(64) static inline constexpr uint8_t ShufRB[] = { RB( 0), RB( 3), RB( 6), RB( 9), RB(12), RB(15), RB(18), RB(21), RB(24), RB(27), RB(30), RB(33), RB(36), RB(39), RB(42), RB(45),
            RB(48), RB(51), RB(54), RB(57), RB(60), RB(63), RB(66), RB(69), RB(72), RB(75), RB(78), RB(81), RB(84), RB(87), RB(90), RB(93),
            RB(32), RB(35), RB(38), RB(41), RB(44), RB(47), RB(50), RB(53), RB(56), RB(59), RB(62), RB(65), RB(68), RB(71), RB(74), RB(77),
            RB(16), RB(19), RB(22), RB(25), RB(28), RB(31), RB(34), RB(37), RB(40), RB(43), RB(46), RB(49), RB(52), RB(55), RB(58), RB(61) };
#undef RB
#define G4(i) G + i, 0, G + i + 3, 0, G + i + 6, 0, G + i + 9, 0 
        alignas(64) static inline constexpr uint8_t ShufG[] = { G4(0), G4(12), G4(24), G4(36), G4(48), G4(60), G4(72), G4(84), G4(32), G4(44), G4(56), G4(68), G4(80), G4(92), G4(104), G4(116) };
#undef G4
    };
    const __m512i ShufRB0, ShufRB1, ShufRB2, ShufRB3, ShufG0, ShufG1;
    __m512i LutYRB, LutYG, LutCbRB, LutCbG, LutCrRB, LutCrG;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I16_AVX512VBMI_Base(const I16x8& lut_, RGBOrder<R, G, B>) noexcept :
        ShufRB0(_mm512_loadu_si512(Shuffler<R, G, B>::ShufRB + 0)),
        ShufRB1(_mm512_loadu_si512(Shuffler<R, G, B>::ShufRB + 64)),
        ShufRB2(_mm512_loadu_si512(Shuffler<R, G, B>::ShufRB + 128)),
        ShufRB3(_mm512_loadu_si512(Shuffler<R, G, B>::ShufRB + 192)),
        ShufG0 (_mm512_loadu_si512(Shuffler<R, G, B>::ShufG  + 0)),
        ShufG1 (_mm512_loadu_si512(Shuffler<R, G, B>::ShufG  + 64))
    {
        const auto lut = lut_.Shuffle<0, 2, 3, 5, 7, 1, 4, 6>();
        LutYRB  = _mm512_broadcastd_epi32(lut);
        LutYG   = _mm512_broadcastw_epi16(lut.MoveToLo<5>());
        LutCbRB = _mm512_broadcastd_epi32(lut.MoveToLo<2>());
        LutCbG  = _mm512_broadcastw_epi16(lut.MoveToLo<6>());
        LutCrRB = _mm512_broadcastd_epi32(lut.MoveToLo<3>());
        LutCrG  = _mm512_broadcastw_epi16(lut.MoveToLo<7>());
    }
};
struct RGB8ToYUV8_I16_AVX512VBMI : public RGB8ToYUV8_I16_AVX512VBMI_Base
{
    using RGB8ToYUV8_I16_AVX512VBMI_Base::RGB8ToYUV8_I16_AVX512VBMI_Base;
    static forceinline auto DP3(const __m512i& rbLo, const __m512i& rbHi, const __m512i& g, const __m512i& lutRB, const __m512i& lutG) noexcept
    {
#define M4(i) i+14, i+13, i+10, i+9, i+6, i+5, i+2, i+1
        const auto ShufMid16 = _mm512_set_epi8(M4(112), M4(96), M4(80), M4(64), M4(48), M4(32), M4(16), M4(0));
#undef M4
        const auto midRB0 = _mm512_madd_epi16(rbLo, lutRB);
        const auto midRB1 = _mm512_madd_epi16(rbHi, lutRB);
        const auto midRB = _mm512_permutex2var_epi8(midRB0, ShufMid16, midRB1); // << (14 - 8)=6
        const auto midG = _mm512_mulhrs_epi16(g, lutG); // << (7 + 14 - 15)=6
        const auto sum = _mm512_srai_epi16(_mm512_add_epi16(_mm512_add_epi16(_mm512_set1_epi16(32), midG), midRB), 6);
        return sum;
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src), dat1 = _mm512_loadu_epi8(src + 64), dat2 = _mm512_loadu_epi8(src + 128);
        const auto u16mask = _cvtu64_mask64(0x5555555555555555u);
        const auto midRB0 = _mm512_maskz_permutexvar_epi8 (u16mask,       ShufRB0, dat0);
        const auto midRB1 = _mm512_maskz_permutex2var_epi8(u16mask, dat0, ShufRB1, dat1);
        const auto midRB2 = _mm512_maskz_permutex2var_epi8(u16mask, dat1, ShufRB2, dat2);
        const auto midRB3 = _mm512_maskz_permutexvar_epi8 (u16mask,       ShufRB3, dat2);
        const auto midG0  = _mm512_slli_epi16(_mm512_maskz_permutex2var_epi8(u16mask, dat0, ShufG0, dat1), 7);
        const auto midG1  = _mm512_slli_epi16(_mm512_maskz_permutex2var_epi8(u16mask, dat1, ShufG1, dat2), 7);

#define DP3x4(out, lut) const auto out##0 = DP3(midRB0, midRB1, midG0, lut##RB, lut##G), out##1 = DP3(midRB2, midRB3, midG1, lut##RB, lut##G)
        DP3x4(lumi, LutY);
        DP3x4(cb,  LutCb);
        DP3x4(cr,  LutCr);
#undef DP3x4
#define ODD16(x) x+30, x+28, x+26, x+24, x+22, x+20, x+18, x+16, x+14, x+12, x+10, x+8, x+6, x+4, x+2, x+0
        const auto ShufLo = _mm512_set_epi8(ODD16(96), ODD16(64), ODD16(32), ODD16(0));
#undef ODD16
        auto outY = _mm512_permutex2var_epi8(lumi0, ShufLo, lumi1);
        if (needAddY)
            outY = _mm512_add_epi8(outY, _mm512_set1_epi8(16));
        const auto cb_ = _mm512_packs_epi16(cb0, cb1), cr_ = _mm512_packs_epi16(cr0, cr1); // 04 15 26 37
        const auto addToU8 = _mm512_set1_epi8(static_cast<int8_t>(128));
        const auto outCb_ = _mm512_add_epi8(cb_, addToU8), outCr_ = _mm512_add_epi8(cr_, addToU8);

        const auto shuffleRGR0 = _mm512_set_epi8(
             21,  20, 100,  20,  19,  99,  19,  18,  98,  18,  17,  97,  17,  16,  96,  16,
             15,  87,  15,  14,  86,  14,  13,  85,  13,  12,  84,  12,  11,  83,  11,  10,
             82,  10,   9,  81,   9,   8,  80,   8,   7,  71,   7,   6,  70,   6,   5,  69,
              5,   4,  68,   4,   3,  67,   3,   2,  66,   2,   1,  65,   1,   0,  64,   0);
        const auto valRGR0 = _mm512_permutex2var_epi8(outY, shuffleRGR0, outCb_);
        const auto shuffleRGR1 = _mm512_set_epi8(
             90,  42,  41,  89,  41,  40,  88,  40,  39,  79,  39,  38,  78,  38,  37,  77,
             37,  36,  76,  36,  35,  75,  35,  34,  74,  34,  33,  73,  33,  32,  72,  32,
             31, 119,  31,  30, 118,  30,  29, 117,  29,  28, 116,  28,  27, 115,  27,  26,
            114,  26,  25, 113,  25,  24, 112,  24,  23, 103,  23,  22, 102,  22,  21, 101);
        const auto valRGR1 = _mm512_permutex2var_epi8(outY, shuffleRGR1, outCb_);
        const auto shuffleRGR2 = _mm512_set_epi8(
             63, 127,  63,  62, 126,  62,  61, 125,  61,  60, 124,  60,  59, 123,  59,  58,
            122,  58,  57, 121,  57,  56, 120,  56,  55, 111,  55,  54, 110,  54,  53, 109,
             53,  52, 108,  52,  51, 107,  51,  50, 106,  50,  49, 105,  49,  48, 104,  48,
             47,  95,  47,  46,  94,  46,  45,  93,  45,  44,  92,  44,  43,  91,  43,  42);
        const auto valRGR2 = _mm512_permutex2var_epi8(outY, shuffleRGR2, outCb_);

        const auto insertB0 = _mm512_set_epi8(
            63, 100, 61, 60, 99, 58, 57, 98, 55, 54, 97, 52, 51, 96, 49, 48,
            87,  46, 45, 86, 43, 42, 85, 40, 39, 84, 37, 36, 83, 34, 33, 82,
            31,  30, 81, 28, 27, 80, 25, 24, 71, 22, 21, 70, 19, 18, 69, 16,
            15,  68, 13, 12, 67, 10,  9, 66,  7,  6, 65,  4,  3, 64,  1,  0);
        const auto out0 = _mm512_permutex2var_epi8(valRGR0, insertB0, outCr_);
        const auto insertB1 = _mm512_set_epi8(
             63,  62,  89,  60,  59,  88,  57,  56,  79,  54,  53,  78,  51,  50,  77,  48,
             47,  76,  45,  44,  75,  42,  41,  74,  39,  38,  73,  36,  35,  72,  33,  32,
            119,  30,  29, 118,  27,  26, 117,  24,  23, 116,  21,  20, 115,  18,  17, 114,
             15,  14, 113,  12,  11, 112,   9,   8, 103,   6,   5, 102,   3,   2, 101,   0);
        const auto out1 = _mm512_permutex2var_epi8(valRGR1, insertB1, outCr_);
        const auto insertB2 = _mm512_set_epi8(
            127,  62,  61, 126,  59,  58, 125,  56,  55, 124,  53,  52, 123,  50,  49, 122,
             47,  46, 121,  44,  43, 120,  41,  40, 111,  38,  37, 110,  35,  34, 109,  32,
             31, 108,  29,  28, 107,  26,  25, 106,  23,  22, 105,  20,  19, 104,  17,  16,
             95,  14,  13,  94,  11,  10,  93,   8,   7,  92,   5,   4,  91,   2,   1,  90);
        const auto out2 = _mm512_permutex2var_epi8(valRGR2, insertB2, outCr_);
        
        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
};
struct RGB8ToYUV8_I8_AVX512VBMI_Base
{
    static constexpr size_t M = 192, N = 192, K = 64;
    template<uint8_t R, uint8_t G, uint8_t B>
    struct Shuffler
    {
#define PUT2x4(a,b,i) a+i, b+i, a+i+3, b+i+3, a+i+6, b+i+6, a+i+9, b+i+9
#define SHUF64(a,b)alignas(64) static inline const uint8_t Shuf##a##b[] = { PUT2x4(a,b,0), PUT2x4(a,b,12), PUT2x4(a,b,24), PUT2x4(a,b,36), PUT2x4(a,b,48), PUT2x4(a,b,60), PUT2x4(a,b,72), PUT2x4(a,b,84), \
            PUT2x4(a, b, 32), PUT2x4(a, b, 44), PUT2x4(a, b, 56), PUT2x4(a, b, 68), PUT2x4(a, b, 80), PUT2x4(a, b, 92), PUT2x4(a, b, 104), PUT2x4(a, b, 116) }
        SHUF64(R, G);
        SHUF64(R, B);
        SHUF64(G, B);
#undef SHUF64
#undef PUT2x4
    };
    const __m512i ShufRG0, ShufRG1, ShufRB0, ShufRB1, ShufGB0, ShufGB1;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I8_AVX512VBMI_Base(RGBOrder<R, G, B>) noexcept :
#define LSHUF(a,b) Shuf##a##b##0(_mm512_loadu_si512(Shuffler<R, G, B>::Shuf##a##b)), Shuf##a##b##1(_mm512_loadu_si512(Shuffler<R, G, B>::Shuf##a##b + 64))
        LSHUF(R, G), LSHUF(R, B), LSHUF(G, B)
#undef LSHUF
    { }
};
struct RGB8ToYUV8_I8_AVX512VBMI : public RGB8ToYUV8_I8_AVX512VBMI_Base
{
    const __m512i LutYRG, LutYGB, LutCbRB, LutCbGB, LutCrRG, LutCrRB;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I8_AVX512VBMI(const int16_t* lut, RGBOrder<R, G, B> order) noexcept : RGB8ToYUV8_I8_AVX512VBMI_Base(order),
#define LLUT(x,i) Lut##x(_mm512_set1_epi16(lut[i]))
        LLUT(YRG, 0), LLUT(YGB, 1), LLUT(CbRB, 2), LLUT(CbGB, 3), LLUT(CrRG, 4), LLUT(CrRB, 5)
#undef LLUT
    { }
    template<bool IsY>
    static forceinline auto DP4(const __m512i& part0, const __m512i& part1, const __m512i& lut0, const __m512i& lut1) noexcept
    {
        const auto mid0 = _mm512_maddubs_epi16(part0, lut0), mid1 = _mm512_maddubs_epi16(part1, lut1);
        const auto sum = _mm512_add_epi16(mid0, mid1);
        const auto add128 = _mm512_set1_epi16(128);
        if constexpr (IsY)
            return _mm512_add_epi16(sum, add128);
        else
            return _mm512_adds_epi16(sum, add128);
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src), dat1 = _mm512_loadu_epi8(src + 64), dat2 = _mm512_loadu_epi8(src + 128);
        const auto midRG0 = _mm512_permutex2var_epi8(dat0, ShufRG0, dat1);
        const auto midRG1 = _mm512_permutex2var_epi8(dat1, ShufRG1, dat2);
        const auto midGB0 = _mm512_permutex2var_epi8(dat0, ShufGB0, dat1);
        const auto midGB1 = _mm512_permutex2var_epi8(dat1, ShufGB1, dat2);
        const auto midRB0 = _mm512_permutex2var_epi8(dat0, ShufRB0, dat1);
        const auto midRB1 = _mm512_permutex2var_epi8(dat1, ShufRB1, dat2);

#define DP4x4(out, isy, a, b, lut) const auto out##0 = DP4<isy>(mid##a##0, mid##b##0, lut##a, lut##b), out##1 = DP4<isy>(mid##a##1, mid##b##1, lut##a, lut##b)
        DP4x4(lumi, true, RG, GB, LutY);
        DP4x4(cb,  false, RB, GB, LutCb);
        DP4x4(cr,  false, RG, RB, LutCr);
#undef DP4x4
#define EVEN16(x) x+31, x+29, x+27, x+25, x+23, x+21, x+19, x+17, x+15, x+13, x+11, x+9, x+7, x+5, x+3, x+1
        const auto ShufHi = _mm512_set_epi8(EVEN16(96), EVEN16(64), EVEN16(32), EVEN16(0));
#undef EVEN16
        auto outY = _mm512_permutex2var_epi8(lumi0, ShufHi, lumi1);
        if (needAddY)
            outY = _mm512_add_epi8(outY, _mm512_set1_epi8(16));
        const auto cb = _mm512_permutex2var_epi8(cb0, ShufHi, cb1), cr = _mm512_permutex2var_epi8(cr0, ShufHi, cr1);
        const auto addToU8 = _mm512_set1_epi8(static_cast<int8_t>(128));
        const auto outCb = _mm512_add_epi8(cb, addToU8), outCr = _mm512_add_epi8(cr, addToU8);

        Combine8x3_512VBMI::DoStore(dst, outY, outCb, outCr);
    }
};
struct RGB8ToYUV8_I8DP4A_AVX512VBMI : public RGB8ToYUV8_I8_AVX512VBMI_Base
{
    const __m512i LutY, LutCb, LutCr;
    template<uint8_t R, uint8_t G, uint8_t B>
    RGB8ToYUV8_I8DP4A_AVX512VBMI(const int32_t* lut, RGBOrder<R, G, B> order) noexcept : RGB8ToYUV8_I8_AVX512VBMI_Base(order),
        LutY(_mm512_set1_epi32(lut[0])), LutCb(_mm512_set1_epi32(lut[1])), LutCr(_mm512_set1_epi32(lut[2]))
    { }
    static forceinline auto DP4(const __m512i& part0, const __m512i& part1, const __m512i& lut, const int32_t base) noexcept
    {
        const auto rgb0246 = _mm512_unpacklo_epi16(part0, part1), rgb1357 = _mm512_unpackhi_epi16(part0, part1);
        const auto adder = _mm512_set1_epi32((base << 8) + 128); // pre-add round
        const auto mid0246 = _mm512_dpbusd_epi32(adder, rgb0246, lut), mid1357 = _mm512_dpbusd_epi32(adder, rgb1357, lut);
        return _mm512_packus_epi32(mid0246, mid1357);
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src), dat1 = _mm512_loadu_epi8(src + 64), dat2 = _mm512_loadu_epi8(src + 128);
        const auto midRG0 = _mm512_permutex2var_epi8(dat0, ShufRG0, dat1);
        const auto midRG1 = _mm512_permutex2var_epi8(dat1, ShufRG1, dat2);
        const auto midGB0 = _mm512_permutex2var_epi8(dat0, ShufGB0, dat1);
        const auto midGB1 = _mm512_permutex2var_epi8(dat1, ShufGB1, dat2);
        const auto midRB0 = _mm512_permutex2var_epi8(dat0, ShufRB0, dat1);
        const auto midRB1 = _mm512_permutex2var_epi8(dat1, ShufRB1, dat2);

#define DP4x4(out, a, b, lut, base) const auto out##0 = DP4(mid##a##0, mid##b##0, lut, base), out##1 = DP4(mid##a##1, mid##b##1, lut, base)
        DP4x4(lumi, RG, GB, LutY, needAddY ? 16 : 0);
        DP4x4(cb,   RB, GB, LutCb, 128);
        DP4x4(cr,   RG, RB, LutCr, 128);
#undef DP4x4
#define EVEN16(x) x+31, x+29, x+27, x+25, x+23, x+21, x+19, x+17, x+15, x+13, x+11, x+9, x+7, x+5, x+3, x+1
        const auto ShufHi = _mm512_set_epi8(EVEN16(96), EVEN16(64), EVEN16(32), EVEN16(0));
#undef EVEN16
        const auto outY = _mm512_permutex2var_epi8(lumi0, ShufHi, lumi1);
        const auto outCb = _mm512_permutex2var_epi8(cb0, ShufHi, cb1);
        const auto outCr = _mm512_permutex2var_epi8(cr0, ShufHi, cr1);
        Combine8x3_512VBMI::DoStore(dst, outY, outCb, outCr);
    }
};
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8, AVX512VBMI_I16)
{
    if (count >= RGB8ToYUV8_I16_AVX512VBMI::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I16_AVX512VBMI proc(I16x8(RGB8ToYCC8LUTI16[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, AVX512VBMI_I8)
{
    if (count >= RGB8ToYUV8_I8_AVX512VBMI::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8_AVX512VBMI proc(reinterpret_cast<const int16_t*>(RGB8ToYCC8LUTI8x4[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I8>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(RGB8ToYCbCr8Fast, AVX512VNNI_I8)
{
    if (count >= RGB8ToYUV8_I8DP4A_AVX512VBMI::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const RGB8ToYUV8_I8DP4A_AVX512VBMI proc(reinterpret_cast<const int32_t*>(RGB8ToYCC8LUTI8x4[mval].data()), RGBOrder<0, 1, 2>{});
        do
        {
            proc.Convert(dest, src, needAddY);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I8>(dest, src, count, mval);
}

template<typename T>
struct YUV8ToRGB8_I16_AVX512VBMI_Base
{
    static constexpr size_t M = 192, N = 192, K = 64;
#define CBR(i) 0, i+1, 0, i+2
#define CBR6x4(i) CBR(i), CBR(i+6), CBR(i+12), CBR(i+18)
    alignas(64) static inline constexpr uint8_t ShufCBR_[] = {
        CBR6x4( 0), CBR6x4(24), CBR6x4(48), CBR6x4( 72),
        CBR6x4( 3), CBR6x4(27), CBR6x4(51), CBR6x4( 75),
        CBR6x4(32), CBR6x4(56), CBR6x4(80), CBR6x4(104),
        CBR6x4(35), CBR6x4(59), CBR6x4(83), CBR6x4(107) };
#undef CBR6
#undef CBR
#define Y4(i) i, 0, i+3, 0, i+6, 0, i+9, 0 
    alignas(64) static inline constexpr uint8_t ShufY_[] = { Y4(0), Y4(12), Y4(24), Y4(36), Y4(48), Y4(60), Y4(72), Y4(84), Y4(32), Y4(44), Y4(56), Y4(68), Y4(80), Y4(92), Y4(104), Y4(116) };
#undef Y4
#define RGR(i) i, i+64, i
#define RGR8(i) RGR(i), RGR(i+1), RGR(i+2), RGR(i+3), RGR(i+4), RGR(i+5), RGR(i+6), RGR(i+7)
    alignas(64) static inline constexpr uint8_t ShufRGR_[] = { RGR8(0), RGR8(16), RGR8(32), RGR8(48), RGR8(8), RGR8(24), RGR8(40), RGR8(56) };
#undef RGR8
#undef RGR
    const __m512i ShufCBR0, ShufCBR1, ShufCBR2, ShufCBR3, ShufY0, ShufY1, ShufRGR0, ShufRGR1, ShufRGR2, U16_16;
    YUV8ToRGB8_I16_AVX512VBMI_Base() noexcept :
        ShufCBR0(_mm512_loadu_si512(ShufCBR_ + 0)),
        ShufCBR1(_mm512_loadu_si512(ShufCBR_ + 64)),
        ShufCBR2(_mm512_loadu_si512(ShufCBR_ + 128)),
        ShufCBR3(_mm512_loadu_si512(ShufCBR_ + 192)),
        ShufY0  (_mm512_loadu_si512(ShufY_   + 0)),
        ShufY1  (_mm512_loadu_si512(ShufY_   + 64)),
        ShufRGR0(_mm512_loadu_si512(ShufRGR_ + 0)),
        ShufRGR1(_mm512_loadu_si512(ShufRGR_ + 64)),
        ShufRGR2(_mm512_loadu_si512(ShufRGR_ + 128)),
        U16_16  (_mm512_set1_epi16(16))
    { }
    forceinline void StoreHalfLaneMix(uint8_t* __restrict dst, const __m512i& dat0, const __m512i& dat1, const __m512i& dat2) const noexcept
    {
        const auto valRGR0 = _mm512_permutex2var_epi8(dat0, ShufRGR0, dat1);
        const auto valRGR1 = _mm512_permutex2var_epi8(dat0, ShufRGR1, dat1);
        const auto valRGR2 = _mm512_permutex2var_epi8(dat0, ShufRGR2, dat1);

        const auto insertB0 = _mm512_set_epi8(
            63, 100, 61, 60, 99, 58, 57, 98, 55, 54, 97, 52, 51, 96, 49, 48,
            87,  46, 45, 86, 43, 42, 85, 40, 39, 84, 37, 36, 83, 34, 33, 82,
            31,  30, 81, 28, 27, 80, 25, 24, 71, 22, 21, 70, 19, 18, 69, 16,
            15,  68, 13, 12, 67, 10,  9, 66,  7,  6, 65,  4,  3, 64,  1,  0);
        const auto out0 = _mm512_permutex2var_epi8(valRGR0, insertB0, dat2);
        const auto insertB1 = _mm512_set_epi8(
             63,  62,  89,  60,  59,  88,  57,  56,  79,  54,  53,  78,  51,  50,  77,  48,
             47,  76,  45,  44,  75,  42,  41,  74,  39,  38,  73,  36,  35,  72,  33,  32,
            119,  30,  29, 118,  27,  26, 117,  24,  23, 116,  21,  20, 115,  18,  17, 114,
             15,  14, 113,  12,  11, 112,   9,   8, 103,   6,   5, 102,   3,   2, 101,   0);
        const auto out1 = _mm512_permutex2var_epi8(valRGR1, insertB1, dat2);
        const auto insertB2 = _mm512_set_epi8(
            127,  62,  61, 126,  59,  58, 125,  56,  55, 124,  53,  52, 123,  50,  49, 122,
             47,  46, 121,  44,  43, 120,  41,  40, 111,  38,  37, 110,  35,  34, 109,  32,
             31, 108,  29,  28, 107,  26,  25, 106,  23,  22, 105,  20,  19, 104,  17,  16,
             95,  14,  13,  94,  11,  10,  93,   8,   7,  92,   5,   4,  91,   2,   1,  90);
        const auto out2 = _mm512_permutex2var_epi8(valRGR2, insertB2, dat2);
        
        _mm512_storeu_epi8(dst +   0, out0);
        _mm512_storeu_epi8(dst +  64, out1);
        _mm512_storeu_epi8(dst + 128, out2);
    }
    forceinline void Convert(uint8_t* __restrict dst, const uint8_t* __restrict src, bool needAddY, bool isRGBFull, bool isRGB) const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        const auto dat0 = _mm512_loadu_epi8(src), dat1 = _mm512_loadu_epi8(src + 64), dat2 = _mm512_loadu_epi8(src + 128);
        const auto u8_128 = _mm512_set1_epi8(static_cast<char>(128));
        const auto dat0s = _mm512_sub_epi8(dat0, u8_128), dat1s = _mm512_sub_epi8(dat1, u8_128), dat2s = _mm512_sub_epi8(dat2, u8_128);
        const auto u16maskL = _cvtu64_mask64(0x5555555555555555u), u16maskH = _cvtu64_mask64(0xaaaaaaaaaaaaaaaau);
        const auto datY0 = _mm512_maskz_permutex2var_epi8(u16maskL, dat0,  ShufY0,   dat1);
        const auto datY1 = _mm512_maskz_permutex2var_epi8(u16maskL, dat1,  ShufY1,   dat2);
        const auto cbr0e = _mm512_maskz_permutex2var_epi8(u16maskH, dat0s, ShufCBR0, dat1s); // 0246
        const auto cbr0o = _mm512_maskz_permutex2var_epi8(u16maskH, dat0s, ShufCBR1, dat1s); // 1357
        const auto cbr1e = _mm512_maskz_permutex2var_epi8(u16maskH, dat1s, ShufCBR2, dat2s); // 8ace
        const auto cbr1o = _mm512_maskz_permutex2var_epi8(u16maskH, dat1s, ShufCBR3, dat2s); // 9bdf
       
        const auto y_0 = needAddY ? _mm512_sub_epi16(datY0, U16_16) : datY0, y_1 = needAddY ? _mm512_sub_epi16(datY1, U16_16) : datY1;
        // (7 + x) - 15
        const auto y0 = _mm512_mulhrs_epi16(_mm512_slli_epi16(y_0, 7), self.MulY);
        const auto y1 = _mm512_mulhrs_epi16(_mm512_slli_epi16(y_1, 7), self.MulY);

#define CalcRGB(x) const auto [r##x, g##x, b##x] = self.Calc(y##x, cbr##x##e, cbr##x##o)
        CalcRGB(0);
        CalcRGB(1);
#undef  CalcRGB

#define Combine4(x) auto x = _mm512_packus_epi16(x##0, x##1)
        Combine4(r);
        Combine4(g);
        Combine4(b);
#undef  Combine4
        if (!isRGBFull)
        {
            const auto rgbMin = _mm512_set1_epi8(16), rgbMax = _mm512_set1_epi8(static_cast<char>(235));
#define ClampRGB(x) x = _mm512_min_epu8(_mm512_max_epu8(x, rgbMin), rgbMax)
            ClampRGB(r);
            ClampRGB(g);
            ClampRGB(b);
#undef  ClampRGB
        }

        if (isRGB)
            StoreHalfLaneMix(dst, r, g, b);
        else
            StoreHalfLaneMix(dst, b, g, r);
    }
};
struct YUV8ToRGB8_I16_AVX512VBMI : public YUV8ToRGB8_I16_AVX512VBMI_Base<YUV8ToRGB8_I16_AVX512VBMI>
{
    const __m512i MulY, MulR, MulB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_AVX512VBMI(const uint32_t* lut) noexcept : YUV8ToRGB8_I16_AVX512VBMI_Base(),
        MulY(_mm512_set1_epi16(static_cast<int16_t>(lut[0] >> 1))),
        MulR(_mm512_set1_epi32(static_cast<int32_t>(lut[1] & 0xffff0000u))),
        MulB(_mm512_set1_epi32(static_cast<int32_t>(lut[1] & 0x0000ffffu))),
        MulG(_mm512_set1_epi32(static_cast<int32_t>(lut[2])))
    { }
    forceinline std::tuple<__m512i, __m512i, __m512i> Calc(const __m512i& y5, const __m512i& cbrE, const __m512i& cbrO) const noexcept
    {
        const auto rE = _mm512_madd_epi16(cbrE, MulR), rO = _mm512_madd_epi16(cbrO, MulR);
        const auto gE = _mm512_madd_epi16(cbrE, MulG), gO = _mm512_madd_epi16(cbrO, MulG);
        const auto bE = _mm512_madd_epi16(cbrE, MulB), bO = _mm512_madd_epi16(cbrO, MulB);
        // (8 + 13) - 16 = 5
        const auto r5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(rE, 16), rO);
        const auto g5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(gE, 16), gO);
        const auto b5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(bE, 16), bO);
        const auto y = _mm512_add_epi16(y5, U16_16); // add rounding
        const auto r = _mm512_srai_epi16(_mm512_add_epi16(r5, y), 5), b = _mm512_srai_epi16(_mm512_add_epi16(b5, y), 5);
        const auto g = _mm512_srai_epi16(_mm512_sub_epi16(y, g5), 5);
        return { r, g, b };
    }
};
struct YUV8ToRGB8_I16_2_AVX512VBMI : public YUV8ToRGB8_I16_AVX512VBMI_Base<YUV8ToRGB8_I16_2_AVX512VBMI>
{
    const __m512i MulY, MulRB, MulG;
    // Y,0,Bcb,Rcr,Gcb,Gcr
    YUV8ToRGB8_I16_2_AVX512VBMI(const uint32_t* lut) noexcept : YUV8ToRGB8_I16_AVX512VBMI_Base(),
        MulY (_mm512_set1_epi16(static_cast<int16_t>(lut[0]))),
        MulRB(_mm512_set1_epi32(static_cast<int32_t>(lut[1]))),
        MulG (_mm512_set1_epi32(static_cast<int32_t>(lut[2])))
    { }
    forceinline std::tuple<__m512i, __m512i, __m512i> Calc(const __m512i& y6, const __m512i& cbrE, const __m512i& cbrO) const noexcept
    {
        // (8 + 13) - 16 = 5
        const auto gE = _mm512_madd_epi16(cbrE, MulG), gO = _mm512_madd_epi16(cbrO, MulG);
        // (8 + 13) - 15 = 6
        const auto rbE = _mm512_mulhrs_epi16(cbrE, MulRB), rbO = _mm512_mulhrs_epi16(cbrO, MulRB);
        const auto r6 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(rbE, 16), rbO);
        const auto b6 = _mm512_mask_blend_epi16(0x55555555u, _mm512_slli_epi32(rbO, 16), rbE);
        const auto g5 = _mm512_mask_blend_epi16(0xaaaaaaaau, _mm512_srli_epi32(gE,  16), gO);
        const auto yrb = _mm512_add_epi16(y6, _mm512_set1_epi16(32)); // add rounding
        const auto r = _mm512_srai_epi16(_mm512_add_epi16(r6, yrb), 6), b = _mm512_srai_epi16(_mm512_add_epi16(b6, yrb), 6);
        const auto g = _mm512_srai_epi16(_mm512_sub_epi16(_mm512_srai_epi16(yrb, 1), g5), 5);
        return { r, g, b };
    }
};
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX512VBMI_I16)
{
    if (count >= YUV8ToRGB8_I16_AVX512VBMI::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_AVX512VBMI proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I16>(dest, src, count, mval);
}
DEFINE_FASTPATH_METHOD(YCbCr8ToRGB8, AVX512VBMI_I16_2)
{
    if (count >= YUV8ToRGB8_I16_2_AVX512VBMI::K)
    {
        [[maybe_unused]] const auto [isRGBFull, isYCCFull, needAddY] = CheckYCCMatrix(mval);
        const YUV8ToRGB8_I16_2_AVX512VBMI proc(reinterpret_cast<const uint32_t*>(YCC8ToRGB8LUTI16[mval].data()));
        do
        {
            proc.Convert(dest, src, needAddY, isRGBFull, true);
            src += proc.M; dest += proc.N; count -= proc.K;
        } while (count >= proc.K);
    }
    if (count)
        Func<AVX2_I16_2>(dest, src, count, mval);
}

DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<G2RGB8_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<G2RGBA8_512VBMI, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<GA2RGBA8_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<RGB2RGBA8_512VBMI, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGBA2RGB8_512VBMI, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX512VBMI)
{
    ProcessLOOP4<RGBToBGR8_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, AVX512VBMI)
{
    ProcessLOOP4<RGBAToBGRA8_512VBMI, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel8_3_1_512VBMI<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel8_3_1_512VBMI<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel8_3_1_512VBMI<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512VBMI)
{
    ExtractLOOP4<2, Extract8x2_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, AVX512VBMI)
{
    ExtractLOOP4<3, Extract8x3_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512VBMI)
{
    ExtractLOOP4<4, Extract8x4_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, AVX512VBMI)
{
    CombineLOOP4<3, Combine8x3_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToG16, AVX512VBMI)
{
    ProcessLOOP4<G8ToG16_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G16ToG8, AVX512VBMI)
{
    ProcessLOOP4<G16ToG8_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGB5x5ToRGB8_512VBMI<true, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGB5x5ToRGB8_512VBMI<false, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGB5x5ToRGB8_512VBMI<true, false>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGB5x5ToRGB8_512VBMI<false, false>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_512VBMI<true, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_512VBMI<false, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGB8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_512VBMI<true, false>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGB8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGB8_LUT_512VBMI<false, false>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<true, false, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<false, false, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<true, true, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<false, true, true>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB565ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<true, false, false>, &Func<AVX512BW>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR565ToRGBA8, AVX512VBMI_2)
{
    ProcessLOOP4<RGB5x5ToRGBA8_LUT_512VBMI<false, false, false>, &Func<AVX512BW>>(dest, src, count);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (defined(__AVX512VBMI2__) || COMMON_COMPILER_MSVC)
#   pragma message("Compiling ColorConvert with AVX512-VBMI2")
struct G2GA8_512VBMI2
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2GA8_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x5555555555555555u);
        //__mmask64, 1 from mem, 0 from Alpha
        const auto out0 = _mm512_mask_expandloadu_epi8(Alpha, mask, src);
        const auto out1 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 32);
        _mm512_storeu_si512(dst     , out0);
        _mm512_storeu_si512(dst + 32, out1);
    }
};
struct RGB2RGBA8_512VBMI2
{
    static constexpr size_t M = 96, N = 32, K = 32;
    __m512i Alpha;
    forceinline RGB2RGBA8_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {

        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto out0 = _mm512_mask_expandloadu_epi8(Alpha, mask, src);
        const auto out1 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 48);
        _mm512_storeu_si512(dst     , out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct RGBA2RGB8_512VBMI2
{
    static constexpr size_t M = 32, N = 96, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 16);
        _mm512_mask_compressstoreu_epi8(dst     , mask, dat0);
        _mm512_mask_compressstoreu_epi8(dst + 48, mask, dat1);
    }
};
template<uint8_t Idx>
struct KeepChannel3_1_512VBMI2
{
    static constexpr size_t M = 192, N = 64, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const auto dat1 = _mm512_loadu_epi8(src + 64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        constexpr uint64_t Mask0 = 0x9249249249249249u << Idx;
        const auto mask0 = _cvtu64_mask64(Mask0);
        _mm512_mask_compressstoreu_epi8(dst, mask0, dat0);

        constexpr uint32_t Mask1Add[3] = { 0u,1u,2u };
        constexpr uint64_t Mask1 = (0x4924924924924924u << Idx) | Mask1Add[Idx];
        const auto mask1 = _cvtu64_mask64(Mask1);
        _mm512_mask_compressstoreu_epi8(dst + (Idx > 0 ? 21 : 22), mask1, dat1);

        constexpr uint32_t Mask2Add[3] = { 0u,0u,1u };
        constexpr uint64_t Mask2 = (0x2492492492492492u << Idx) | Mask2Add[Idx];
        const auto mask2 = _cvtu64_mask64(Mask2);
        _mm512_mask_compressstoreu_epi8(dst + (Idx > 1 ? 42 : 43), mask2, dat2);
    }
};
struct Extract8x2_512VBMI2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src);

        constexpr uint64_t Mask0 = 0x5555555555555555u;
        const auto mask0 = _cvtu64_mask64(Mask0);
        const U8x32 out0 = _mm512_castsi512_si256(_mm512_maskz_compress_epi8(mask0, dat));

        constexpr uint64_t Mask1 = 0xaaaaaaaaaaaaaaaau;
        const auto mask1 = _cvtu64_mask64(Mask1);
        const U8x32 out1 = _mm512_castsi512_si256(_mm512_maskz_compress_epi8(mask1, dat));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
    }
};
struct Extract8x3_512VBMI2
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src);
        const U8x32 dat1(src + 64);

        constexpr uint64_t Mask0R = 0x9249249249249249u;
        constexpr uint64_t Mask0G = Mask0R << 1;
        constexpr uint64_t Mask0B = Mask0R << 2;
        const U8x32 val0R = _mm512_castsi512_si256(_mm512_maskz_compress_epi8(_cvtu64_mask64(Mask0R), dat0));
        const U8x32 val0G = _mm512_castsi512_si256(_mm512_maskz_compress_epi8(_cvtu64_mask64(Mask0G), dat0));
        const U8x32 val0B = _mm512_castsi512_si256(_mm512_maskz_compress_epi8(_cvtu64_mask64(Mask0B), dat0));

        constexpr uint32_t Mask1R = 0x24924924u;
        constexpr uint32_t Mask1G = (Mask1R << 1) | 1u;
        constexpr uint32_t Mask1B = (Mask1R << 2) | 2u;
        const auto mid1R = _mm256_castsi256_si128(_mm256_maskz_compress_epi8(_cvtu32_mask32(Mask1R), dat1));
        const auto mid1G = _mm256_castsi256_si128(_mm256_maskz_compress_epi8(_cvtu32_mask32(Mask1G), dat1));
        const auto mid1B = _mm256_castsi256_si128(_mm256_maskz_compress_epi8(_cvtu32_mask32(Mask1B), dat1));

        constexpr uint32_t MaskInsertR  = ~((1u << 22) - 1u);
        constexpr uint32_t MaskInsertGB = ~((1u << 21) - 1u);
        const auto maskShiftR  = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9);
        const auto maskShiftGB = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        const U8x32 out0 = _mm256_mask_shuffle_epi8(val0R, _cvtu32_mask32(MaskInsertR ), _mm256_broadcastsi128_si256(mid1R), maskShiftR );
        const U8x32 out1 = _mm256_mask_shuffle_epi8(val0G, _cvtu32_mask32(MaskInsertGB), _mm256_broadcastsi128_si256(mid1G), maskShiftGB);
        const U8x32 out2 = _mm256_mask_shuffle_epi8(val0B, _cvtu32_mask32(MaskInsertGB), _mm256_broadcastsi128_si256(mid1B), maskShiftGB);

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
    }
};
struct Extract8x4_512VBMI2
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi32(src);

        constexpr uint64_t MaskR = 0x1111111111111111u;
        constexpr uint64_t MaskG = MaskR << 1;
        constexpr uint64_t MaskB = MaskR << 2;
        constexpr uint64_t MaskA = MaskR << 3;

        const U8x16 out0 = _mm512_castsi512_si128(_mm512_maskz_compress_epi8(_cvtu64_mask64(MaskR), dat));
        const U8x16 out1 = _mm512_castsi512_si128(_mm512_maskz_compress_epi8(_cvtu64_mask64(MaskG), dat));
        const U8x16 out2 = _mm512_castsi512_si128(_mm512_maskz_compress_epi8(_cvtu64_mask64(MaskB), dat));
        const U8x16 out3 = _mm512_castsi512_si128(_mm512_maskz_compress_epi8(_cvtu64_mask64(MaskA), dat));

        out0.Save(dst[0]);
        out1.Save(dst[1]);
        out2.Save(dst[2]);
        out3.Save(dst[3]);
    }
};
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX512VBMI2)
{
    ProcessLOOP4<G2GA8_512VBMI2, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX512VBMI2)
{
    ProcessLOOP4<RGB2RGBA8_512VBMI2, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX512VBMI2)
{
    ProcessLOOP4<RGBA2RGB8_512VBMI2, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, AVX512VBMI2)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI2<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, AVX512VBMI2)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI2<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, AVX512VBMI2)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI2<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512VBMI2)
{
    ExtractLOOP4<2, Extract8x2_512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x3, AVX512VBMI2)
{
    ExtractLOOP4<3, Extract8x3_512VBMI2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512VBMI2)
{
    ExtractLOOP4<4, Extract8x4_512VBMI2, &Func<LOOP>>(dest, src, count);
}
#endif


}
