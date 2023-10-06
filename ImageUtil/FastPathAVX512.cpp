#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 320
#endif
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, AVX512)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,    AVX512VBMI2, AVX512BW);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,   AVX512VBMI, AVX512BW);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,  AVX512VBMI, AVX512BW);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8, AVX512VBMI, AVX512BW);
}
