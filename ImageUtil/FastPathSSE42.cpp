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

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,    SIMD128, BMI2, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,   SIMD128, BMI2, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,  SIMD128, SSSE3, BMI2, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8, SSSE3, BMI2, LOOP);
}
