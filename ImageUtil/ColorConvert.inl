
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
#define RGB8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define BGR8ToRGBA8Info     (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define RGBA8ToRGB8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToBGR8Info     (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB8ToBGR8Info      (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGBA8ToBGRA8Info    (void)(uint32_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToR8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToG8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToB8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToA8Info       (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB8ToR8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToG8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGB8ToB8Info        (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define RGBA8ToRA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToGA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGBA8ToBA8Info      (void)(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count)
#define RGB555ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR555ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB555ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR555ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB5551ToRGBA8Info  (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define BGR5551ToRGBA8Info  (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)


namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::xziar::img::ColorConvertor;
using ::xziar::img::STBResize;

DEFINE_FASTPATHS(STBResize, DoResize)

DEFINE_FASTPATHS(ColorConvertor, 
    G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGB8, GA8ToRGBA8,
    RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, 
    RGB8ToBGR8, RGBA8ToBGRA8, 
    RGBA8ToR8, RGBA8ToG8, RGBA8ToB8, RGBA8ToA8, RGB8ToR8, RGB8ToG8, RGB8ToB8, 
    RGBA8ToRA8, RGBA8ToGA8, RGBA8ToBA8,
    RGB555ToRGB8, BGR555ToRGB8, RGB555ToRGBA8, BGR555ToRGBA8, RGB5551ToRGBA8, BGR5551ToRGBA8)


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


DEFINE_FASTPATH_METHOD(G8ToGA8, LOOP)
{
    const auto mask = static_cast<uint16_t>(static_cast<uint8_t>(alpha) << 8);
#define LOOP_GRAY_GRAYA *dest++ = uint16_t(*src++) | mask;
    while (count >= 8)
    {
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        LOOP_GRAY_GRAYA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 6: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 5: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 4: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 3: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 2: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    case 1: LOOP_GRAY_GRAYA;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_GRAY_GRAYA
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, LOOP)
{
#define LOOP_GRAY_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src; src++
    while (count >= 8)
    {
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        LOOP_GRAY_RGB;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 6: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 5: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 4: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 3: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 2: LOOP_GRAY_RGB;
        [[fallthrough]];
    case 1: LOOP_GRAY_RGB;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, LOOP)
{
    const auto mask = static_cast<uint32_t>(static_cast<uint8_t>(alpha) << 24);
#define LOOP_GRAY_RGBA *dest++ = uint32_t(*src++) * 0x010101u | mask
    while (count >= 8)
    {
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        LOOP_GRAY_RGBA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 6: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 5: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 4: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 3: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 2: LOOP_GRAY_RGBA;
        [[fallthrough]];
    case 1: LOOP_GRAY_RGBA;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_GRAY_RGBA
}
DEFINE_FASTPATH_METHOD(GA8ToRGB8, LOOP)
{
#define LOOP_GRAYA_RGB do { const auto tmp = static_cast<uint8_t>(*src++); *dest++ = tmp; *dest++ = tmp; *dest++ = tmp; } while(0)
    while (count >= 8)
    {
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        LOOP_GRAYA_RGB;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 6: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 5: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 4: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 3: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 2: LOOP_GRAYA_RGB;
        [[fallthrough]];
    case 1: LOOP_GRAYA_RGB;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_GRAYA_RGB
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, LOOP)
{
#define LOOP_GRAYA_RGBA do { const uint32_t tmp = *src++; *dest++ = (tmp & 0xffu) * 0x0101u | (tmp << 16u); } while(0)
    while (count >= 8)
    {
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        LOOP_GRAYA_RGBA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 6: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 5: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 4: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 3: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 2: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    case 1: LOOP_GRAYA_RGBA;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_GRAYA_RGBA
}
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB3To4(uint32_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha) noexcept
{
    const uint32_t a = static_cast<uint32_t>(alpha) << 24;
#define LOOP_RGB_RGBA do { const uint32_t r = src[IdxR]; const uint32_t g = src[IdxG]; const uint32_t b = src[IdxB]; src += 3; *dest++ = r | (g << 8) | (b << 16) | a; } while(0)
    while (count >= 8)
    {
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        LOOP_RGB_RGBA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 6: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 5: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 4: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 3: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 2: LOOP_RGB_RGBA;
        [[fallthrough]];
    case 1: LOOP_RGB_RGBA;
        [[fallthrough]];
    default:
        break;
    }
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
    while (count >= 8)
    {
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        LOOP_RGBA_RGB;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 6: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 5: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 4: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 3: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 2: LOOP_RGBA_RGB;
        [[fallthrough]];
    case 1: LOOP_RGBA_RGB;
        [[fallthrough]];
    default:
        break;
    }
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
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, LOOP)
{
#define LOOP_RGB_BGR do { const auto r = *src++; const auto g = *src++; const auto b = *src++; std::atomic_signal_fence(std::memory_order_acquire); *dest++ = b; *dest++ = g; *dest++ = r; } while(0)
    while (count >= 8)
    {
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        LOOP_RGB_BGR;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGB_BGR;
        [[fallthrough]];
    case 6: LOOP_RGB_BGR;
        [[fallthrough]];
    case 5: LOOP_RGB_BGR;
        [[fallthrough]];
    case 4: LOOP_RGB_BGR;
        [[fallthrough]];
    case 3: LOOP_RGB_BGR;
        [[fallthrough]];
    case 2: LOOP_RGB_BGR;
        [[fallthrough]];
    case 1: LOOP_RGB_BGR;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_RGB_BGR
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, LOOP)
{
#define LOOP_RGBA_BGRA do { const uint32_t tmp = *src++; const uint32_t r = (tmp & 0xffu); const uint32_t b = ((tmp >> 16) & 0xffu); *dest++ = (tmp & 0xff00ff00u) | (r << 16u) | b; } while(0)
    while (count >= 8)
    {
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        LOOP_RGBA_BGRA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 6: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 5: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 4: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 3: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 2: LOOP_RGBA_BGRA;
        [[fallthrough]];
    case 1: LOOP_RGBA_BGRA;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_RGBA_BGRA
}
template<uint8_t Shift>
forceinline void KeepChannelLoop4_1(uint8_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_CH do { const auto val = *src++; *dest++ = static_cast<uint8_t>(val >> (Shift * 8)); } while(0)
    while (count >= 8)
    {
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        LOOP_RGBA_CH;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGBA_CH;
        [[fallthrough]];
    case 6: LOOP_RGBA_CH;
        [[fallthrough]];
    case 5: LOOP_RGBA_CH;
        [[fallthrough]];
    case 4: LOOP_RGBA_CH;
        [[fallthrough]];
    case 3: LOOP_RGBA_CH;
        [[fallthrough]];
    case 2: LOOP_RGBA_CH;
        [[fallthrough]];
    case 1: LOOP_RGBA_CH;
        [[fallthrough]];
    default:
        break;
    }
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
template<uint8_t Idx>
forceinline void KeepChannelLoop3_1(uint8_t* __restrict dest, const uint8_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGB_CH do { const auto val = src[Idx]; src += 3; *dest++ = val; } while(0)
    while (count >= 8)
    {
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        LOOP_RGB_CH;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGB_CH;
        [[fallthrough]];
    case 6: LOOP_RGB_CH;
        [[fallthrough]];
    case 5: LOOP_RGB_CH;
        [[fallthrough]];
    case 4: LOOP_RGB_CH;
        [[fallthrough]];
    case 3: LOOP_RGB_CH;
        [[fallthrough]];
    case 2: LOOP_RGB_CH;
        [[fallthrough]];
    case 1: LOOP_RGB_CH;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_RGB_CH
}
DEFINE_FASTPATH_METHOD(RGB8ToR8, LOOP)
{
    KeepChannelLoop3_1<0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToG8, LOOP)
{
    KeepChannelLoop3_1<1>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToB8, LOOP)
{
    KeepChannelLoop3_1<2>(dest, src, count);
}
template<uint8_t Shift>
forceinline void KeepChannelLoop4_2A(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept
{
#define LOOP_RGBA_CHA do { const auto val = *src++; \
if constexpr (Shift < 2) { *dest++ = static_cast<uint16_t>(static_cast<uint8_t>(val >> (Shift * 8)) | ((val >> 16) & 0xff00u)); } \
else { *dest++ = static_cast<uint16_t>(val >> 16); } } while(0)
    while (count >= 8)
    {
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        LOOP_RGBA_CHA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 6: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 5: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 4: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 3: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 2: LOOP_RGBA_CHA;
        [[fallthrough]];
    case 1: LOOP_RGBA_CHA;
        [[fallthrough]];
    default:
        break;
    }
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
template<uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB555ToRGB8(uint8_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    // 00000000000xxxxx R -> xxxxx000 << 3 Idx0
    // 000000xxxxx00000 G -> xxxxx000 >> 2 Idx1
    // 0xxxxx0000000000 B -> xxxxx000 >> 7 Idx2
#define LOOP_555_RGB do { const auto val = *src++;          \
const auto r = static_cast<uint8_t>((val & 0x001fu) << 3);  \
const auto g = static_cast<uint8_t>((val & 0x03e0u) >> 2);  \
const auto b = static_cast<uint8_t>((val & 0x7c00u) >> 7);  \
dest[IdxR] = r; dest[IdxG] = g; dest[IdxB] = b; dest += 3; } while (0)
    while (count >= 8)
    {
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        LOOP_555_RGB;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_555_RGB;
        [[fallthrough]];
    case 6: LOOP_555_RGB;
        [[fallthrough]];
    case 5: LOOP_555_RGB;
        [[fallthrough]];
    case 4: LOOP_555_RGB;
        [[fallthrough]];
    case 3: LOOP_555_RGB;
        [[fallthrough]];
    case 2: LOOP_555_RGB;
        [[fallthrough]];
    case 1: LOOP_555_RGB;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_555_RGB
}
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, LOOP)
{
    RGB555ToRGB8<0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, LOOP)
{
    RGB555ToRGB8<2, 1, 0>(dest, src, count);
}
template<bool TakeAlpha, uint8_t IdxR, uint8_t IdxG, uint8_t IdxB>
forceinline void RGB555ToRGBA8(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept
{
    // 00000000000xxxxx R -> xxxxx000 << 3 Idx0
    // 000000xxxxx00000 G -> xxxxx000 >> 2 Idx1
    // 0xxxxx0000000000 B -> xxxxx000 >> 7 Idx2
#define LOOP_555_RGBA do { const auto val = *src++; uint32_t tmp = (!TakeAlpha || (val & 0x8000u)) ? 0xff000000u : 0x0u; \
tmp |= (val & 0x1fu) << (IdxR * 8 + 3); \
if constexpr (IdxG * 8 > 2) { tmp |= (val & 0x03e0u) << (IdxG * 8 - 2); } else { tmp |= (val & 0x03e0u) >> 2; } \
if constexpr (IdxB * 8 > 7) { tmp |= (val & 0x7c00u) << (IdxB * 8 - 7); } else { tmp |= (val & 0x7c00u) >> 7; } \
*dest++ = tmp; } while(0)
    while (count >= 8)
    {
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        LOOP_555_RGBA;
        count -= 8;
    }
    switch (count)
    {
    case 7: LOOP_555_RGBA;
        [[fallthrough]];
    case 6: LOOP_555_RGBA;
        [[fallthrough]];
    case 5: LOOP_555_RGBA;
        [[fallthrough]];
    case 4: LOOP_555_RGBA;
        [[fallthrough]];
    case 3: LOOP_555_RGBA;
        [[fallthrough]];
    case 2: LOOP_555_RGBA;
        [[fallthrough]];
    case 1: LOOP_555_RGBA;
        [[fallthrough]];
    default:
        break;
    }
#undef LOOP_RGBA_CHA
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, LOOP)
{
    RGB555ToRGBA8<false, 0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, LOOP)
{
    RGB555ToRGBA8<false, 2, 1, 0>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, LOOP)
{
    RGB555ToRGBA8<true, 0, 1, 2>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, LOOP)
{
    RGB555ToRGBA8<true, 2, 1, 0>(dest, src, count);
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
struct RGB555ToRGB8_BMI2
{
    static constexpr size_t M = 8, N = 24, K = 8;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat0 = reinterpret_cast<const uint64_t*>(src)[0];
        const auto dat1 = reinterpret_cast<const uint64_t*>(src)[1];
        
        constexpr uint64_t removeAlphaMask = 0x7fff7fff7fff7fffu; // 4 item-> 60bits
        const auto mid0 = _pext_u64(dat0, removeAlphaMask);
        const auto mid1 = _pext_u64(dat1, removeAlphaMask);
        
        const auto val0 = mid0;
        const auto val1 = mid0 >> 40 | (mid1 << 20);
        const auto val2 = mid1 >> 20;

        constexpr uint64_t expandMask = 0xf8f8f8f8f8f8f8f8u; // 8 element -> use 40bits
        const auto out0 = _pdep_u64(val0, expandMask);
        const auto out1 = _pdep_u64(val1, expandMask);
        const auto out2 = _pdep_u64(val2, expandMask);

        reinterpret_cast<uint64_t*>(dst)[0] = out0;
        reinterpret_cast<uint64_t*>(dst)[1] = out1;
        reinterpret_cast<uint64_t*>(dst)[2] = out2;

    }
};
template<bool TakeAlpha>
struct RGB555ToRGBA8_BMI2
{
    static constexpr size_t M = 4, N = 4, K = 4;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const auto dat = reinterpret_cast<const uint64_t*>(src)[0];
        constexpr uint64_t mask = 0x01f8f8f801f8f8f8u; // ok to leave 1 bit in alpha, will be set to 1 or equal to desired val
        const auto val0 = _pdep_u64(dat, mask);
        const auto val1 = _pdep_u64(dat >> 32, mask);
        if constexpr (TakeAlpha)
        {
            constexpr uint64_t alpahMask = 0x8000800080008000u;
            const auto alphas = static_cast<uint32_t>(_pext_u64(dat, alpahMask));
            CM_ASSUME(alphas <= 0xf);
            constexpr uint64_t AlphaVals[4] = /*00, 01, 10, 11*/
            {
                0x0000000000000000u, 0x00000000ff000000u, 0xff00000000000000u, 0xff000000ff000000u
            };
            const auto out0 = val0 | AlphaVals[alphas & 0x3u];
            const auto out1 = val1 | AlphaVals[alphas >> 2];
            reinterpret_cast<uint64_t*>(dst)[0] = out0;
            reinterpret_cast<uint64_t*>(dst)[1] = out1;
        }
        else
        {
            constexpr uint64_t alpha = 0xff000000ff000000u;
            const auto out0 = val0 | alpha;
            const auto out1 = val1 | alpha;
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
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, BMI2)
{
    ProcessLOOP4<RGB555ToRGB8_BMI2, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, BMI2)
{
    ProcessLOOP4<RGB555ToRGBA8_BMI2<false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, BMI2)
{
    ProcessLOOP4<RGB555ToRGBA8_BMI2<true>, &Func<LOOP>>(dest, src, count);
}
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
template<bool IsRGB>
struct RGB555ToRGB8_128
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        //const auto val = *src;
        //const auto r = static_cast<uint8_t>(((val >>  0) & 0x1fu) << 3);
        //const auto g = static_cast<uint8_t>(((val >>  5) & 0x1fu) << 3);
        //const auto b = static_cast<uint8_t>(((val >> 10) & 0x1fu) << 3);

        constexpr uint8_t OffR = IsRGB ? 0 : 1;
        constexpr uint8_t OffB = IsRGB ? 1 : 0;
        constexpr uint8_t ShiftR = IsRGB ? 3 : 1;
        constexpr uint8_t ShiftB = IsRGB ? 1 : 3;

        const auto dat0 = U16x8(src + 0).As<U8x16>(); // 01234567
        const auto dat1 = U16x8(src + 8).As<U8x16>(); // 89abcdef

        const U8x16 dat0RRB = dat0.Shuffle<OffR + 0, OffR + 0, OffB + 0, OffR + 2, OffR + 2, OffB + 2, OffR + 4, OffR + 4, OffB + 4, OffR + 6, OffR + 6, OffB + 6, OffR + 8, OffR + 8, OffB + 8, OffR + 10>(); // RRB RRB RRB RRB RRB R
        const U8x16 val0RR_ = dat0RRB.As<U16x8>().ShiftLeftLogic<ShiftR>().template As<U8x16>(); // RR_ RR_ RR_ RR_ RR_ R
        const U8x16 val0__B = dat0RRB.As<U16x8>().ShiftLeftLogic<ShiftB>().template As<U8x16>(); // __B __B __B __B __B _
        const U8x16 val0RRB = val0RR_.SelectWith<0b0100100100100100>(val0__B); // RRB RRB RRB RRB RRB R
        //assert((val0RRB.Val[0] & 0xf8u) == r);
        //assert((val0RRB.Val[1] & 0xf8u) == r);
        //assert((val0RRB.Val[2] & 0xf8u) == b);
        const auto dat0G_ = dat0.As<U16x8>().ShiftRightLogic<2>(); // G_G_G_G_G_G_G_G_
        //assert((dat0G_.Val[0] & 0xf8u) == g);
        const auto val0GGG = dat0G_.As<U8x16>().Shuffle<0, 0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 9>(); // GGG GGG GGG GGG GGG _
        //assert((val0GGG.Val[0] & 0xf8u) == g);
        //assert((val0GGG.Val[1] & 0xf8u) == g);
        //assert((val0GGG.Val[2] & 0xf8u) == g);
        const auto val0RGB = val0RRB.SelectWith<0b0010010010010010>(val0GGG); // RGB RGB RGB RGB RGB R
        //assert((val0RGB.Val[0] & 0xf8u) == r);
        //assert((val0RGB.Val[1] & 0xf8u) == g);
        //assert((val0RGB.Val[2] & 0xf8u) == b);

        const auto dat01 = dat0.As<U64x2>().SelectWith<0b01>(dat1.As<U64x2>()).As<U8x16>(); // 89ab4567
        const U8x16 dat1RRB = dat01.Shuffle<OffR + 10, OffB + 10, OffR + 12, OffR + 12, OffB + 12, OffR + 14, OffR + 14, OffB + 14, OffR + 0, OffR + 0, OffB + 0, OffR + 2, OffR + 2, OffB + 2, OffR + 4, OffR + 4>(); // RB RRB RRB RRB RRB RR
        const U8x16 val1RR_ = dat1RRB.As<U16x8>().ShiftLeftLogic<ShiftR>().template As<U8x16>(); // R_ RR_ RR_ RR_ RR_ RR
        const U8x16 val1__B = dat1RRB.As<U16x8>().ShiftLeftLogic<ShiftB>().template As<U8x16>(); // _B __B __B __B __B __
        const U8x16 val1RRB = val1RR_.SelectWith<0b0010010010010010>(val1__B); // RB RRB RRB RRB RRB RR
        const auto dat1G_ = dat01.As<U16x8>().ShiftRightLogic<2>(); // G_G_G_G_G_G_G_G_
        const auto val1GGG = dat1G_.As<U8x16>().Shuffle<10, 10, 12, 12, 12, 14, 14, 14, 0, 0, 0, 2, 2, 2, 4, 4>(); // GG GGG GGG GGG GGG GG
        const auto val1RGB = val1RRB.SelectWith<0b1001001001001001>(val1GGG); // GB RGB RGB RGB RGB RG

        const U8x16 dat2RRB = dat1.Shuffle<OffB + 4, OffR + 6, OffR + 6, OffB + 6, OffR + 8, OffR + 8, OffB + 8, OffR + 10, OffR + 10, OffB + 10, OffR + 12, OffR + 12, OffB + 12, OffR + 14, OffR + 14, OffB + 14>(); // B RRB RRB RRB RRB RRB
        const U8x16 val2RR_ = dat2RRB.As<U16x8>().ShiftLeftLogic<ShiftR>().template As<U8x16>(); // _ RR_ RR_ RR_ RR_ RR_
        const U8x16 val2__B = dat2RRB.As<U16x8>().ShiftLeftLogic<ShiftB>().template As<U8x16>(); // B __B __B __B __B __B
        const U8x16 val2RRB = val2RR_.SelectWith<0b1001001001001001>(val2__B); // B RRB RRB RRB RRB RRB
        const auto dat2G_ = dat1.As<U16x8>().ShiftRightLogic<2>(); // G_G_G_G_G_G_G_G_
        const auto val2GGG = dat2G_.As<U8x16>().Shuffle<5, 6, 6, 6, 8, 8, 8, 10, 10, 10, 12, 12, 12, 14, 14, 14>(); // _ GGG GGG GGG GGG GGG
        const auto val2RGB = val2RRB.SelectWith<0b0100100100100100>(val2GGG); // RGB RGB RGB RGB RGB R

        const U8x16 maskLo3bit(uint8_t(0xf8u));

        const auto out0 = val0RGB.And(maskLo3bit);
        const auto out1 = val1RGB.And(maskLo3bit);
        const auto out2 = val2RGB.And(maskLo3bit);

        out0.Save(dst + 0);
        out1.Save(dst + 16);
        out2.Save(dst + 32);
    }
};
template<bool IsRGB, bool TakeAlpha>
struct RGB555ToRGBA8_128
{
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x8 dat(src);

        const U16x8 maskHi5bit(uint16_t(0xf800u));
        const auto datR_ = dat.ShiftLeftLogic<3>();
        const auto dat_G = dat.ShiftLeftLogic<6>().And(maskHi5bit);
        const U16x8 maskLo3bit(uint16_t(0xfff8u));
        const auto valBA = dat.As<I16x8>().ShiftRightArith<7>().As<U16x8>().And(maskLo3bit); // BA

        U16x8 valLo, valHi;
        if constexpr (IsRGB)
        {
            valLo = datR_.As<U8x16>().SelectWith<0xaaaa>(dat_G.As<U8x16>()).As<U16x8>(); // RG
            if constexpr (!TakeAlpha)
            {
                const U16x8 maskAlpha(uint16_t(0xff00u));
                valHi = valBA.Or(maskAlpha);
            }
            else
                valHi = valBA;
        }
        else
        {
            valLo = valBA.As<U8x16>().SelectWith<0xaaaa>(dat_G.As<U8x16>()).As<U16x8>(); // BG
            if constexpr (!TakeAlpha)
            {
                const U16x8 maskAlpha(uint16_t(0xff00u));
                valHi = datR_.Or(maskAlpha); // RA
            }
            else
                valHi = datR_.As<U8x16>().SelectWith<0xaaaa>(valBA.As<U8x16>()).As<U16x8>(); // RA
        }

        const auto out0 = valLo.ZipLo(valHi);
        const auto out1 = valLo.ZipHi(valHi);
        out0.As<U32x4>().Save(dst);
        out1.As<U32x4>().Save(dst + 4);
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
DEFINE_FASTPATH_METHOD(RGB555ToRGB8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGB8_128<true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGB8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGB8_128<false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGBA8_128<true, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGBA8_128<false, false>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGBA8_128<true, true>, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, SIMD128)
{
    ProcessLOOP4<RGB555ToRGBA8_128<false, true>, &Func<LOOP>>(dest, src, count);
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
struct RGBAToBGRA_SSSE3
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

        //const auto mid0 = dat0.As<U8x16>().Shuffle<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        //const auto mid1 = dat1.As<U8x16>().Shuffle<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        //const auto mid2 = dat2.As<U8x16>().Shuffle<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        //const auto mid3 = dat3.As<U8x16>().Shuffle<Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12, Idx, Idx + 4, Idx + 8, Idx + 12>();
        //
        //const auto val01 = mid0.As<U32x4>().ZipLo(mid1.As<U32x4>());
        //const auto val23 = mid2.As<U32x4>().ZipLo(mid3.As<U32x4>());
        //const auto out = val01.As<U64x2>().ZipLo(val23.As<U64x2>()).As<U8x16>();
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
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, SSSE3)
{
    ProcessLOOP4<RGBToBGR_SSSE3, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, SSSE3)
{
    ProcessLOOP4<RGBAToBGRA_SSSE3, &Func<LOOP>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(RGB8ToBGR8, NEON)
{
    ProcessLOOP4<RGBToBGR_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBGRA8, NEON)
{
    ProcessLOOP4<RGBAToBGRA_NEON, &Func<LOOP>>(dest, src, count);
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
#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 200
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
DEFINE_FASTPATH_METHOD(RGBA8ToGA8, NEONA64)
{
    ProcessLOOP4<RGBAToGA_NEONA64, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGBA8ToBA8, NEONA64)
{
    ProcessLOOP4<RGBAToBA_NEONA64, &Func<LOOP>>(dest, src, count);
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
template<bool IsRGB, bool TakeAlpha>
struct RGB555ToRGBA8_256
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat(src);

        const U16x16 maskHi5bit(uint16_t(0xf800u));
        const auto datR_ = dat.ShiftLeftLogic<3>();
        const auto dat_G = dat.ShiftLeftLogic<6>().And(maskHi5bit);
        const U16x16 maskLo3bit(uint16_t(0xfff8u));
        const auto valBA = dat.As<I16x16>().ShiftRightArith<7>().As<U16x16>().And(maskLo3bit); // BA

        U16x16 valLo, valHi;
        if constexpr (IsRGB)
        {
            valLo = datR_.As<U8x32>().SelectWith<0xaaaaaaaa>(dat_G.As<U8x32>()).As<U16x16>(); // RG
            if constexpr (!TakeAlpha)
            {
                const U16x16 maskAlpha(uint16_t(0xff00u));
                valHi = valBA.Or(maskAlpha);
            }
            else
                valHi = valBA;
        }
        else
        {
            valLo = valBA.As<U8x32>().SelectWith<0xaaaaaaaa>(dat_G.As<U8x32>()).As<U16x16>(); // BG
            if constexpr (!TakeAlpha)
            {
                const U16x16 maskAlpha(uint16_t(0xff00u));
                valHi = datR_.Or(maskAlpha); // RA
            }
            else
                valHi = datR_.As<U8x32>().SelectWith<0xaaaaaaaa>(valBA.As<U8x32>()).As<U16x16>(); // RA
        }

        const auto out = valLo.Zip(valHi).As<U32x8>();
        out.Data[0].Save(dst);
        out.Data[1].Save(dst + 8);

        //const auto out01Lo = valLo.ZipLoLane(valHi);
        //const auto out01Hi = valLo.ZipHiLane(valHi);
        //const U32x8 out0 = _mm256_permute2x128_si256(out01Lo, out01Hi, 0x20);
        //const U32x8 out1 = _mm256_permute2x128_si256(out01Lo, out01Hi, 0x31);
        //out0.Save(dst);
        //out1.Save(dst + 8);
    }
};
template<bool IsRGB, bool TakeAlpha>
struct RGB555ToRGBA8_256_2
{
    static constexpr size_t M = 16, N = 16, K = 16;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
    {
        const U16x16 dat(src);

        const U16x16 maskHi5bit(uint16_t(0xf800u));
        const auto datRx = dat.ShiftLeftLogic<3>();
        const auto dat_G = dat.ShiftLeftLogic<6>().And(maskHi5bit);
        const auto datxBA = dat.As<I16x16>().ShiftRightArith<7>().As<U16x16>();

        const U16x16 maskLo5bit(uint16_t(0x00f8u));
        U16x16 valLo, valHi;
        if constexpr (IsRGB)
        {
            valLo = datRx.And(maskLo5bit).Or(dat_G); // RG
            // valLo = datR_.As<U8x32>().SelectWith<0xaaaaaaaa>(dat_G.As<U8x32>()).As<U16x16>(); // RG
            const U16x16 maskHi13bit(uint16_t(0xfff8u));
            const auto valBA = datxBA.And(maskHi13bit);
            if constexpr (!TakeAlpha)
            {
                const U16x16 maskAlpha(uint16_t(0xff00u));
                valHi = valBA.Or(maskAlpha);
            }
            else
                valHi = valBA;
        }
        else
        {
            const auto valB_ = datxBA.And(maskLo5bit);
            valLo = valB_.Or(dat_G); // BG
            // valLo = valBA.As<U8x32>().SelectWith<0xaaaaaaaa>(dat_G.As<U8x32>()).As<U16x16>(); // BG
            if constexpr (!TakeAlpha)
            {
                const U16x16 maskAlpha(uint16_t(0xff00u));
                valHi = datRx.Or(maskAlpha); // RA
            }
            else
            {
                valHi = datRx.As<U8x32>().SelectWith<0xaaaaaaaa>(datxBA.As<U8x32>()).As<U16x16>(); // RA
            }
        }

        const auto out = valLo.Zip(valHi).As<U32x8>();
        out.Data[0].Save(dst);
        out.Data[1].Save(dst + 8);
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
    ProcessLOOP4<RGBAToBGRA_256, &Func<SSSE3>>(dest, src, count);
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
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB555ToRGBA8_256<true, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB555ToRGBA8_256<false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB555ToRGBA8_256<true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX2)
{
    ProcessLOOP4<RGB555ToRGBA8_256<false, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB555ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB555ToRGBA8_256_2<true, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR555ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB555ToRGBA8_256_2<false, false>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB5551ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB555ToRGBA8_256_2<true, true>, &Func<SIMD128>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(BGR5551ToRGBA8, AVX22)
{
    ProcessLOOP4<RGB555ToRGBA8_256_2<false, true>, &Func<SIMD128>>(dest, src, count);
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
        constexpr uint64_t Mask1 = 0x4924924924924924u << Idx | Mask1Add[Idx];
        const auto mask1 = _cvtu64_mask64(Mask1);
        _mm512_mask_compressstoreu_epi8(dst + (Idx > 0 ? 21 : 22), mask1, dat1);

        constexpr uint32_t Mask2Add[3] = { 0u,0u,1u };
        constexpr uint64_t Mask2 = 0x2492492492492492u << Idx | Mask2Add[Idx];
        const auto mask2 = _cvtu64_mask64(Mask2);
        _mm512_mask_compressstoreu_epi8(dst + (Idx > 1 ? 42 : 43), mask2, dat2);
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
#endif


}
