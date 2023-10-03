
#include "FastPathCategory.h"
#include "ColorConvert.h"
#include "common/simd/SIMD.hpp"


#define G8ToGA8Info     (void)(uint16_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha)
#define G8ToRGB8Info    (void)(uint8_t*  __restrict dest, const uint8_t* __restrict src, size_t count)
#define G8ToRGBA8Info   (void)(uint32_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha)


namespace
{
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
using ::xziar::img::ColorConvertor;


DEFINE_FASTPATHS(ColorConvertor, G8ToGA8, G8ToRGB8, G8ToRGBA8)


template<typename Proc, auto F, typename Src, typename Dst, typename... Args>
forceinline void ProcessLOOP4(Dst* __restrict dest, const Src* __restrict src, size_t count, Args&&... args) noexcept
{
    Proc proc(std::forward<Args>(args)...);
    for (size_t iter = count / (Proc::M * 4); iter > 0; iter--)
    {
        proc(dest + Proc::N * 0, src + Proc::M * 0);
        proc(dest + Proc::N * 1, src + Proc::M * 1);
        proc(dest + Proc::N * 2, src + Proc::M * 2);
        proc(dest + Proc::N * 3, src + Proc::M * 3);
        src += Proc::M * 4; dest += Proc::N * 4;
    }
    count = count % (Proc::M * 4);
    switch (count / Proc::M)
    {
    case 3: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    case 2: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    case 1: proc(dest, src); src += Proc::M; dest += Proc::N;
        [[fallthrough]];
    default: break;
    }
    count = count % Proc::M;
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


#if COMMON_ARCH_X86 && (defined(__BMI2__) || COMMON_COMPILER_MSVC)
#   pragma message("Compiling ColorConvert with BMI2")
struct G2GA_BMI2
{
    static constexpr size_t M = 4, N = 4;
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
DEFINE_FASTPATH_METHOD(G8ToGA8, BMI2)
{
    ProcessLOOP4<G2GA_BMI2, &Func<LOOP>>(dest, src, count, alpha);
}
struct G2RGB_BMI2
{
    static constexpr size_t M = 8, N = 24;
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
DEFINE_FASTPATH_METHOD(G8ToRGB8, BMI2)
{
    ProcessLOOP4<G2RGB_BMI2, &Func<LOOP>>(dest, src, count);
}
struct G2RGBA_BMI2
{
    static constexpr size_t M = 4, N = 4;
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, BMI2)
{
    ProcessLOOP4<G2RGBA_BMI2, &Func<LOOP>>(dest, src, count, alpha);
}
#endif

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41 && COMMON_SIMD_LV < 100) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 20)
struct G2GA_128
{
    static constexpr size_t M = 16, N = 16;
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
DEFINE_FASTPATH_METHOD(G8ToGA8, SIMD128)
{
    ProcessLOOP4<G2GA_128, &Func<LOOP>>(dest, src, count, alpha);
}
struct G2RGB_128
{
    static constexpr size_t M = 16, N = 48;
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
DEFINE_FASTPATH_METHOD(G8ToRGB8, SIMD128)
{
    ProcessLOOP4<G2RGB_128, &Func<LOOP>>(dest, src, count);
}
struct G2RGBA_128
{
    static constexpr size_t M = 16, N = 16;
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SIMD128)
{
    ProcessLOOP4<G2RGBA_128, &Func<LOOP>>(dest, src, count, alpha);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31
struct G2RGBA_SSSE3
{
    static constexpr size_t M = 16, N = 16;
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, SSSE3)
{
    ProcessLOOP4<G2RGBA_SSSE3, &Func<LOOP>>(dest, src, count, alpha);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
struct G2GA_256
{
    static constexpr size_t M = 32, N = 32;
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
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX2)
{
    ProcessLOOP4<G2GA_256, &Func<LOOP>>(dest, src, count, alpha);
}
struct G2RGB_256
{
    static constexpr size_t M = 32, N = 96;
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
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX2)
{
    ProcessLOOP4<G2RGB_256, &Func<LOOP>>(dest, src, count);
}
struct G2RGBA_256
{
    static constexpr size_t M = 32, N = 32;
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX2)
{
    ProcessLOOP4<G2RGBA_256, &Func<LOOP>>(dest, src, count, alpha);
}
struct G2RGBA_256_2
{
    static constexpr size_t M = 32, N = 32;
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX22)
{
    ProcessLOOP4<G2RGBA_256_2, &Func<LOOP>>(dest, src, count, alpha);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320
struct G2GA_512
{
    static constexpr size_t M = 64, N = 64;
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
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX512BW)
{
    ProcessLOOP4<G2GA_512, &Func<AVX2>>(dest, src, count, alpha);
}
struct G2RGB_512
{
    static constexpr size_t M = 64, N = 192;
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
DEFINE_FASTPATH_METHOD(G8ToRGB8, AVX512BW)
{
    ProcessLOOP4<G2RGB_512, &Func<AVX2>>(dest, src, count);
}
struct G2RGBA_512
{
    static constexpr size_t M = 64, N = 64;
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
        //const auto mask = _cvtu64_mask64(0x7777777777777777u);
        //const auto rgba0123 = _mm512_mask_unpacklo_epi8(Alpha, mask, rgLo, rgLo); // [*0~*3]
        //const auto rgba4567 = _mm512_mask_unpackhi_epi8(Alpha, mask, rgLo, rgLo); // [*4~*7]
        //const auto rgba89ab = _mm512_mask_unpacklo_epi8(Alpha, mask, rgHi, rgHi); // [*8~*b]
        //const auto rgbacdef = _mm512_mask_unpackhi_epi8(Alpha, mask, rgHi, rgHi); // [*c~*f]
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
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512BW)
{
    ProcessLOOP4<G2RGBA_512, &Func<AVX2>>(dest, src, count, alpha);
}
struct G2RGBA_512_2
{
    static constexpr size_t M = 64, N = 64;
    __m512i Alpha;
    forceinline G2RGBA_512_2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto dat = _mm512_loadu_epi8(src); // 0~3f
        const auto shuffleMask1 = _mm512_set_epi8(-1,  3,  3,  3, -1,  2,  2,  2, -1,  1,  1,  1, -1,  0,  0,  0, -1,  3,  3,  3, -1,  2,  2,  2, -1,  1,  1,  1, -1,  0,  0,  0, -1,  3,  3,  3, -1,  2,  2,  2, -1,  1,  1,  1, -1,  0,  0,  0, -1,  3,  3,  3, -1,  2,  2,  2, -1,  1,  1,  1, -1,  0,  0,  0);
        const auto shuffleMask2 = _mm512_set_epi8(-1,  7,  7,  7, -1,  6,  6,  6, -1,  5,  5,  5, -1,  4,  4,  4, -1,  7,  7,  7, -1,  6,  6,  6, -1,  5,  5,  5, -1,  4,  4,  4, -1,  7,  7,  7, -1,  6,  6,  6, -1,  5,  5,  5, -1,  4,  4,  4, -1,  7,  7,  7, -1,  6,  6,  6, -1,  5,  5,  5, -1,  4,  4,  4);
        const auto shuffleMask3 = _mm512_set_epi8(-1, 11, 11, 11, -1, 10, 10, 10, -1,  9,  9,  9, -1,  8,  8,  8, -1, 11, 11, 11, -1, 10, 10, 10, -1,  9,  9,  9, -1,  8,  8,  8, -1, 11, 11, 11, -1, 10, 10, 10, -1,  9,  9,  9, -1,  8,  8,  8, -1, 11, 11, 11, -1, 10, 10, 10, -1,  9,  9,  9, -1,  8,  8,  8);
        const auto shuffleMask4 = _mm512_set_epi8(-1, 15, 15, 15, -1, 14, 14, 14, -1, 13, 13, 13, -1, 12, 12, 12, -1, 15, 15, 15, -1, 14, 14, 14, -1, 13, 13, 13, -1, 12, 12, 12, -1, 15, 15, 15, -1, 14, 14, 14, -1, 13, 13, 13, -1, 12, 12, 12, -1, 15, 15, 15, -1, 14, 14, 14, -1, 13, 13, 13, -1, 12, 12, 12);
        const auto mask = _cvtu64_mask64(0x7777777777777777u);
        const auto rgba0123 = _mm512_mask_shuffle_epi8(Alpha, mask, dat, shuffleMask1);
        const auto rgba4567 = _mm512_mask_shuffle_epi8(Alpha, mask, dat, shuffleMask2);
        const auto rgba89ab = _mm512_mask_shuffle_epi8(Alpha, mask, dat, shuffleMask3);
        const auto rgbacdef = _mm512_mask_shuffle_epi8(Alpha, mask, dat, shuffleMask4);
        const auto mix0 = _mm512_shuffle_i64x2(rgba0123, rgba4567, 0b10001000); // [00~03][20~23][04~07][24~27]
        const auto mix1 = _mm512_shuffle_i64x2(rgba0123, rgba4567, 0b11011101); // [10~13][30~33][14~17][34~37]
        const auto mix2 = _mm512_shuffle_i64x2(rgba89ab, rgbacdef, 0b10001000); // [08~0b][28~2b][0c~0f][2c~2f]
        const auto mix3 = _mm512_shuffle_i64x2(rgba89ab, rgbacdef, 0b11011101); // [18~1b][38~3b][1c~1f][3c~3f]
        const auto out0 = _mm512_shuffle_i64x2(mix0, mix2, 0b10001000); // [00~03][04~07][08~0b][0c~0f]
        const auto out1 = _mm512_shuffle_i64x2(mix1, mix3, 0b10001000); // [10~13][14~17][18~1b][1c~1f]
        const auto out2 = _mm512_shuffle_i64x2(mix0, mix2, 0b11011101); // [20~23][24~27][28~2b][2c~2f]
        const auto out3 = _mm512_shuffle_i64x2(mix1, mix3, 0b11011101); // [30~33][34~37][38~3b][3c~3f]
        _mm512_storeu_si512(dst + 0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512BW2)
{
    ProcessLOOP4<G2RGBA_512_2, &Func<AVX2>>(dest, src, count, alpha);
}
#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (defined(__AVX512VBMI2__) || COMMON_COMPILER_MSVC)
#   pragma message("Compiling ColorConvert with AVX512-VBMI2")
struct G2GA_512VBMI2
{
    static constexpr size_t M = 64, N = 64;
    __m512i Alpha;
    forceinline G2GA_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi8(static_cast<char>(alpha))) {}
    forceinline void operator()(uint16_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x5555555555555555u);
        //__mmask64, 1 from mem, 0 from Alpha
        const auto outLo = _mm512_mask_expandloadu_epi8(Alpha, mask, src);
        const auto outHi = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 32);
        _mm512_storeu_si512(dst     , outLo);
        _mm512_storeu_si512(dst + 32, outHi);
    }
};
DEFINE_FASTPATH_METHOD(G8ToGA8, AVX512VBMI2)
{
    ProcessLOOP4<G2GA_512VBMI2, &Func<AVX2>>(dest, src, count, alpha);
}
struct G2RGBA_512VBMI2
{
    static constexpr size_t M = 64, N = 64;
    __m512i Alpha;
    forceinline G2RGBA_512VBMI2(std::byte alpha) noexcept : Alpha(_mm512_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(alpha) << 24))) {}
    forceinline void operator()(uint32_t* __restrict dst, const uint8_t* __restrict src) const noexcept
    {
        const auto mask = _cvtu64_mask64(0x1111111111111111u);
        //__mmask64, 1 from mem, 0 from Alpha
        const auto mid0 = _mm512_mask_expandloadu_epi8(Alpha, mask, src +  0);
        const auto mid1 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 16);
        const auto mid2 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 32);
        const auto mid3 = _mm512_mask_expandloadu_epi8(Alpha, mask, src + 48);
        const auto shufMask = _mm512_set_epi8(
            15, 12, 12, 12, 11, 8, 8, 8, 7, 4, 4, 4, 3, 0, 0, 0,
            15, 12, 12, 12, 11, 8, 8, 8, 7, 4, 4, 4, 3, 0, 0, 0,
            15, 12, 12, 12, 11, 8, 8, 8, 7, 4, 4, 4, 3, 0, 0, 0,
            15, 12, 12, 12, 11, 8, 8, 8, 7, 4, 4, 4, 3, 0, 0, 0);
        const auto out0 = _mm512_shuffle_epi8(mid0, shufMask);
        const auto out1 = _mm512_shuffle_epi8(mid1, shufMask);
        const auto out2 = _mm512_shuffle_epi8(mid2, shufMask);
        const auto out3 = _mm512_shuffle_epi8(mid3, shufMask);
        _mm512_storeu_si512(dst +  0, out0);
        _mm512_storeu_si512(dst + 16, out1);
        _mm512_storeu_si512(dst + 32, out2);
        _mm512_storeu_si512(dst + 48, out3);
    }
};
DEFINE_FASTPATH_METHOD(G8ToRGBA8, AVX512VBMI2)
{
    ProcessLOOP4<G2RGBA_512VBMI2, &Func<AVX2>>(dest, src, count, alpha);
}
#endif


}
