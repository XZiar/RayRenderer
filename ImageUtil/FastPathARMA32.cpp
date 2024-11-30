#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 100
#endif
#include "common/simd/SIMD128.hpp"

#define IMGU_FASTPATH_STB 1

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, A32)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,         SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,        SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,       SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGB8,       SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8,      SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GfToGAf,         SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GfToRGBf,        SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GfToRGBAf,       SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GAfToRGBf,       SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GAfToRGBAf,      SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToRGBA8,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR8ToRGBA8,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRGB8,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGR8,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBfToRGBAf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGRfToRGBAf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToRGBf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBGRf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToBGR8,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGRA8,    NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBfToBGRf,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBGRAf,    NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToR8,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToG8,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToB8,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToA8,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToR8,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToG8,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToB8,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToRf,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToGf,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBf,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBAfToAf,       NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBfToRf,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBfToGf,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBfToBf,        NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x2,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x3,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract8x4,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract32x2,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract32x3,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Extract32x4,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine8x2,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine8x3,      NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine8x4,      NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine32x2,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine32x3,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(Combine32x4,     NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB555ToRGB8,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGB8,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB555ToRGBA8,   NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGBA8,   NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB5551ToRGBA8,  NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR5551ToRGBA8,  NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGB8,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGB8,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGBA8,   NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGBA8,   NEON, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBf,     NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBAf,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBAf,    NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB10A2ToRGBAf,  NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(BGR10A2ToRGBAf,  NEON, LOOP);
}


DEFINE_FASTPATH_PARTIAL(STBResize, A32)
{
    REGISTER_FASTPATH_VARIANTS(DoResize, NEON);
}
