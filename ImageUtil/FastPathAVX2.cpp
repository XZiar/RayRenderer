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
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,      AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,     AVX2);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,    AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8,   AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGB8ToRGBA8,  AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(BGR8ToRGBA8,  AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRGB8,  AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGR8,  AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGB8ToBGR8,   AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBGRA8, AVX2);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToR8,    AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToG8,    AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToB8,    AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGBA8ToA8,    AVX2); // BMI2 is always slower
    REGISTER_FASTPATH_VARIANTS(RGB8ToR8,     BMI2);
    REGISTER_FASTPATH_VARIANTS(RGB8ToG8,     BMI2);
    REGISTER_FASTPATH_VARIANTS(RGB8ToB8,     BMI2);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRA8,   AVX2, BMI2);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToGA8,   AVX2, BMI2);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToBA8,   AVX2, BMI2);
}

DEFINE_FASTPATH_PARTIAL(STBResize, AVX2)
{
    REGISTER_FASTPATH_VARIANTS(DoResize, AVX2);
}