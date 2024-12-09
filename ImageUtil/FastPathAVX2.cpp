#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 200
#endif
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

#define IMGU_FASTPATH_STB 1
#define STBIR_AVX2 1
#define STBIR_FP16C 1
#define STBIR_USE_FMA 1

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, AVX2)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,         AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,        AVX2);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,       AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGB8,       AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8,      AVX2); // BMI2 is always slower

    REGISTER_FASTPATH_VARIANTS(G16ToGA16,       AVX2);
    REGISTER_FASTPATH_VARIANTS(G16ToRGB16,      AVX2);
    REGISTER_FASTPATH_VARIANTS(G16ToRGBA16,     AVX2);
    REGISTER_FASTPATH_VARIANTS(GA16ToRGB16,     AVX2);
    REGISTER_FASTPATH_VARIANTS(GA16ToRGBA16,    AVX2);
    
    REGISTER_FASTPATH_VARIANTS(GfToGAf,         AVX);
    REGISTER_FASTPATH_VARIANTS(GfToRGBf,        AVX);
    REGISTER_FASTPATH_VARIANTS(GfToRGBAf,       AVX);
    REGISTER_FASTPATH_VARIANTS(GAfToRGBf,       AVX);
    REGISTER_FASTPATH_VARIANTS(GAfToRGBAf,      AVX);
    
    REGISTER_FASTPATH_VARIANTS(RGB8ToRGBA8,     AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(BGR8ToRGBA8,     AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRGB8,     AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGR8,     AVX2); // BMI2 is always slower
    
    REGISTER_FASTPATH_VARIANTS(RGB16ToRGBA16,   AVX2);
    REGISTER_FASTPATH_VARIANTS(BGR16ToRGBA16,   AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBA16ToRGB16,   AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBA16ToBGR16,   AVX2);
    
    REGISTER_FASTPATH_VARIANTS(RGBfToRGBAf,     AVX);
    REGISTER_FASTPATH_VARIANTS(BGRfToRGBAf,     AVX);
    REGISTER_FASTPATH_VARIANTS(RGBAfToRGBf,     AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBGRf,     AVX2);
    
    REGISTER_FASTPATH_VARIANTS(RGB8ToBGR8,      AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGRA8,    AVX2);

    REGISTER_FASTPATH_VARIANTS(RGB16ToBGR16,    AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBA16ToBGRA16,  AVX2);
    
    REGISTER_FASTPATH_VARIANTS(RGBfToBGRf,      AVX);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBGRAf,    AVX);
    
    REGISTER_FASTPATH_VARIANTS(RGB8ToR8,        AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGB8ToG8,        AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGB8ToB8,        AVX2); // BMI2 is always slower

    REGISTER_FASTPATH_VARIANTS(RGB16ToR16,      AVX2);
    REGISTER_FASTPATH_VARIANTS(RGB16ToG16,      AVX2);
    REGISTER_FASTPATH_VARIANTS(RGB16ToB16,      AVX2);

    REGISTER_FASTPATH_VARIANTS(RGBAfToRf,       AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBAfToGf,       AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBAfToBf,       AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBAfToAf,       AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBfToRf,        AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBfToGf,        AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBfToBf,        AVX2);

    REGISTER_FASTPATH_VARIANTS(Extract8x2,      AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract8x3,      AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(Extract8x4,      AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract16x2,     AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract16x3,     AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(Extract16x4,     AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract32x2,     AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract32x3,     AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Extract32x4,     AVX2, SSSE3);

    REGISTER_FASTPATH_VARIANTS(Combine8x2,      AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(Combine8x3,      AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(Combine8x4,      AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(Combine16x2,     AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(Combine16x3,     AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(Combine16x4,     AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(Combine32x2,     AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(Combine32x3,     AVX2, SSSE3);
    REGISTER_FASTPATH_VARIANTS(Combine32x4,     AVX2, SIMD128);

    REGISTER_FASTPATH_VARIANTS(G8ToG16,         AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(G16ToG8,         AVX2, SSSE3);

    REGISTER_FASTPATH_VARIANTS(RGB555ToRGB8,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGB8,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(RGB555ToRGBA8,   AVX2, SIMD128); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(BGR555ToRGBA8,   AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(RGB5551ToRGBA8,  AVX2, SIMD128); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(BGR5551ToRGBA8,  AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGB8,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGB8,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(RGB565ToRGBA8,   AVX2, SIMD128);
    REGISTER_FASTPATH_VARIANTS(BGR565ToRGBA8,   AVX2, SIMD128);

    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBf,     AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBf,     AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(RGB10ToRGBAf,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(BGR10ToRGBAf,    AVX2, SSE41);
    REGISTER_FASTPATH_VARIANTS(RGB10A2ToRGBAf,  AVX2, AVX);
    REGISTER_FASTPATH_VARIANTS(BGR10A2ToRGBAf,  AVX2, AVX);
}

DEFINE_FASTPATH_PARTIAL(STBResize, AVX2)
{
    REGISTER_FASTPATH_VARIANTS(DoResize, AVX2);
}