
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

#define DoResizeInfo (void)(const ::xziar::img::STBResize::ResizeInfo& info)

#define G8ToGA8Info         (void)(uint16_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define G8ToRGB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define G8ToRGBA8Info       (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define GA8ToRGB8Info       (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define GA8ToRGBA8Info      (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define GfToGAfInfo         (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define GfToRGBfInfo        (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define GfToRGBAfInfo       (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define GAfToRGBfInfo       (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define GAfToRGBAfInfo      (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGB8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define BGR8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define RGBA8ToRGB8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToBGR8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBfToRGBAfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define BGRfToRGBAfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count, float alpha)
#define RGBAfToRGBfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBGRfInfo     (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGB8ToBGR8Info      (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGBA8ToBGRA8Info    (void)(uint32_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBfToBGRfInfo      (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBGRAfInfo    (void)(float* __restrict dest, const float* __restrict src, size_t count)
#define RGBA8ToR8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToG8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToB8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToA8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB8ToR8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToG8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGBAfToRfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToGfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToBfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBAfToAfInfo       (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToRfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToGfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBfToBfInfo        (void)(float*  __restrict dest, const float* __restrict src, size_t count)
#define RGBA8ToRA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToGA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToBA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define Extract8x2Info      (void)(uint8_t* const * __restrict dest, const uint16_t* __restrict src, size_t count)
#define Extract8x3Info      (void)(uint8_t* const * __restrict dest, const uint8_t*  __restrict src, size_t count)
#define Extract8x4Info      (void)(uint8_t* const * __restrict dest, const uint32_t* __restrict src, size_t count)
#define Extract32x2Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Extract32x3Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Extract32x4Info     (void)(float* const * __restrict dest, const float* __restrict src, size_t count)
#define Combine8x2Info      (void)(uint16_t* __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine8x3Info      (void)(uint8_t*  __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine8x4Info      (void)(uint32_t* __restrict dest, const uint8_t* const * __restrict src, size_t count)
#define Combine32x2Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
#define Combine32x3Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
#define Combine32x4Info     (void)(float* __restrict dest, const float* const * __restrict src, size_t count)
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


namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::xziar::img::ColorConvertor;
using ::xziar::img::STBResize;

DEFINE_FASTPATHS(STBResize, DoResize)

DEFINE_FASTPATHS(ColorConvertor, 
    G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGB8, GA8ToRGBA8, GfToGAf, GfToRGBf, GfToRGBAf, GAfToRGBf, GAfToRGBAf,
    RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, RGBfToRGBAf, BGRfToRGBAf, RGBAfToRGBf, RGBAfToBGRf,
    RGB8ToBGR8, RGBA8ToBGRA8, RGBfToBGRf, RGBAfToBGRAf,
    RGBA8ToR8, RGBA8ToG8, RGBA8ToB8, RGBA8ToA8, RGB8ToR8, RGB8ToG8, RGB8ToB8, RGBAfToRf, RGBAfToGf, RGBAfToBf, RGBAfToAf, RGBfToRf, RGBfToGf, RGBfToBf,
    RGBA8ToRA8, RGBA8ToGA8, RGBA8ToBA8,
    Extract8x2, Extract8x3, Extract8x4, Extract32x2, Extract32x3, Extract32x4,
    Combine8x2, Combine8x3, Combine8x4, Combine32x2, Combine32x3, Combine32x4,
    RGB555ToRGB8, BGR555ToRGB8, RGB555ToRGBA8, BGR555ToRGBA8, RGB5551ToRGBA8, BGR5551ToRGBA8,
    RGB565ToRGB8, BGR565ToRGB8, RGB565ToRGBA8, BGR565ToRGBA8,
    RGB10ToRGBf, BGR10ToRGBf, RGB10ToRGBAf, BGR10ToRGBAf, RGB10A2ToRGBAf, BGR10A2ToRGBAf)


#ifdef IMGU_FASTPATH_STB
forceinline void CallStbResize(const ::xziar::img::STBResize::ResizeInfo& info)
{
    ::stbir_resize(info.Input, static_cast<int32_t>(info.InputSizes.first), static_cast<int32_t>(info.InputSizes.second), 0,
        info.Output, static_cast<int32_t>(info.OutputSizes.first), static_cast<int32_t>(info.OutputSizes.second), 0,
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

template<size_t C, typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void ExtractLOOP4(Dst* const __restrict * dest, const Src* __restrict src, size_t count, Args&&... args) noexcept
{
    Proc proc(std::forward<Args>(args)...);
    std::array<Dst* __restrict, C> dsts = { };
    for (size_t i = 0; i < C; ++i)
        dsts[i] = dest[i];
    for (size_t iter = count / (Proc::K * 4); iter > 0; iter--)
    {
        proc(dsts.data(), src + Proc::M * 0);
        for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        proc(dsts.data(), src + Proc::M * 1);
        for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        proc(dsts.data(), src + Proc::M * 2);
        for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        proc(dsts.data(), src + Proc::M * 3);
        for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        src += Proc::M * 4;
    }
    count = count % (Proc::K * 4);
    switch (count / Proc::K)
    {
    case 3: proc(dsts.data(), src); src += Proc::M; for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        [[fallthrough]];
    case 2: proc(dsts.data(), src); src += Proc::M; for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        [[fallthrough]];
    case 1: proc(dsts.data(), src); src += Proc::M; for (size_t i = 0; i < C; ++i) dsts[i] += Proc::N;
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::K;
    if (count)
        F(const_cast<Dst* const*>(dsts.data()), src, count, std::forward<Args>(args)...);
}
template<size_t C, typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void CombineLOOP4(Dst* __restrict dest, const Src* const __restrict * src, size_t count, Args&&... args) noexcept
{
    Proc proc(std::forward<Args>(args)...);
    std::array<const Src* __restrict, C> srcs = { };
    for (size_t i = 0; i < C; ++i)
        srcs[i] = src[i];
    for (size_t iter = count / (Proc::K * 4); iter > 0; iter--)
    {
        proc(dest + Proc::N * 0, srcs.data());
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        proc(dest + Proc::N * 1, srcs.data());
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        proc(dest + Proc::N * 2, srcs.data());
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        proc(dest + Proc::N * 3, srcs.data());
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        dest += Proc::N * 4;
    }
    count = count % (Proc::K * 4);
    switch (count / Proc::K)
    {
    case 3: proc(dest, srcs.data()); dest += Proc::N; 
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        [[fallthrough]];
    case 2: proc(dest, srcs.data()); dest += Proc::N; 
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        [[fallthrough]];
    case 1: proc(dest, srcs.data()); dest += Proc::N; 
        for (size_t i = 0; i < C; ++i) srcs[i] += Proc::M;
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::K;
    if (count)
        F(dest, const_cast<Src* const*>(srcs.data()), count, std::forward<Args>(args)...);
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
DEFINE_FASTPATH_METHOD(GfToGAf, LOOP)
{
#define LOOP_GRAY_GRAYA *dest++ = *src++; *dest++ = alpha
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
#define LOOP_GRAY_RGBA *dest++ = *src; *dest++ = *src; *dest++ = *src; *dest++ = alpha; src++
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
#define LOOP_GRAYA_RGBA *dest++ = *src; *dest++ = *src; *dest++ = *src++; *dest++ = *src++
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
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGBf3To4(float* __restrict dest, const float* __restrict src, size_t count, float alpha) noexcept
{
#define LOOP_RGB_RGBA do { const auto r = src[IdxR]; const auto g = src[IdxG]; const auto b = src[IdxB]; src += 3; *dest++ = r; *dest++ = g; *dest++ = b; * dest++ = alpha; } while(0)
    LOOP8(LOOP_RGB_RGBA)
#undef LOOP_RGB_RGBA
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, LOOP)
{
    RGBf3To4<0, 1, 2>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, LOOP)
{
    RGBf3To4<2, 1, 0>(dest, src, count, alpha);
}
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGBf4To3(float* __restrict dest, const float* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_RGB *dest++ = src[IdxR]; *dest++ = src[IdxG]; *dest++ = src[IdxB]; src += 4
    LOOP8(LOOP_RGBA_RGB)
#undef LOOP_RGBA_RGB
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, LOOP)
{
    RGBf4To3<0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, LOOP)
{
    RGBf4To3<2, 1, 0>(dest, src, count);
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
template<uint8_t Idx>
forceinline void KeepChannelLoop4_1(uint8_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_CH do { const auto val = *src++; *dest++ = static_cast<uint8_t>(val >> (Idx * 8)); } while(0)
    LOOP8(LOOP_RGBA_CH)
#undef LOOP_RGBA_CH
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, LOOP)
{
    KeepChannelLoop4_1<0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, LOOP)
{
    KeepChannelLoop4_1<1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, LOOP)
{
    KeepChannelLoop4_1<2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, LOOP)
{
    KeepChannelLoop4_1<3>(dest, src, count);
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
template<uint8_t Shift>
forceinline void KeepChannelLoop4_2A(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_CHA do { const auto val = *src++; \
if constexpr (Shift < 2) { *dest++ = static_cast<uint16_t>(static_cast<uint8_t>(val >> (Shift * 8)) | ((val >> 16) & 0xff00u)); } \
else { *dest++ = static_cast<uint16_t>(val >> 16); } } while(0)
    LOOP8(LOOP_RGBA_CHA)
#undef LOOP_RGBA_CHA
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, LOOP)
{
    KeepChannelLoop4_2A<0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, LOOP)
{
    KeepChannelLoop4_2A<1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, LOOP)
{
    KeepChannelLoop4_2A<2>(dest, src, count);
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
    LOOP8(LOOP_XXX_RGB)
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
    LOOP8(LOOP_XXX_RGBA)
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
struct KeepChannel4_1_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];

        constexpr uint64_t mask = uint64_t(0xff000000ffu) << (Idx * 8);
        const auto val01 = static_cast<uint32_t>(_pext_u64(dat0, mask));
        const auto val23 = static_cast<uint32_t>(_pext_u64(dat1, mask));
        const auto out = (val23 << 16) | val01;
        reinterpret_cast<uint32_t*>(dst)[0] = out;
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
template<uint32_t Idx>
struct KeepChannel4_2A_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];

        constexpr uint64_t mask = (uint64_t(0xff000000ffu) << (Idx * 8)) | uint64_t(0xff000000ff000000u);
        const auto val01 = static_cast<uint32_t>(_pext_u64(dat0, mask));
        const auto val23 = static_cast<uint32_t>(_pext_u64(dat1, mask));
        reinterpret_cast<uint32_t*>(dst)[0] = val01;
        reinterpret_cast<uint32_t*>(dst)[1] = val23;
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
DEFINE_FASTPATH_METHOD(RGBA8ToR8, BMI2)
{
    ProcessLOOP4<KeepChannel4_1_BMI2<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, BMI2)
{
    ProcessLOOP4<KeepChannel4_1_BMI2<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, BMI2)
{
    ProcessLOOP4<KeepChannel4_1_BMI2<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, BMI2)
{
    ProcessLOOP4<KeepChannel4_1_BMI2<3>, &Func<LOOP>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, BMI2)
{
    ProcessLOOP4<KeepChannel4_2A_BMI2<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, BMI2)
{
    ProcessLOOP4<KeepChannel4_2A_BMI2<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, BMI2)
{
    ProcessLOOP4<KeepChannel4_2A_BMI2<2>, &Func<LOOP>>(dest, src, count);
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

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
struct G2GA_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2GA_128(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        const auto outLo = dat.ZipLo(Alpha).As<U16x8>();
        const auto outHi = dat.ZipHi(Alpha).As<U16x8>();
        outLo.Save(dst);
        outHi.Save(dst + 8);
    }
};
struct G2RGB_128
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
struct G2RGBA_128
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2RGBA_128(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
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
struct Gf2GAf_128
{
    static constexpr size_t M = 4, N = 8, K = 4;
    F32x4 Alpha;
    forceinline Gf2GAf_128(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst + 0);
        out[1].Save(dst + 4);
    }
};
struct Gf2RGBf_128
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
struct Gf2RGBAf_128
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Alpha;
    forceinline Gf2RGBAf_128(float alpha) noexcept : Alpha(alpha) {}
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
struct GAf2RGBf_128
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
struct GAf2RGBAf_128
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
struct RGBAToBGRA_128
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
struct RGBAfToBGRAf_128
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
    ProcessLOOP4<G2GA_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, SIMD128)
{
    ProcessLOOP4<G2RGB_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SIMD128)
{
    ProcessLOOP4<G2RGBA_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToGAf, SIMD128)
{
    ProcessLOOP4<Gf2GAf_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, SIMD128)
{
    ProcessLOOP4<Gf2RGBf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, SIMD128)
{
    ProcessLOOP4<Gf2RGBAf_128, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, SIMD128)
{
    ProcessLOOP4<GAf2RGBf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, SIMD128)
{
    ProcessLOOP4<GAf2RGBAf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, SIMD128)
{
    ProcessLOOP4<RGBAToBGRA_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, SIMD128)
{
    ProcessLOOP4<RGBAfToBGRAf_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, SIMD128)
{
    CombineLOOP4<4, Combine8x4_128, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, SIMD128)
{
    CombineLOOP4<4, Combine32x4_128, &Func<LOOP>>(dest, src, count);
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
struct RGB3To4_SSSE3
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U32x4 Alpha;
    forceinline RGB3To4_SSSE3(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24u) {}
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
struct RGB4To3_SSSE3
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
template<bool IsRGB>
struct RGBf3To4_SSSE3
{
    static constexpr size_t M = 12, N = 16, K = 4;
    F32x4 Alpha;
    forceinline RGBf3To4_SSSE3(float alpha) noexcept : Alpha(alpha) {}
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
struct RGBf4To3_SSE41
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
struct RGBToBGR_SSSE3
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
struct RGBfToBGRf_SSSE3
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
struct KeepChannel4_1_SSSE3
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);
        
        // assume SSE4.1 only has pblendw, which runs on shuffle port, not ALU port like vpblendd
        const auto shuffleMask0 = _mm_setr_epi8(Idx, Idx + 4, Idx + 8, Idx + 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto shuffleMask1 = _mm_setr_epi8(-1, -1, -1, -1, Idx, Idx + 4, Idx + 8, Idx + 12, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto shuffleMask2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, Idx, Idx + 4, Idx + 8, Idx + 12, -1, -1, -1, -1);
        const auto shuffleMask3 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, Idx, Idx + 4, Idx + 8, Idx + 12);

        const U32x4 mid0 = _mm_shuffle_epi8(dat0, shuffleMask0);
        const U32x4 mid1 = _mm_shuffle_epi8(dat1, shuffleMask1);
        const U32x4 mid2 = _mm_shuffle_epi8(dat2, shuffleMask2);
        const U32x4 mid3 = _mm_shuffle_epi8(dat3, shuffleMask3);

        const auto val01 = mid0.Or(mid1);
        const auto val23 = mid2.Or(mid3);
        const auto out = val01.Or(val23).As<U8x16>();
        out.Save(dst);
    }
};
template<int8_t Idx>
struct KeepChannel3_1_SSE41
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat0(src);
        const U8x16 dat1(src + 16);
        const U8x16 dat2(src + 32);

        const auto shuffleMask0 = _mm_setr_epi8(Idx, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx > 0 ? -1 : Idx + 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto shuffleMask1 = _mm_setr_epi8(-1, -1, -1, -1, -1, Idx - 1, Idx + 2, Idx + 5, Idx + 8, Idx + 11, Idx > 1 ? -1 : Idx + 14, -1, -1, -1, -1, -1);
        const auto shuffleMask2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, Idx > 1 ? 0 : -1, Idx + 1, Idx + 4, Idx + 7, Idx + 10, Idx + 13);

        const U8x16 mid0 = _mm_shuffle_epi8(dat0, shuffleMask0);
        const U8x16 mid1 = _mm_shuffle_epi8(dat1, shuffleMask1);
        const U8x16 mid2 = _mm_shuffle_epi8(dat2, shuffleMask2);

        const auto out = mid0.Or(mid1).Or(mid2);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF4_1_SSSE3
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
struct KeepChannelF3_1_SSSE3
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
template<uint8_t Idx>
struct KeepChannel4_2A_SSE41
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);
        
        const U8x16 mid0 = dat0.As<U8x16>().Shuffle<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x16 mid1 = dat1.As<U8x16>().Shuffle<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x16 mid2 = dat2.As<U8x16>().Shuffle<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x16 mid3 = dat3.As<U8x16>().Shuffle<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();

        const auto val01 = mid0.As<U64x2>().ZipLo(mid1.As<U64x2>()).As<U16x8>();
        const auto val23 = mid2.As<U64x2>().ZipLo(mid3.As<U64x2>()).As<U16x8>();
        val01.Save(dst);
        val23.Save(dst + 8);
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
    forceinline void operator()(uint8_t* __restrict * __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat0(src);
        const U8x16 dat1(src + 16);
        const U8x16 dat2(src + 32);

        const U8x16 mid0 = dat0.Shuffle<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // RRRRRRGGGGGBBBBB
        const U8x16 mid1 = dat1.Shuffle<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // GGGGGGBBBBBRRRRR
        const U8x16 mid2 = dat2.Shuffle<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // BBBBBBRRRRRGGGGG
        
        const U8x16 maskLo6  = _mm_setr_epi8(-1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0);
        const U8x16 maskMid6 = _mm_setr_epi8( 0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0);
        const U8x16 maskHi5  = _mm_setr_epi8( 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1);

        const U8x16 rx12 = mid1.MoveToLo<5>().SelectWith<MaskType::FullEle>(mid2.MoveToHi<5>(), maskHi5);  // GBBBBBRRRRRRRRRR
        const U8x16 g12x = mid0.MoveToLo<6>().SelectWith<MaskType::FullEle>(mid1.MoveToHi<5>(), maskMid6); // GGGGGGGGGGG.....
        const U8x16 bx01 = mid0.MoveToLo<5>().SelectWith<MaskType::FullEle>(mid1.MoveToHi<5>(), maskHi5);  // RGGGGGBBBBBBBBBB

        const auto out0 = rx12.SelectWith<MaskType::FullEle>(mid0, maskLo6); // 0000001111122222
        const auto out1 = g12x.SelectWith<MaskType::FullEle>(mid2, maskHi5); // 0000011111122222
        const auto out2 = bx01.MoveToLoWith<6>(mid2); // 0000011111222222

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
struct Combine8x2_SSSE3
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
struct Combine8x3_SSE41
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x16 dat0(src[0]);
        const U8x16 dat1(src[1]);
        const U8x16 dat2(src[2]);

        const U8x16 maskLo6 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const U8x16 maskMid6 = _mm_setr_epi8(0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0);
        const U8x16 maskHi5 = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1);

        const auto dat12Lo = dat1.MoveToHi<6>().SelectWith<MaskType::FullEle>(dat2.MoveToHi<11>(), maskHi5); // 000000GGGGGBBBBB
        const auto val0 = dat12Lo.SelectWith<MaskType::FullEle>(dat0, maskLo6); // RRRRRRGGGGGBBBBB

        const auto dat12Mid = dat2.MoveToHi<1>().SelectWith<MaskType::FullEle>(dat1.MoveToLo<5>(), maskLo6); // GGGGGGBBBBBBBBBB
        const auto val1 = dat12Mid.SelectWith<MaskType::FullEle>(dat0.MoveToHi<5>(), maskHi5); // GGGGGGBBBBBRRRRR

        const auto dat20Hi = dat0.MoveToLo<5>().SelectWith<MaskType::FullEle>(dat2.MoveToLo<10>(), maskLo6); // BBBBBBRRRRR00000
        const auto val2 = dat20Hi.SelectWith<MaskType::FullEle>(dat1, maskHi5); // BBBBBBRRRRRGGGGG

        const auto out0 = val0.Shuffle<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // RGBRGBRGBRGBRGBR
        const auto out1 = val1.Shuffle<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // GBRGBRGBRGBRGBRG
        const auto out2 = val2.Shuffle<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // BRGBRGBRGBRGBRGB

        out0.Save(dst +  0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
struct Combine32x2_SSSE3
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SSSE3)
{
    ProcessLOOP4<G2RGBA_SSSE3, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, SSSE3)
{
    ProcessLOOP4<RGB3To4_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, SSSE3)
{
    ProcessLOOP4<RGB3To4_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, SSSE3)
{
    ProcessLOOP4<RGB4To3_SSSE3<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, SSSE3)
{
    ProcessLOOP4<RGB4To3_SSSE3<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, SSSE3)
{
    ProcessLOOP4<RGBf3To4_SSSE3<true>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, SSSE3)
{
    ProcessLOOP4<RGBf3To4_SSSE3<false>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, SSE41)
{
    ProcessLOOP4<RGBf4To3_SSE41<true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, SSE41)
{
    ProcessLOOP4<RGBf4To3_SSE41<false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, SSSE3)
{
    ProcessLOOP4<RGBToBGR_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, SSSE3)
{
    ProcessLOOP4<RGBfToBGRf_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, SSSE3)
{
    ProcessLOOP4<KeepChannel4_1_SSSE3<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, SSSE3)
{
    ProcessLOOP4<KeepChannel4_1_SSSE3<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, SSSE3)
{
    ProcessLOOP4<KeepChannel4_1_SSSE3<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, SSSE3)
{
    ProcessLOOP4<KeepChannel4_1_SSSE3<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, SSE41)
{
    ProcessLOOP4<KeepChannel3_1_SSE41<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, SSE41)
{
    ProcessLOOP4<KeepChannel3_1_SSE41<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, SSE41)
{
    ProcessLOOP4<KeepChannel3_1_SSE41<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, SSSE3)
{
    ProcessLOOP4<KeepChannelF4_1_SSSE3<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, SSSE3)
{
    ProcessLOOP4<KeepChannelF4_1_SSSE3<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, SSSE3)
{
    ProcessLOOP4<KeepChannelF4_1_SSSE3<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, SSSE3)
{
    ProcessLOOP4<KeepChannelF4_1_SSSE3<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, SSSE3)
{
    ProcessLOOP4<KeepChannelF3_1_SSSE3<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, SSSE3)
{
    ProcessLOOP4<KeepChannelF3_1_SSSE3<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, SSSE3)
{
    ProcessLOOP4<KeepChannelF3_1_SSSE3<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, SSE41)
{
    ProcessLOOP4<KeepChannel4_2A_SSE41<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, SSE41)
{
    ProcessLOOP4<KeepChannel4_2A_SSE41<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, SSE41)
{
    ProcessLOOP4<KeepChannel4_2A_SSE41<2>, &Func<LOOP>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(Combine8x2, SSSE3)
{
    CombineLOOP4<2, Combine8x2_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, SSE41)
{
    CombineLOOP4<3, Combine8x3_SSE41, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, SSSE3)
{
    CombineLOOP4<2, Combine32x2_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, SSSE3)
{
    CombineLOOP4<3, Combine32x3_SSSE3, &Func<LOOP>>(dest, src, count);
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
struct G2GA_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2GA_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x16(src);
        uint8x16x2_t out2;
        out2.val[0] = dat;
        out2.val[1] = Alpha;
        vst2q_u8(reinterpret_cast<uint8_t*>(dst), out2);
    }
};
struct G2RGB_NEON
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
struct G2RGBA_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    U8x16 Alpha;
    forceinline G2RGBA_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
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
struct GA2RGB_NEON
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
struct GA2RGBA_NEON
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
struct Gf2GAf_NEON
{
    static constexpr size_t M = 4, N = 8, K = 4;
    F32x4 Alpha;
    forceinline Gf2GAf_NEON(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x4(src);
        float32x4x2_t out2;
        out2.val[0] = dat;
        out2.val[1] = Alpha;
        vst2q_f32(dst, out2);
    }
};
struct Gf2RGBf_NEON
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
struct Gf2RGBAf_NEON
{
    static constexpr size_t M = 4, N = 16, K = 4;
    F32x4 Alpha;
    forceinline Gf2RGBAf_NEON(float alpha) noexcept : Alpha(alpha) {}
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
struct GAf2RGBf_NEON
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
struct GAf2RGBAf_NEON
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
struct RGB3To4_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U8x16 Alpha;
    forceinline RGB3To4_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
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
struct RGB4To3_NEON
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
struct RGBf3To4_NEON
{
    static constexpr size_t M = 12, N = 16, K = 4;
    F32x4 Alpha;
    forceinline RGBf3To4_NEON(float alpha) noexcept : Alpha(alpha) {}
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
struct RGBf4To3_NEON
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
struct RGBToBGR_NEON
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
struct RGBAToBGRA_NEON
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
struct RGBfToBGRf_NEON
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
struct RGBAfToBGRAf_NEON
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
struct KeepChannel4_1_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        U8x16 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel3_1_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(reinterpret_cast<const uint8_t*>(src));
        U8x16 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF4_1_NEON
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
struct KeepChannelF3_1_NEON
{
    static constexpr size_t M = 12, N = 4, K = 4;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = vld3q_f32(src);
        F32x4 out(dat.val[Idx]);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel4_2A_NEON
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x2_t out2;
        out2.val[0] = dat.val[Idx];
        out2.val[1] = dat.val[3];
        vst2q_u8(reinterpret_cast<uint8_t*>(dst), out2);
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
template<bool IsRGB, bool Is555>
struct RGB5x5ToRGB8_NEON : RGB5x5ToRGB8_128Base<Is555>
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src + 0); // 01234567
        const auto dat1 = U16x8(src + 8); // 89abcdef

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
    static_assert(Is555 || !Is555);
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = U16x8(src + 0); // 01234567
        const auto dat1 = U16x8(src + 8); // 89abcdef

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
            out4.val[3] = datHi.As<I8x16>().ShiftRightArith<7>().As<U8x16>();
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
DEFINE_FASTPATH_METHOD(G8ToGA8, NEON)
{
    ProcessLOOP4<G2GA_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, NEON)
{
    ProcessLOOP4<G2RGB_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, NEON)
{
    ProcessLOOP4<G2RGBA_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, NEON)
{
    ProcessLOOP4<GA2RGB_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, NEON)
{
    ProcessLOOP4<GA2RGBA_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, NEON)
{
    ProcessLOOP4<Gf2GAf_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, NEON)
{
    ProcessLOOP4<Gf2RGBf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, NEON)
{
    ProcessLOOP4<Gf2RGBAf_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, NEON)
{
    ProcessLOOP4<GAf2RGBf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, NEON)
{
    ProcessLOOP4<GAf2RGBAf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, NEON)
{
    ProcessLOOP4<RGB3To4_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, NEON)
{
    ProcessLOOP4<RGB3To4_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, NEON)
{
    ProcessLOOP4<RGB4To3_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, NEON)
{
    ProcessLOOP4<RGB4To3_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, NEON)
{
    ProcessLOOP4<RGBf3To4_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, NEON)
{
    ProcessLOOP4<RGBf3To4_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, NEON)
{
    ProcessLOOP4<RGBf4To3_NEON<0, 1, 2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, NEON)
{
    ProcessLOOP4<RGBf4To3_NEON<2, 1, 0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, NEON)
{
    ProcessLOOP4<RGBToBGR_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, NEON)
{
    ProcessLOOP4<RGBAToBGRA_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, NEON)
{
    ProcessLOOP4<RGBfToBGRf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, NEON)
{
    ProcessLOOP4<RGBAfToBGRAf_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, NEON)
{
    ProcessLOOP4<KeepChannel4_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, NEON)
{
    ProcessLOOP4<KeepChannel4_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, NEON)
{
    ProcessLOOP4<KeepChannel4_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, NEON)
{
    ProcessLOOP4<KeepChannel4_1_NEON<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, NEON)
{
    ProcessLOOP4<KeepChannel3_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, NEON)
{
    ProcessLOOP4<KeepChannel3_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, NEON)
{
    ProcessLOOP4<KeepChannel3_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, NEON)
{
    ProcessLOOP4<KeepChannelF4_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, NEON)
{
    ProcessLOOP4<KeepChannelF4_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, NEON)
{
    ProcessLOOP4<KeepChannelF4_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, NEON)
{
    ProcessLOOP4<KeepChannelF4_1_NEON<3>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, NEON)
{
    ProcessLOOP4<KeepChannelF3_1_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, NEON)
{
    ProcessLOOP4<KeepChannelF3_1_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, NEON)
{
    ProcessLOOP4<KeepChannelF3_1_NEON<2>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, NEON)
{
    ProcessLOOP4<KeepChannel4_2A_NEON<0>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, NEON)
{
    ProcessLOOP4<KeepChannel4_2A_NEON<1>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, NEON)
{
    ProcessLOOP4<KeepChannel4_2A_NEON<2>, &Func<LOOP>>(dest, src, count);
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
struct RGBAToGA_NEONA64
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = U32x4(src +  0).As<U8x16>();
        const auto dat1 = U32x4(src +  4).As<U8x16>();
        const auto dat2 = U32x4(src +  8).As<U8x16>();
        const auto dat3 = U32x4(src + 12).As<U8x16>();

        const U8x16 out0 = vuzp2q_u8(dat0, dat1);
        const U8x16 out1 = vuzp2q_u8(dat2, dat3);
        
        //const auto mid0 = vshrn_n_u16(dat0, 8);
        //const auto mid2 = vshrn_n_u16(dat2, 8);
        //const U8x16 out0 = vshrn_high_n_u16(mid0, dat1, 8);
        //const U8x16 out1 = vshrn_high_n_u16(mid2, dat3, 8);
        
        out0.As<U16x8>().Save(dst);
        out1.As<U16x8>().Save(dst + 8);
    }
};
struct RGBAToBA_NEONA64
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = U32x4(src +  0).As<U16x8>();
        const auto dat1 = U32x4(src +  4).As<U16x8>();
        const auto dat2 = U32x4(src +  8).As<U16x8>();
        const auto dat3 = U32x4(src + 12).As<U16x8>();

        const U16x8 out0 = vuzp2q_u16(dat0, dat1);
        const U16x8 out1 = vuzp2q_u16(dat2, dat3);

        //const auto mid0 = vshrn_n_u32(dat0, 16);
        //const auto mid2 = vshrn_n_u32(dat2, 16);
        //const U16x8 out0 = vshrn_high_n_u32(mid0, dat1, 16);
        //const U16x8 out1 = vshrn_high_n_u32(mid2, dat3, 16);

        out0.Save(dst);
        out1.Save(dst + 8);
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
DEFINE_FASTPATH_METHOD(GfToRGBAf, NEONA64)
{
    ProcessLOOP4<Gf2RGBAf_NEON64, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, NEONA64)
{
    ProcessLOOP4<GAf2RGBAf_NEON64, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, NEONA64)
{
    ProcessLOOP4<RGBAToGA_NEONA64, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, NEONA64)
{
    ProcessLOOP4<RGBAToBA_NEONA64, &Func<LOOP>>(dest, src, count);
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
struct G2GA_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U8x32 Alpha;
    forceinline G2GA_256(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x32(src);
        const auto out = dat.Zip(Alpha);
        out.Data[0].As<U16x16>().Save(dst);
        out.Data[1].As<U16x16>().Save(dst + 16);
    }
};
struct G2GA_256_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U16x16 Alpha;
    forceinline G2GA_256_2(std::byte alpha) noexcept : Alpha(static_cast<uint16_t>(static_cast<uint16_t>(alpha) << 8)) {}
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
struct G2RGB_256
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
struct G2RGBA_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U8x32 Alpha;
    forceinline G2RGBA_256(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
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
struct G2RGBA_256_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U32x8 Alpha;
    forceinline G2RGBA_256_2(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24) {}
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
struct GA2RGB_256
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
struct GA2RGBA_256
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
struct Gf2GAf_256
{
    static constexpr size_t M = 8, N = 16, K = 8;
    F32x8 Alpha;
    forceinline Gf2GAf_256(float alpha) noexcept : Alpha(alpha) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = F32x8(src);
        const auto out = dat.Zip(Alpha);
        out[0].Save(dst + 0);
        out[1].Save(dst + 8);
    }
};
struct Gf2RGBf_256
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
struct Gf2RGBAf_256
{
    static constexpr size_t M = 8, N = 32, K = 8;
    F32x8 Alpha;
    forceinline Gf2RGBAf_256(float alpha) noexcept : Alpha(alpha) {}
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
struct GAf2RGBf_256
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
struct GAf2RGBAf_256
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
struct RGB3To4_256
{
    static constexpr size_t M = 96, N = 32, K = 32;
    U32x8 Alpha;
    forceinline RGB3To4_256(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24u) {}
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
struct RGB4To3_256
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
struct RGBf3To4_256
{
    static constexpr size_t M = 24, N = 32, K = 8;
    F32x8 Alpha;
    forceinline RGBf3To4_256(float alpha) noexcept : Alpha(alpha) {}
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
struct RGBf4To3_256
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
struct RGBToBGR_256
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
struct RGBToBGR_256_2
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
struct RGBAToBGRA_256
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
struct RGBfToBGRf_256
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
struct RGBAfToBGRAf_256
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
template<int8_t Idx>
struct KeepChannel3_1_256
{
    static constexpr size_t M = 96, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x32 dat0(src);
        const U8x32 dat1(src + 32);
        const U8x32 dat2(src + 64);

        const auto shuffleMask0 = _mm256_setr_epi8(
            Idx + 0, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx > 0 ? -1 : Idx + 15,      -1,      -1,      -1,       -1,                      -1, -1, -1, -1, -1, -1,
                 -1,      -1,      -1,      -1,       -1,                 Idx - 1, Idx + 2, Idx + 5, Idx + 8, Idx + 11, Idx > 1 ? -1 : Idx + 14, -1, -1, -1, -1, -1);
        const auto shuffleMask1 = _mm256_setr_epi8(
                 -1,      -1,      -1,      -1,       -1,                      -1,      -1,      -1,      -1,       -1, Idx > 1 ? 0 : -1, Idx + 1, Idx + 4, Idx + 7, Idx + 10, Idx + 13,
            Idx + 0, Idx + 3, Idx + 6, Idx + 9, Idx + 12, Idx > 0 ? -1 : Idx + 15,      -1,      -1,      -1,       -1,               -1,      -1,      -1,      -1,       -1,       -1);
        const auto shuffleMask2 = _mm256_setr_epi8(
                 -1,      -1,      -1,      -1,       -1, Idx - 1, Idx + 2, Idx + 5, Idx + 8, Idx + 11, Idx > 1 ? -1 : Idx + 14,      -1,      -1,      -1,       -1,       -1,
                 -1,      -1,      -1,      -1,       -1,      -1,      -1,      -1,      -1,       -1, Idx > 1 ?  0 :       -1, Idx + 1, Idx + 4, Idx + 7, Idx + 10, Idx + 13);

        const U8x32 mid0l1l = _mm256_shuffle_epi8(dat0, shuffleMask0);
        const U8x32 mid2l0h = _mm256_shuffle_epi8(dat1, shuffleMask1);
        const U8x32 mid1h2h = _mm256_shuffle_epi8(dat2, shuffleMask2);

        const auto mix0l2h = mid0l1l.PermuteLane<0, 3>(mid1h2h);
        const auto mix1l1h = mid0l1l.PermuteLane<1, 2>(mid1h2h);

        const auto out = mix0l2h.Or(mid2l0h).Or(mix1l1h);
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel4_1_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x8 dat0(src);
        const U32x8 dat1(src + 8);
        const U32x8 dat2(src + 16);
        const U32x8 dat3(src + 24);

        const U8x32 mid0 = dat0.As<U8x32>().ShuffleLane<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        const U8x32 mid1 = dat1.As<U8x32>().ShuffleLane<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        const U8x32 mid2 = dat2.As<U8x32>().ShuffleLane<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        const U8x32 mid3 = dat3.As<U8x32>().ShuffleLane<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();

        const auto val01 = mid0.As<U32x8>().SelectWith<0b10101010>(mid1.As<U32x8>()); // 0,2,0,2 1,3,1,3
        const auto val23 = mid2.As<U32x8>().SelectWith<0b10101010>(mid3.As<U32x8>()); // 4.6.4.6 5,7,5,7
        const auto val0123 = val01.SelectWith<0b11001100>(val23); // 0,2,4,6 1,3,5,7
        const auto out = val0123.Shuffle<0, 4, 1, 5, 2, 6, 3, 7>().As<U8x32>();

        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannelF4_1_256
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
struct KeepChannelF3_1_256
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
template<uint8_t Idx>
struct KeepChannel4_2A_256
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x8 dat0(src);
        const U32x8 dat1(src + 8);
        const U32x8 dat2(src + 16);
        const U32x8 dat3(src + 24);

        const U8x32 mid0 = dat0.As<U8x32>().ShuffleLane<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x32 mid1 = dat1.As<U8x32>().ShuffleLane<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x32 mid2 = dat2.As<U8x32>().ShuffleLane<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();
        const U8x32 mid3 = dat3.As<U8x32>().ShuffleLane<Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15, Idx, 3, Idx + 4, 7, Idx + 8, 11, Idx + 12, 15>();

        const auto val01 = mid0.As<U64x4>().SelectWith<0b1010>(mid1.As<U64x4>()); // 0,2 1,3
        const auto val23 = mid2.As<U64x4>().SelectWith<0b1010>(mid3.As<U64x4>()); // 4.6 5,7
        const auto out0 = val01.Shuffle<0, 2, 1, 3>().As<U16x16>();
        const auto out1 = val23.Shuffle<0, 2, 1, 3>().As<U16x16>();

        out0.Save(dst);
        out1.Save(dst + 16);
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
        const U8x32 dat0(src);
        const U8x32 dat1(src + 32);
        const U8x32 dat2(src + 64);

        const U8x32 mid0_ = dat0.ShuffleLane<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // RRRRRRGGGGGBBBBB | GGGGGGBBBBBRRRRR
        const U8x32 mid1_ = dat1.ShuffleLane<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // BBBBBBRRRRRGGGGG | RRRRRRGGGGGBBBBB
        const U8x32 mid2_ = dat2.ShuffleLane<0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14>(); // GGGGGGBBBBBRRRRR | BBBBBBRRRRRGGGGG

        const U8x32 mid0 = mid0_.PermuteLane<0, 3>(mid1_); // RRRRRRGGGGGBBBBB
        const U8x32 mid1 = mid0_.PermuteLane<1, 2>(mid2_); // GGGGGGBBBBBRRRRR
        const U8x32 mid2 = mid1_.PermuteLane<0, 3>(mid2_); // BBBBBBRRRRRGGGGG

        const U8x32 maskLo6  = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0);
        const U8x32 maskMid6 = _mm256_setr_epi8( 0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0);
        const U8x32 maskHi5  = _mm256_setr_epi8( 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1);

        const U8x32 rx12 = mid1.MoveLaneToLo<5>().SelectWith<MaskType::FullEle>(mid2.MoveLaneToHi<5>(), maskHi5);  // GBBBBBRRRRRRRRRR
        const U8x32 g12x = mid0.MoveLaneToLo<6>().SelectWith<MaskType::FullEle>(mid1.MoveLaneToHi<5>(), maskMid6); // GGGGGGGGGGG.....
        const U8x32 bx01 = mid0.MoveLaneToLo<5>().SelectWith<MaskType::FullEle>(mid1.MoveLaneToHi<5>(), maskHi5);  // RGGGGGBBBBBBBBBB

        const auto out0 = rx12.SelectWith<MaskType::FullEle>(mid0, maskLo6); // 0000001111122222
        const auto out1 = g12x.SelectWith<MaskType::FullEle>(mid2, maskHi5); // 0000011111122222
        const auto out2 = bx01.MoveLaneToLoWith<6>(mid2); // 0000011111222222

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
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const U8x32 dat0(src[0]);
        const U8x32 dat1(src[1]);
        const U8x32 dat2(src[2]);

        const U8x32 maskLo6 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const U8x32 maskMid6 = _mm256_setr_epi8(0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0);
        const U8x32 maskHi5 = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1);

        const auto dat12Lo = dat1.MoveLaneToHi<6>().SelectWith<MaskType::FullEle>(dat2.MoveLaneToHi<11>(), maskHi5); // 000000GGGGGBBBBB
        const auto val0 = dat12Lo.SelectWith<MaskType::FullEle>(dat0, maskLo6); // RRRRRRGGGGGBBBBB

        const auto dat12Mid = dat2.MoveLaneToHi<1>().SelectWith<MaskType::FullEle>(dat1.MoveLaneToLo<5>(), maskLo6); // GGGGGGBBBBBBBBBB
        const auto val1 = dat12Mid.SelectWith<MaskType::FullEle>(dat0.MoveLaneToHi<5>(), maskHi5); // GGGGGGBBBBBRRRRR

        const auto dat20Hi = dat0.MoveLaneToLo<5>().SelectWith<MaskType::FullEle>(dat2.MoveLaneToLo<10>(), maskLo6); // BBBBBBRRRRR00000
        const auto val2 = dat20Hi.SelectWith<MaskType::FullEle>(dat1, maskHi5); // BBBBBBRRRRRGGGGG

        const auto out03 = val0.ShuffleLane<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // RGBRGBRGBRGBRGBR
        const auto out14 = val1.ShuffleLane<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // GBRGBRGBRGBRGBRG
        const auto out25 = val2.ShuffleLane<0, 6, 11, 1, 7, 12, 2, 8, 13, 3, 9, 14, 4, 10, 15, 5>(); // BRGBRGBRGBRGBRGB

        const auto out0 = out03.PermuteLane<0, 2>(out14);
        const auto out1 = out25.PermuteLane<0, 3>(out03);
        const auto out2 = out14.PermuteLane<1, 3>(out25);

        out0.Save(dst + 0);
        out1.Save(dst + 32);
        out2.Save(dst + 64);
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
template<bool IsRGB, bool HasAlpha, bool Is555>
struct RGB5x5ToRGBA8_256 : RGB5x5ToRGB8_256Base<Is555>
{
    static_assert(Is555 || !Is555);
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat(src); // 01234567 | 89abcdef

        const auto [midR, midG, midB] = this->RGB5x5ToRGB8Hi(dat);

        const U16x16 valLo = (IsRGB ? midR : midB).template As<U8x32>().ZipEven(midG.template As<U8x32>()).template As<U16x16>();
        U16x16 valHi;
        if constexpr (HasAlpha)
        {
            const auto midA = dat.As<I16x16>().ShiftRightArith<7>().As<U16x16>();
            valHi = (IsRGB ? midB : midR).template As<U8x32>().ZipEven(midA.As<U8x32>()).template As<U16x16>();
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
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX2)
{
    ProcessLOOP4<G2GA_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX22)
{
    ProcessLOOP4<G2GA_256_2, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX2)
{
    ProcessLOOP4<G2RGB_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX2)
{
    ProcessLOOP4<G2RGBA_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX22)
{
    ProcessLOOP4<G2RGBA_256_2, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, AVX2)
{
    ProcessLOOP4<GA2RGB_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX2)
{
    ProcessLOOP4<GA2RGBA_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, AVX)
{
    ProcessLOOP4<Gf2GAf_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, AVX)
{
    ProcessLOOP4<Gf2RGBf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, AVX)
{
    ProcessLOOP4<Gf2RGBAf_256, &Func<SIMD128>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, AVX)
{
    ProcessLOOP4<GAf2RGBf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, AVX)
{
    ProcessLOOP4<GAf2RGBAf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB3To4_256<0, 1, 2>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGR8ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB3To4_256<2, 1, 0>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX2)
{
    ProcessLOOP4<RGB4To3_256<0, 1, 2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGR8, AVX2)
{
    ProcessLOOP4<RGB4To3_256<2, 1, 0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, AVX)
{
    ProcessLOOP4<RGBf3To4_256<true>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, AVX)
{
    ProcessLOOP4<RGBf3To4_256<false>, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, AVX2)
{
    ProcessLOOP4<RGBf4To3_256<true>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, AVX2)
{
    ProcessLOOP4<RGBf4To3_256<false>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX2)
{
    ProcessLOOP4<RGBToBGR_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX22)
{
    ProcessLOOP4<RGBToBGR_256_2, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, AVX2)
{
    ProcessLOOP4<RGBAToBGRA_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, AVX)
{
    ProcessLOOP4<RGBfToBGRf_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, AVX)
{
    ProcessLOOP4<RGBAfToBGRAf_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, AVX2)
{
    ProcessLOOP4<KeepChannel4_1_256<0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, AVX2)
{
    ProcessLOOP4<KeepChannel4_1_256<1>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, AVX2)
{
    ProcessLOOP4<KeepChannel4_1_256<2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, AVX2)
{
    ProcessLOOP4<KeepChannel4_1_256<3>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, AVX2)
{
    ProcessLOOP4<KeepChannel3_1_256<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, AVX2)
{
    ProcessLOOP4<KeepChannel3_1_256<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, AVX2)
{
    ProcessLOOP4<KeepChannel3_1_256<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, AVX2)
{
    ProcessLOOP4<KeepChannelF4_1_256<0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, AVX2)
{
    ProcessLOOP4<KeepChannelF4_1_256<1>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, AVX2)
{
    ProcessLOOP4<KeepChannelF4_1_256<2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, AVX2)
{
    ProcessLOOP4<KeepChannelF4_1_256<3>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, AVX2)
{
    ProcessLOOP4<KeepChannelF3_1_256<0>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, AVX2)
{
    ProcessLOOP4<KeepChannelF3_1_256<1>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, AVX2)
{
    ProcessLOOP4<KeepChannelF3_1_256<2>, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, AVX2)
{
    ProcessLOOP4<KeepChannel4_2A_256<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, AVX2)
{
    ProcessLOOP4<KeepChannel4_2A_256<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, AVX2)
{
    ProcessLOOP4<KeepChannel4_2A_256<2>, &Func<SSE41>>(dest, src, count);
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
    CombineLOOP4<2, Combine8x2_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x3, AVX2)
{
    CombineLOOP4<3, Combine8x3_256, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine8x4, AVX2)
{
    CombineLOOP4<4, Combine8x4_256, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x2, AVX2)
{
    CombineLOOP4<2, Combine32x2_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x3, AVX2)
{
    CombineLOOP4<3, Combine32x3_256, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Combine32x4, AVX2)
{
    CombineLOOP4<4, Combine32x4_256, &Func<SIMD128>>(dest, src, count);
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
struct G2GA_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2GA_512(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
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
struct G2RGB_512
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
struct G2RGBA_512
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2RGBA_512(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
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
struct GA2RGBA_512
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src); // 0~1f
        const auto shuffleMaskLo = _mm512_set_epi8(
            7, 6, 6, 6, 5, 4, 4, 4, 3, 2, 2, 2, 1, 0, 0, 0,
            7, 6, 6, 6, 5, 4, 4, 4, 3, 2, 2, 2, 1, 0, 0, 0,
            7, 6, 6, 6, 5, 4, 4, 4, 3, 2, 2, 2, 1, 0, 0, 0,
            7, 6, 6, 6, 5, 4, 4, 4, 3, 2, 2, 2, 1, 0, 0, 0);
        const auto shuffleMaskHi = _mm512_set_epi8(
            15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10, 9, 8, 8, 8,
            15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10, 9, 8, 8, 8,
            15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10, 9, 8, 8, 8,
            15, 14, 14, 14, 13, 12, 12, 12, 11, 10, 10, 10, 9, 8, 8, 8);
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
struct Gf2GAf_512
{
    static constexpr size_t M = 16, N = 32, K = 16;
    __m512 Alpha;
    forceinline Gf2GAf_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_ps(src);
        const auto out0 = _mm512_mask_permutexvar_ps(Alpha, 0x5555, _mm512_set_epi32( 7,  7,  6,  6,  5,  5,  4,  4,  3,  3,  2,  2,  1,  1,  0,  0), dat);
        const auto out1 = _mm512_mask_permutexvar_ps(Alpha, 0x5555, _mm512_set_epi32(15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10,  9,  9,  8,  8), dat);
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
    }
};
struct Gf2RGBf_512
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
struct Gf2RGBAf_512
{
    static constexpr size_t M = 16, N = 64, K = 16;
    __m512 Alpha;
    forceinline Gf2RGBAf_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
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
struct GAf2RGBf_512
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
struct GAf2RGBAf_512
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
struct RGBf3To4_512
{
    static constexpr size_t M = 48, N = 64, K = 16;
    __m512 Alpha;
    forceinline RGBf3To4_512(float alpha) noexcept : Alpha(_mm512_set1_ps(alpha)) {}
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);

        const auto out0 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(0, B + 9, G + 9, R + 9, 0, B + 6, G + 6, R + 6, 0, B + 3, G + 3, R + 3, 0, B + 0, G + 0, R + 0), dat0);
        const auto mid01 = _mm512_mask_blend_ps(0x00ff, dat0, dat1);
        const auto out1 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(0, B + 5, G + 5, R + 5, 0, B + 2, G + 2, R + 2, 0, (B + 15) % 16, (G + 15) % 16, (R + 15) % 16, 0, B + 12, G + 12, R + 12), mid01);
        const auto mid12 = _mm512_mask_blend_ps(0x00ff, dat1, dat2);
        const auto out2 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(0, B + 1, G + 1, R + 1, 0, (B + 14) % 16, (G + 14) % 16, (R + 14) % 16, 0, B + 11, G + 11, R + 11, 0, B + 8, G + 8, R + 8), mid12);
        const auto out3 = _mm512_mask_permutexvar_ps(Alpha, 0x7777, _mm512_set_epi32(0, B + 13, G + 13, R + 13, 0, B + 10, G + 10, R + 10, 0, B +  7, G +  7, R +  7, 0, B +  4, G +  4, R +  4), dat2);
        
        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
        _mm512_storeu_ps(dst + 48, out3);
    }
};
template<uint8_t R, uint8_t G, uint8_t B>
struct RGBf4To3_512
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
struct RGBfToBGRf_512
{
    static constexpr size_t M = 48, N = 48, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src +  0);
        const auto dat1 = _mm512_loadu_ps(src + 16);
        const auto dat2 = _mm512_loadu_ps(src + 32);

        const auto out0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2), dat1);
        const auto out2 = _mm512_permutex2var_ps(dat2, _mm512_set_epi32(13, 14, 15, 10, 11, 12,  7,  8,  9,  4,  5,  6,  1,  2,  3, 30), dat1);
        const auto mix02 = _mm512_mask_blend_ps(0xff00, dat2, dat0);
        const auto out1 = _mm512_permutex2var_ps(dat1, _mm512_set_epi32(15, 16, 11, 12, 13,  8,  9, 10,  5,  6,  7,  2,  3,  4, 31,  0), mix02);

        _mm512_storeu_ps(dst +  0, out0);
        _mm512_storeu_ps(dst + 16, out1);
        _mm512_storeu_ps(dst + 32, out2);
    }
};
struct RGBAfToBGRAf_512
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
struct KeepChannel4_1_512
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        auto mid0 = dat0, mid1 = dat1;
        if constexpr (Idx > 0)
        {
            mid0 = _mm512_srli_epi32(mid0, Idx * 8);
            mid1 = _mm512_srli_epi32(mid1, Idx * 8);
        }
        const U8x16 out0 = _mm512_cvtepi32_epi8(mid0);
        const U8x16 out1 = _mm512_cvtepi32_epi8(mid1);

        out0.Save(dst);
        out1.Save(dst + 16);
    }
};
template<uint8_t Idx>
struct KeepChannelF4_1_512
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
struct KeepChannelF3_1_512
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
template<uint8_t Idx>
struct KeepChannel4_2A_512
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);
        __m512i out;

        if constexpr (Idx != 2)
        {
            const auto shufMask = _mm512_set_epi8(
                -1, -1, -1, -1, -1, -1, -1, -1, 15, Idx + 12, 11, Idx + 8, 7, Idx + 4, 3, Idx,
                -1, -1, -1, -1, -1, -1, -1, -1, 15, Idx + 12, 11, Idx + 8, 7, Idx + 4, 3, Idx,
                -1, -1, -1, -1, -1, -1, -1, -1, 15, Idx + 12, 11, Idx + 8, 7, Idx + 4, 3, Idx,
                -1, -1, -1, -1, -1, -1, -1, -1, 15, Idx + 12, 11, Idx + 8, 7, Idx + 4, 3, Idx);
            const auto mid0 = _mm512_shuffle_epi8(dat0, shufMask);
            const auto mid1 = _mm512_shuffle_epi8(dat1, shufMask);

            const auto combineMask = _mm512_set_epi64(14, 12, 10, 8, 6, 4, 2, 0);
            out = _mm512_permutex2var_epi64(mid0, combineMask, mid1);
        }
        else
        {
            const auto combineMask = _mm512_set_epi16(
                63, 61, 59, 57, 55, 53, 51, 49,
                47, 45, 43, 41, 39, 37, 35, 33,
                31, 29, 27, 25, 23, 21, 19, 17,
                15, 13, 11,  9,  7,  5,  3,  1);
            out = _mm512_permutex2var_epi16(dat0, combineMask, dat1);
        }

        _mm512_storeu_epi64(dst, out);
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

        const auto shuffleMask = _mm512_set_epi8(
            15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0,
            15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0,
            15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0,
            15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);
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

        const auto shuffleMask = _mm512_set_epi8(
            15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0,
            15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0,
            15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0,
            15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
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

        const auto mid0 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(-1, -1, -1, -1, -1, 30, 27, 24, 21, 18, 15, 12,  9,  6,  3,  0), dat1);
        const auto mid1 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(-1, -1, -1, -1, -1, 31, 28, 25, 22, 19, 16, 13, 10,  7,  4,  1), dat1);
        const auto mid2 = _mm512_permutex2var_ps(dat0, _mm512_set_epi32(-1, -1, -1, -1, -1, -1, 29, 26, 23, 20, 17, 14, 11,  8,  5,  2), dat1);

        const auto out0 = _mm512_mask_permutexvar_ps(mid0, 0b1111100000000000, _mm512_set_epi32(13, 10, 7, 4, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1), dat2);
        const auto out1 = _mm512_mask_permutexvar_ps(mid1, 0b1111100000000000, _mm512_set_epi32(14, 11, 8, 5, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1), dat2);
        const auto out2 = _mm512_mask_permutexvar_ps(mid2, 0b1111110000000000, _mm512_set_epi32(15, 12, 9, 6, 3,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1), dat2);

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
struct Combine32x2_512
{
    static constexpr size_t M = 16, N = 32, K = 16;
    forceinline void operator()(float* __restrict dst, const float* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_ps(src[0]);
        const auto dat1 = _mm512_loadu_ps(src[1]);

        const auto mixLo = _mm512_castps_pd(_mm512_unpacklo_ps(dat0, dat1)); // 0246
        const auto mixHi = _mm512_castps_pd(_mm512_unpackhi_ps(dat0, dat1)); // 1357

        const auto out0 = _mm512_castpd_ps(_mm512_permutex2var_pd(mixLo, _mm512_set_epi64(11, 10, 3, 2,  9,  8, 1, 0), mixHi));
        const auto out1 = _mm512_castpd_ps(_mm512_permutex2var_pd(mixLo, _mm512_set_epi64(15, 14, 7, 6, 13, 12, 5, 4), mixHi));

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
    static_assert(Is555 || !Is555);
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi16(src + 0);

        const auto [midR, midG, midB] = this->RGB5x5ToRGB8Hi(dat);

        const auto shuffleToLo = _mm512_set_epi8(
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1);
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

        const auto shuffleRG2 = _mm512_set_epi32(-1, 31, 15, -1, 30, 14, -1, 29, 13, -1, 28, 12, -1, 27, 11, -1);
        const auto shuffleRG1 = _mm512_set_epi32(26, 10, -1, 25,  9, -1, 24,  8, -1, 23,  7, -1, 22,  6, -1, 21);
        const auto shuffleRG0 = _mm512_set_epi32( 5, -1, 20,  4, -1, 19,  3, -1, 18,  2, -1, 17,  1, -1, 16,  0);
        const auto valRG_0 = _mm512_permutex2var_ps(valR, shuffleRG0, valG);
        const auto valRG_1 = _mm512_permutex2var_ps(valR, shuffleRG1, valG);
        const auto valRG_2 = _mm512_permutex2var_ps(valR, shuffleRG2, valG);

        const auto shuffleB2 = _mm512_set_epi32(15, -1, -1, 14, -1, -1, 13, -1, -1, 12, -1, -1, 11, -1, -1, 10);
        const auto shuffleB1 = _mm512_set_epi32(-1, -1,  9, -1, -1,  8, -1, -1,  7, -1, -1,  6, -1, -1,  5, -1);
        const auto shuffleB0 = _mm512_set_epi32(-1,  4, -1, -1,  3, -1, -1,  2, -1, -1,  1, -1, -1,  0, -1, -1);
        const auto out0 = _mm512_mask_permutexvar_ps(valRG_0, 0b0100100100100100, shuffleB0, valB);
        const auto out1 = _mm512_mask_permutexvar_ps(valRG_1, 0b0010010010010010, shuffleB1, valB);
        const auto out2 = _mm512_mask_permutexvar_ps(valRG_2, 0b1001001001001001, shuffleB2, valB);

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
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX512BW)
{
    ProcessLOOP4<G2GA_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX512BW)
{
    ProcessLOOP4<G2RGB_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512BW)
{
    ProcessLOOP4<G2RGBA_512, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX512BW)
{
    ProcessLOOP4<GA2RGBA_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToGAf, AVX512BW)
{
    ProcessLOOP4<Gf2GAf_512, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GfToRGBf, AVX512BW)
{
    ProcessLOOP4<Gf2RGBf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GfToRGBAf, AVX512BW)
{
    ProcessLOOP4<Gf2RGBAf_512, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GAfToRGBf, AVX512BW)
{
    ProcessLOOP4<GAf2RGBf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(GAfToRGBAf, AVX512BW)
{
    ProcessLOOP4<GAf2RGBAf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRGBAf, AVX512BW)
{
    ProcessLOOP4<RGBf3To4_512<0, 1, 2>, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(BGRfToRGBAf, AVX512BW)
{
    ProcessLOOP4<RGBf3To4_512<2, 1, 0>, &Func<AVX>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBAfToRGBf, AVX512BW)
{
    ProcessLOOP4<RGBf4To3_512<0, 1, 2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRf, AVX512BW)
{
    ProcessLOOP4<RGBf4To3_512<2, 1, 0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBGRf, AVX512BW)
{
    ProcessLOOP4<RGBfToBGRf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBGRAf, AVX512BW)
{
    ProcessLOOP4<RGBAfToBGRAf_512, &Func<AVX>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_1_512<3>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToRf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF4_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToGf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF4_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToBf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF4_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBAfToAf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF4_1_512<3>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToRf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF3_1_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToGf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF3_1_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBfToBf, AVX512BW)
{
    ProcessLOOP4<KeepChannelF3_1_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_2A_512<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_2A_512<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, AVX512BW)
{
    ProcessLOOP4<KeepChannel4_2A_512<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512BW)
{
    ExtractLOOP4<2, Extract8x2_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x2, AVX512BW2)
{
    ExtractLOOP4<2, Extract8x2_512_2, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512BW)
{
    ExtractLOOP4<4, Extract8x4_512, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(Extract8x4, AVX512BW2)
{
    ExtractLOOP4<4, Extract8x4_512_2, &Func<AVX2>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(Combine8x4, AVX512BW)
{
    CombineLOOP4<4, Combine8x4_512, &Func<AVX2>>(dest, src, count);
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
struct G2RGB_512VBMI
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
struct G2RGBA_512VBMI
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2RGBA_512VBMI(std::byte alpha) noexcept : Alpha(_mm512_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(alpha) << 24))) {}
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
struct GA2RGBA_512VBMI
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
struct RGB2RGBA_512VBMI
{
    static constexpr size_t M = 192, N = 64, K = 64;
    __m512i Alpha;
    forceinline RGB2RGBA_512VBMI(std::byte alpha) noexcept : Alpha(_mm512_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(alpha) << 24))) {}
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
struct RGBA2RGB_512VBMI
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
struct RGBToBGR_512VBMI
{
    static constexpr size_t M = 192, N = 192, K = 64;
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src +   0);
        const auto dat1 = _mm512_loadu_epi8(src +  64);
        const auto dat2 = _mm512_loadu_epi8(src + 128);

        const auto shuffleMask0 = _mm512_set_epi8(
            65, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 
            45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 
            31, 32, 27, 28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 
            17, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2);
        const auto out0 = _mm512_permutex2var_epi8(dat0, shuffleMask0, dat1);

        const auto mix02 = _mm512_mask_blend_epi64(0xf0, dat2, dat0);
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
struct RGBAToBGRA_512VBMI
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
struct KeepChannel4_1_512VBMI
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        const auto shufMask = _mm512_castsi256_si512(_mm256_setr_epi8(
            Idx +  0, Idx +  4, Idx +  8, Idx + 12, Idx + 16, Idx + 20, Idx + 24, Idx + 28, Idx + 32, Idx +  36, Idx +  40, Idx +  44, Idx +  48, Idx +  52, Idx +  56, Idx +  60,
            Idx + 64, Idx + 68, Idx + 72, Idx + 76, Idx + 80, Idx + 84, Idx + 88, Idx + 92, Idx + 96, Idx + 100, Idx + 104, Idx + 108, Idx + 112, Idx + 116, Idx + 120, Idx + 124));

        const U8x32 out = _mm512_castsi512_si256(_mm512_permutex2var_epi8(dat0, shufMask, dat1));
        out.Save(dst);
    }
};
template<uint8_t Idx>
struct KeepChannel3_1_512VBMI
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
template<uint8_t Idx>
struct KeepChannel4_2A_512VBMI
{
    static constexpr size_t M = 32, N = 32, K = 32;
    forceinline void operator()(uint16_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi32(src);
        const auto dat1 = _mm512_loadu_epi32(src + 16);

        const auto combineMask = _mm512_set_epi8(
            127, Idx + 124, 123, Idx + 120, 119, Idx + 116, 115, Idx + 112, 111, Idx + 108, 107, Idx + 104, 103, Idx + 100,  99, Idx +  96,
             95, Idx +  92,  91, Idx +  88,  87, Idx +  84,  83, Idx +  80,  79, Idx +  76,  75, Idx +  72,  71, Idx +  68,  67, Idx +  64,
             63, Idx +  60,  59, Idx +  56,  55, Idx +  52,  51, Idx +  48,  47, Idx +  44,  43, Idx +  40,  39, Idx +  36,  35, Idx +  32,
             31, Idx +  28,  27, Idx +  24,  23, Idx +  20,  19, Idx +  16,  15, Idx +  12,  11, Idx +   8,   7, Idx +   4,   3, Idx +   0);
        
        const auto out = _mm512_permutex2var_epi8(dat0, combineMask, dat1);
        _mm512_storeu_epi16(dst, out);
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
             0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
             0,  0,  0,  0,  0, 126, 123, 120, 117, 114, 111, 108, 105, 102, 99, 96,
            93, 90, 87, 84, 81,  78,  75,  72,  69,  66,  63,  60,  57,  54, 51, 48,
            45, 42, 39, 36, 33,  30,  27,  24,  21,  18,  15,  12,   9,   6,  3,  0);
        const auto val0 = _mm512_permutex2var_epi8(dat0, shuffle0Mask, dat1);
        const auto insert0Mask = _mm512_set_epi8(
            61, 58, 55, 52, 49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16,
            13, 10,  7,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0);
        const auto out0 = _mm512_mask_permutexvar_epi8(val0, maskRG, insert0Mask, dat2);

        const auto shuffle1Mask = _mm512_set_epi8(
             0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
             0,  0,  0,  0,  0, 127, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97,
            94, 91, 88, 85, 82,  79,  76,  73,  70,  67,  64,  61,  58,  55,  52, 49,
            46, 43, 40, 37, 34,  31,  28,  25,  22,  19,  16,  13,  10,   7,   4,  1);
        const auto val1 = _mm512_permutex2var_epi8(dat0, shuffle1Mask, dat1);
        const auto insert1Mask = _mm512_set_epi8(
            62, 59, 56, 53, 50, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17,
            14, 11,  8,  5,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0);
        const auto out1 = _mm512_mask_permutexvar_epi8(val1, maskRG, insert1Mask, dat2);

        const auto shuffle2Mask = _mm512_set_epi8(
             0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,
             0,  0,  0,  0,  0,  0, 125, 122, 119, 116, 113, 110, 107, 104, 101, 98,
            95, 92, 89, 86, 83, 80,  77,  74,  71,  68,  65,  62,  59,  56,  53, 50,
            47, 44, 41, 38, 35, 32,  29,  26,  23,  20,  17,  14,  11,   8,   5,  2);
        const auto val2 = _mm512_permutex2var_epi8(dat0, shuffle2Mask, dat1);
        const auto insert2Mask = _mm512_set_epi8(
            63, 60, 57, 54, 51, 48, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18,
            15, 12,  9,  6,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0);
        const auto out2 = _mm512_mask_permutexvar_epi8(val2, maskB, insert2Mask, dat2);

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
    forceinline void operator()(uint8_t* __restrict dst, const uint8_t* __restrict* __restrict src) const noexcept
    {
        const auto dat0 = _mm512_loadu_epi8(src[0]);
        const auto dat1 = _mm512_loadu_epi8(src[1]);
        const auto dat2 = _mm512_loadu_epi8(src[2]);

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

        const auto shuffleToLo = _mm512_set_epi8(
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1,
            15, 15, 13, 13, 11, 11, 9, 9, 7, 7, 5, 5, 3, 3, 1, 1);
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
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<G2RGB_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<G2RGBA_512VBMI, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<GA2RGBA_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX512VBMI)
{
    ProcessLOOP4<RGB2RGBA_512VBMI, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX512VBMI)
{
    ProcessLOOP4<RGBA2RGB_512VBMI, &Func<SSSE3>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, AVX512VBMI)
{
    ProcessLOOP4<RGBToBGR_512VBMI, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, AVX512VBMI)
{
    ProcessLOOP4<RGBAToBGRA_512VBMI, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToR8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_1_512VBMI<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToG8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_1_512VBMI<1>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToB8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_1_512VBMI<2>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToA8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_1_512VBMI<3>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI<0>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI<1>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel3_1_512VBMI<2>, &Func<SSE41>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRA8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_2A_512VBMI<0>, &Func<AVX2>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, AVX512VBMI)
{
    ProcessLOOP4<KeepChannel4_2A_512VBMI<1>, &Func<AVX2>>(dest, src, count);
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
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (defined(__AVX512VBMI2__) || COMMON_COMPILER_MSVC)
#   pragma message("Compiling ColorConvert with AVX512-VBMI2")
struct G2GA_512VBMI2
{
    static constexpr size_t M = 64, N = 64, K = 64;
    __m512i Alpha;
    forceinline G2GA_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
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
struct RGB2RGBA_512VBMI2
{
    static constexpr size_t M = 96, N = 32, K = 32;
    __m512i Alpha;
    forceinline RGB2RGBA_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {

        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto out0 = _mm512_mask_expandloadu_epi8(Alpha, mask, src);
        const auto out1 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 48);
        _mm512_storeu_si512(dst     , out0);
        _mm512_storeu_si512(dst + 16, out1);
    }
};
struct RGBA2RGB_512VBMI2
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
    ProcessLOOP4<G2GA_512VBMI2, &Func<AVX2>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, AVX512VBMI2)
{
    ProcessLOOP4<RGB2RGBA_512VBMI2, &Func<SSSE3>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, AVX512VBMI2)
{
    ProcessLOOP4<RGBA2RGB_512VBMI2, &Func<SSSE3>>(dest, src, count);
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
