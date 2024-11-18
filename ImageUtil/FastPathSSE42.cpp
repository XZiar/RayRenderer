#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 42
#if COMMON_COMPILER_MSVC
#   define __SSE4_2__ 1
#   define __SSE4_1__ 1
#   define __SSSE3__  1
#   define __SSE3__   1
#endif
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 41
#endif
#include "common/simd/SIMD128.hpp"

#define IMGU_FASTPATH_STB 1
#define STBIR_SSE2 1

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,         SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,        SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,       SIMD128, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGB8,       SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8,      SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToRGBA8,     SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR8ToRGBA8,     SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRGB8,     SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGR8,     SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToBGR8,      SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGRA8,    SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToR8,       SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToG8,       SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToB8,       SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToA8,       SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToR8,        SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToG8,        SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToB8,        SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRA8,      SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToGA8,      SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBA8,      SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x2,      SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x3,      SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x4,      SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB555ToRGB8,    SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGB8,    SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB555ToRGBA8,   SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGBA8,   SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB5551ToRGBA8,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR5551ToRGBA8,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGB8,    SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGB8,    SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGBA8,   SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGBA8,   SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBf,     SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBf,     SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBAf,    SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBAf,    SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10A2ToRGBAf,  SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10A2ToRGBAf,  SSE41, LOOP);
}

DEFINE_FASTPATH_PARTIAL(STBResize, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(DoResize, SSE2);
}
