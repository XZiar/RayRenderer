#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 200
#endif
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, AVX2)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,    AVX2);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,   AVX2);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,  AVX2, AVX22);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8, AVX2);
}
