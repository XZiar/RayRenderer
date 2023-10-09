
#include "FastPathCategory.h"
#include "ColorConvert.h"
#include "common/simd/SIMD.hpp"

#include <bit>

#define G8ToGA8Info     (void)(uint16_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define G8ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count)
#define G8ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define GA8ToRGBA8Info  (void)(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count)
#define RGB8ToRGBA8Info (void)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha)
#define RGBA8ToRGB8Info (void)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count)


namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::xziar::img::ColorConvertor;


DEFINE_FASTPATHS(ColorConvertor, G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGBA8, RGB8ToRGBA8, RGBA8ToRGB8)


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
#define LOOP_GRAY_GRAYA *dest = uint16_t(*src++) | mask; dest++; count--
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
#define LOOP_GRAY_RGB *dest++ = *src; *dest++ = *src; *dest++ = *src; src++; count--
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
#define LOOP_GRAY_RGBA *dest = uint32_t(*src++) * 0x010101u | mask; dest++; count--
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
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, LOOP)
{
#define LOOP_GRAYA_RGBA do { const uint32_t tmp = *src++; *dest++ = (tmp & 0xffu) * 0x0101u | (tmp << 16u); count--; } while(0)
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
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, LOOP)
{
    const uint32_t a = static_cast<uint32_t>(alpha) << 24;
#define LOOP_RGB_RGBA do { const uint32_t r = *src++; const uint32_t g = *src++; const uint32_t b = *src++; *dest++ = r | (g << 8) | (b << 16) | a; count--; } while(0)
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
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, LOOP)
{
#define LOOP_RGB_RGBA do { const auto val = *src++; *dest++ = static_cast<uint8_t>(val); *dest++ = static_cast<uint8_t>(val >> 8); *dest++ = static_cast<uint8_t>(val >> 16); count--; } while(0)
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


#if COMMON_ARCH_X86 && (defined(__BMI2__) || COMMON_COMPILER_MSVC)
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
#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41 && COMMON_SIMD_LV < 100) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
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
struct RGB2RGBA_SSSE3
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U32x4 Alpha;
    forceinline RGB2RGBA_SSSE3(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24u) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const U8x16 dat0(src +  0);
        const U8x16 dat1(src + 16);
        const U8x16 dat2(src + 32);

        const auto shuffleMask0 = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
        const auto shuffleMask1 = _mm_setr_epi8(4, 5, 6, -1, 7, 8, 9, -1, 10, 11, 12, -1, 13, 14, 15, -1);


        const U32x4 mid0 = _mm_shuffle_epi8(dat0, shuffleMask0);
        const auto out0 = mid0.Or(Alpha);
        out0.Save(dst);

        const U8x16 dat1_ = _mm_alignr_epi8(dat1, dat0, 12);
        const U32x4 mid1 = _mm_shuffle_epi8(dat1_, shuffleMask0);
        const auto out1 = mid1.Or(Alpha);
        out1.Save(dst + 4);

        const U8x16 dat2_ = _mm_alignr_epi8(dat2, dat1, 8);
        const U32x4 mid2 = _mm_shuffle_epi8(dat2_, shuffleMask0); 
        const auto out2 = mid2.Or(Alpha);
        out2.Save(dst + 8);

        const U32x4 mid3 = _mm_shuffle_epi8(dat2, shuffleMask1);
        const auto out3 = mid3.Or(Alpha);
        out3.Save(dst + 12);
    }
};
struct RGBA2RGB_SSSE3
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const U32x4 dat0(src);
        const U32x4 dat1(src + 4);
        const U32x4 dat2(src + 8);
        const U32x4 dat3(src + 12);

        const auto mid0123 = dat0.As<U8x16>().Shuffle<0, 0, 0, 0, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14>();
        const auto mid4567 = dat1.As<U8x16>().Shuffle<0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15, 15, 15, 15>();
        const auto mid89ab = dat2.As<U8x16>().Shuffle<0, 0, 0, 0, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14>();
        const auto midcdef = dat3.As<U8x16>().Shuffle<0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15, 15, 15, 15>();
        
        const U8x16 out0 = _mm_alignr_epi8(mid4567, mid0123, 4);
        out0.Save(dst);
        const U8x16 out1 = F32x4(_mm_shuffle_ps(mid4567.As<F32x4>(), mid89ab.As<F32x4>(), 0b10011001)).As<U8x16>();
        out1.Save(dst + 16);
        const U8x16 out2 = _mm_alignr_epi8(midcdef, mid89ab, 12);
        out2.Save(dst + 32);
    }
};
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SSSE3)
{
    ProcessLOOP4<G2RGBA_SSSE3, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, SSSE3)
{
    ProcessLOOP4<RGB2RGBA_SSSE3, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, SSSE3)
{
    ProcessLOOP4<RGBA2RGB_SSSE3, &Func<LOOP>>(dest, src, count);
}
#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
# if COMMON_ARCH_X86
#   define ALGO SSSE3
# else
#   define ALGO SIMD128
# endif
struct PPCAT(GA2RGBA_, ALGO)
{
    static constexpr size_t M = 8, N = 8, K = 8;
    forceinline void operator()(uint32_t* __restrict dst, const uint16_t* __restrict src) const noexcept
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
struct RGB2RGBA_NEON
{
    static constexpr size_t M = 48, N = 16, K = 16;
    U8x16 Alpha;
    forceinline RGB2RGBA_NEON(std::byte alpha) noexcept : Alpha(static_cast<uint8_t>(alpha)) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = vld3q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x4_t out4;
        out4.val[0] = dat.val[0];
        out4.val[1] = dat.val[1];
        out4.val[2] = dat.val[2];
        out4.val[3] = Alpha;
        vst4q_u8(reinterpret_cast<uint8_t*>(dst), out4);
    }
};
struct RGBA2RGB_NEON
{
    static constexpr size_t M = 16, N = 48, K = 16;
    forceinline void operator()(uint8_t* __restrict dst, const uint32_t* __restrict src) const noexcept
    {
        const auto dat = vld4q_u8(reinterpret_cast<const uint8_t*>(src));
        uint8x16x3_t out3;
        out3.val[0] = dat.val[0];
        out3.val[1] = dat.val[1];
        out3.val[2] = dat.val[2];
        vst3q_u8(dst, out3);
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
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, NEON)
{
    ProcessLOOP4<GA2RGBA_NEON, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(RGB8ToRGBA8, NEON)
{
    ProcessLOOP4<RGB2RGBA_NEON, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(RGBA8ToRGB8, NEON)
{
    ProcessLOOP4<RGBA2RGB_NEON, &Func<LOOP>>(dest, src, count);
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
        const auto outLoLane = dat.ZipLoLane(Alpha).As<U16x16>();
        const auto outHiLane = dat.ZipHiLane(Alpha).As<U16x16>();
        const U16x16 outLo = _mm256_permute2x128_si256(outLoLane.Data, outHiLane.Data, 0x20);
        const U16x16 outHi = _mm256_permute2x128_si256(outLoLane.Data, outHiLane.Data, 0x31);
        outLo.Save(dst);
        outHi.Save(dst + 16);
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
        const U8x32 out0 = _mm256_permute2x128_si256(mid0.Data, mid1.Data, 0x20);
        const U8x32 out1 = _mm256_permute2x128_si256(mid2.Data, mid0.Data, 0x30);
        const U8x32 out2 = _mm256_permute2x128_si256(mid1.Data, mid2.Data, 0x31);
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
        const auto rgba0123 = rgLo.ZipLoLane(baLo).As<U32x8>(); // [0~3][10~13]
        const auto rgba4567 = rgLo.ZipHiLane(baLo).As<U32x8>(); // [4~7][14~17]
        const auto rgba89ab = rgHi.ZipLoLane(baHi).As<U32x8>(); // [8~b][18~1b]
        const auto rgbacdef = rgHi.ZipHiLane(baHi).As<U32x8>(); // [c~f][1c~1f]
        const U32x8 out0 = _mm256_permute2x128_si256(rgba0123.Data, rgba4567.Data, 0x20); // [0~3][4~7]
        const U32x8 out1 = _mm256_permute2x128_si256(rgba89ab.Data, rgbacdef.Data, 0x20); // [8~b][c~f]
        const U32x8 out2 = _mm256_permute2x128_si256(rgba0123.Data, rgba4567.Data, 0x31); // [10~13][14~17]
        const U32x8 out3 = _mm256_permute2x128_si256(rgba89ab.Data, rgbacdef.Data, 0x31); // [18~1b][1c~1f]
        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
    }
};
struct G2RGBA_256_2
{
    static constexpr size_t M = 32, N = 32, K = 32;
    U32x8 Alpha;
    forceinline G2RGBA_256_2(std::byte alpha) noexcept : Alpha(static_cast<uint32_t>(alpha) << 24) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = U8x32(src);
        const auto shuffleMask1 = _mm256_setr_epi8( 0,  0,  0, -1,  1,  1,  1, -1,  2,  2,  2, -1,  3,  3,  3, -1,  0,  0,  0, -1,  1,  1,  1, -1,  2,  2,  2, -1,  3,  3,  3, -1);
        const auto shuffleMask2 = _mm256_setr_epi8( 4,  4,  4, -1,  5,  5,  5, -1,  6,  6,  6, -1,  7,  7,  7, -1,  4,  4,  4, -1,  5,  5,  5, -1,  6,  6,  6, -1,  7,  7,  7, -1);
        const auto shuffleMask3 = _mm256_setr_epi8( 8,  8,  8, -1,  9,  9,  9, -1, 10, 10, 10, -1, 11, 11, 11, -1,  8,  8,  8, -1,  9,  9,  9, -1, 10, 10, 10, -1, 11, 11, 11, -1);
        const auto shuffleMask4 = _mm256_setr_epi8(12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1, 12, 12, 12, -1, 13, 13, 13, -1, 14, 14, 14, -1, 15, 15, 15, -1);
        const U32x8 rgb0123 = _mm256_shuffle_epi8(dat, shuffleMask1);
        const U32x8 rgb4567 = _mm256_shuffle_epi8(dat, shuffleMask2);
        const U32x8 rgb89ab = _mm256_shuffle_epi8(dat, shuffleMask3);
        const U32x8 rgbcdef = _mm256_shuffle_epi8(dat, shuffleMask4);
        const auto rgba0123 = rgb0123.Or(Alpha); // [0~3][10~13]
        const auto rgba4567 = rgb4567.Or(Alpha); // [4~7][14~17]
        const auto rgba89ab = rgb89ab.Or(Alpha); // [8~b][18~1b]
        const auto rgbacdef = rgbcdef.Or(Alpha); // [c~f][1c~1f]
        const U32x8 out0 = _mm256_permute2x128_si256(rgba0123.Data, rgba4567.Data, 0x20); // [0~3][4~7]
        const U32x8 out1 = _mm256_permute2x128_si256(rgba89ab.Data, rgbacdef.Data, 0x20); // [8~b][c~f]
        const U32x8 out2 = _mm256_permute2x128_si256(rgba0123.Data, rgba4567.Data, 0x31); // [10~13][14~17]
        const U32x8 out3 = _mm256_permute2x128_si256(rgba89ab.Data, rgbacdef.Data, 0x31); // [18~1b][1c~1f]
        out0.Save(dst);
        out1.Save(dst + 8);
        out2.Save(dst + 16);
        out3.Save(dst + 24);
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
        const U32x8 out0 = _mm256_permute2x128_si256(mid0, mid1, 0x20);
        const U32x8 out1 = _mm256_permute2x128_si256(mid0, mid1, 0x31);
        out0.Save(dst);
        out1.Save(dst + 8);
    }
};
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX2)
{
    ProcessLOOP4<G2GA_256, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX2)
{
    ProcessLOOP4<G2RGB_256, &Func<LOOP>>(dest, src, count);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX2)
{
    ProcessLOOP4<G2RGBA_256, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX22)
{
    ProcessLOOP4<G2RGBA_256_2, &Func<LOOP>>(dest, src, count, alpha);
}
DEFINE_FASTPATH_METHOD(GA8ToRGBA8, AVX2)
{
    ProcessLOOP4<GA2RGBA_256, &Func<LOOP>>(dest, src, count);
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
#endif


}
