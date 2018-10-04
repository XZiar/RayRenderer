#include "oglRely.h"
#include "DataTypeConvert.h"
#include "common/SIMD.hpp"
#include "cpuid/libcpuid.h"

#if defined(__F16C__)
#   pragma message("Compiling OpenGLUtil with F16C support")
#endif

namespace oglu
{
namespace convert
{

static bool CheckF16C()
{
    if (!cpuid_present())               return false;
    struct cpu_raw_data_t raw;
    if (cpuid_get_raw_data(&raw) < 0)   return false;
    struct cpu_id_t data;
    if (cpu_identify(&raw, &data) < 0)  return false;
    if (data.flags[CPU_FEATURE_F16C]) 
        return true;
    else
        return false;
}

void ConvertToHalf(const float* __restrict src, b3d::half* __restrict dst, size_t size)
{
    static const bool INTRIN = CheckF16C();
#if defined(__F16C__) && COMMON_SIMD_LV >= 100
    if (INTRIN)
    {
        while (size > 8)
        {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), _mm256_cvtps_ph(_mm256_loadu_ps(src), 0));
            dst += 8, src += 8, size -= 8;
        }
    }
#endif
    while (size > 0)
    {
        *dst++ = *src++;
    }
}

void ConvertFromHalf(const b3d::half* __restrict src, float* __restrict dst, size_t size)
{
    static const bool INTRIN = CheckF16C();
#if defined(__F16C__) && COMMON_SIMD_LV >= 100
    if (INTRIN)
    {
        while (size > 8)
        {
            _mm256_storeu_ps(dst, _mm256_cvtph_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src))));
            dst += 8, src += 8, size -= 8;
        }
    }
#endif
    while (size > 0)
    {
        *dst++ = *src++;
    }
}

}
}
