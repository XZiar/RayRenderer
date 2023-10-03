#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 200
#endif
#include "common/simd/SIMD128.hpp"

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, A64)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,   SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8, SIMD128, LOOP);
}
