#pragma once
#include "../CommonRely.hpp"

#if COMMON_ARCH_X86
# if COMMON_COMPILER_CLANG
#   if COMMON_CLANG_VER < 30000
#     error clang version too low to use this header, at least clang 3.0.0
#   endif
#   include <x86intrin.h>
#   if COMMON_CLANG_VER < 30601
#     define CMSIMD_WA_SET128       1 // https://reviews.llvm.org/D9855
#   endif
#   if COMMON_CLANG_VER < 30801
#     define CMSIMD_WA_LOADUSI64    1 // https://reviews.llvm.org/D21504
#   endif
# elif COMMON_COMPILER_ICC
#   if COMMON_ICC_VER < 130000
#     error ICC version too low to use this header, at least icc 13.0
#   endif
#   include <x86intrin.h>
#   if COMMON_ICC_VER < 160000
#     define CMSIMD_WA_LOADUSI64    1
#     define CMSIMD_WA_LOADUSI      1
#   endif
#   if COMMON_ICC_VER < 170000
#     define CMSIMD_WA_ZEXTAVX      1
#   endif
# elif COMMON_COMPILER_GCC
#   if COMMON_GCC_VER < 40500
#     error GCC version too low to use this header, at least gcc 4.5.0
#   endif
#   include <x86intrin.h>
#   if COMMON_GCC_VER < 80000
#     define CMSIMD_WA_SET128       1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80582
#   endif
#   if COMMON_GCC_VER < 90000
#     define CMSIMD_WA_LOADUSI64    1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78782
#   endif
#   if COMMON_GCC_VER < 100000
#     define CMSIMD_WA_ZEXTAVX      1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83250
#   endif
#   if COMMON_GCC_VER < 100100
#     define CMSIMD_WA_LOADU2       1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91341
#   endif
#   if COMMON_GCC_VER < 110000
#     define CMSIMD_WA_LOADUSI      1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95483
#   endif
#   if COMMON_GCC_VER < 110300 || (COMMON_GCC_VER >= 120000 && COMMON_GCC_VER < 120100)
#     define CMSIMD_FIX_LOADUSI     1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99754
#   endif
# elif COMMON_COMPILER_MSVC
#   if COMMON_MSVC_VER < 120000
#     error MSVC version too low to use this header, at least msvc 12.0 (VS6.0)
#   endif
#   include <intrin.h>
#   if (_M_IX86_FP == 1) && !defined(__SSE__)
#     define __SSE__ 1
#   elif (_M_IX86_FP == 2) && !defined(__SSE2__)
#     define __SSE2__ 1
#   endif
#   if COMMON_MSVC_VER < 191400 
#     define CMSIMD_WA_ZEXTAVX      1 // https://developercommunity.visualstudio.com/t/missing-zero-extension-avx-and-avx512-intrinsics/175737
#   endif
# else
#  error Unknown compiler, not supported by this header
# endif
// workrounds
# ifdef CMSIMD_WA_SET128
#   undef CMSIMD_WA_SET128
#   define _mm256_set_m128(/* __m128 */ hi, /* __m128 */ lo)  _mm256_insertf128_ps(_mm256_castps128_ps256(lo), (hi), 0x1)
#   define _mm256_set_m128d(/* __m128d */ hi, /* __m128d */ lo)  _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
#   define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
# endif
# ifdef CMSIMD_WA_LOADU2
#   undef CMSIMD_WA_LOADU2
#   define _mm256_loadu2_m128(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128(_mm_loadu_ps(hi), _mm_loadu_ps(lo))
#   define _mm256_loadu2_m128d(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128d(_mm_loadu_pd(hi), _mm_loadu_pd(lo))
#   define _mm256_loadu2_m128i(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128i(_mm_loadu_si128(hi), _mm_loadu_si128(lo))
#   define _mm256_storeu2_m128(/* float* */ hiaddr, /* float* */ loaddr, /* __m256 */ a) do { __m256 _a = (a); _mm_storeu_ps((loaddr), _mm256_castps256_ps128(_a)); _mm_storeu_ps((hiaddr), _mm256_extractf128_ps(_a, 0x1)); } while (0)
#   define _mm256_storeu2_m128d(/* double* */ hiaddr, /* double* */ loaddr, /* __m256d */ a) do { __m256d _a = (a); _mm_storeu_pd((loaddr), _mm256_castpd256_pd128(_a)); _mm_storeu_pd((hiaddr), _mm256_extractf128_pd(_a, 0x1)); } while (0)
#   define _mm256_storeu2_m128i(/* __m128i* */ hiaddr, /* __m128i* */ loaddr, /* __m256i */ a) do { __m256i _a = (a); _mm_storeu_si128((loaddr), _mm256_castsi256_si128(_a)); _mm_storeu_si128((hiaddr), _mm256_extractf128_si256(_a, 0x1)); } while (0)
# endif
# ifdef CMSIMD_WA_ZEXTAVX
#   undef CMSIMD_WA_ZEXTAVX
#   define _mm256_zextps128_ps256(a) _mm256_insertf128_ps(_mm256_setzero_ps(), a, 0)
#   define _mm256_zextpd128_pd256(a) _mm256_insertf128_pd(_mm256_setzero_pd(), a, 0)
#   define _mm256_zextsi128_si256(a) _mm256_insertf128_si256(_mm256_setzero_si256(), a, 0)
# endif
# ifdef CMSIMD_WA_LOADUSI64
#   undef CMSIMD_WA_LOADUSI64
#   define _mm_loadu_si64(mem_addr) _mm_loadl_epi64((const __m128i*)(mem_addr))
#   define _mm_storeu_si64(mem_addr, a) _mm_storel_epi64((__m128i*)(mem_addr), (a))
# endif
# ifdef CMSIMD_WA_LOADUSI
#   undef CMSIMD_WA_LOADUSI
#   define _mm_loadu_si32(mem_addr) _mm_cvtsi32_si128(*(const int*)(mem_addr))
#   define _mm_loadu_si16(mem_addr) _mm_cvtsi32_si128(*(const short*)(mem_addr))
#   define _mm_storeu_si32(mem_addr, a) (void)(*(int*)(mem_addr) = _mm_cvtsi128_si32((a)))
#   define _mm_storeu_si16(mem_addr, a) (void)(*(short*)(mem_addr) = (short)_mm_cvtsi128_si32((a)))
# endif
# ifdef CMSIMD_FIX_LOADUSI
#   undef CMSIMD_FIX_LOADUSI
#   define _mm_loadu_si32_correct(addr) _mm_set_epi32(0, 0, 0, reinterpret_cast<const int32_t*>(addr)[0])
#   define _mm_loadu_si16_correct(addr) _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, reinterpret_cast<const int16_t*>(addr)[0])
// forceinline __m128i _mm_loadu_si32_correct(const void* __restrict addr) noexcept
// {
//     return _mm_set_epi32(0, 0, 0, reinterpret_cast<const int32_t*>(addr)[0]);
// }
// forceinline __m128i _mm_loadu_si16_correct(const void* __restrict addr) noexcept
// {
//     return _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, reinterpret_cast<const int16_t*>(addr)[0]);
// }
# else
#   define _mm_loadu_si32_correct(addr) _mm_loadu_si32(addr)
#   define _mm_loadu_si16_correct(addr) _mm_loadu_si16(addr)
// forceinline __m128i _mm_loadu_si32_correct(const void* addr) noexcept
// {
//     return _mm_loadu_si32(addr);
// }
// forceinline __m128i _mm_loadu_si16_correct(const void* addr) noexcept
// {
//     return _mm_loadu_si16(addr);
// }
# endif
#elif COMMON_ARCH_ARM
# if COMMON_COMPILER_CLANG
#   if COMMON_CLANG_VER < 30000
#     error clang version too low to use this header, at least clang 3.0.0
#   endif
#   include <arm_neon.h>
# elif COMMON_COMPILER_GCC
#   if COMMON_GCC_VER < 40300
#     error GCC version too low to use this header, at least gcc 4.3.0
#   endif
#   include <arm_neon.h>
# elif COMMON_COMPILER_MSVC
#   if COMMON_MSVC_VER < 191600
#     error MSVC version too low to use this header, at least msvc 19.16 (VS15.9.0)
#   endif
#   include <arm_neon.h>
# else
#  error Unknown compiler, not supported by this header
# endif
#else
#  error Unknown architecture, not supported by this header
#endif

#if COMMON_ARCH_X86

# if defined(__AVX512BW__) && defined(__AVX512DQ__) && defined(__AVX512VL__)
#    define COMMON_SIMD_LV_ 320
# elif defined(__AVX512F__) && defined(__AVX512CD__)
#    define COMMON_SIMD_LV_ 310
# elif defined(__AVX2__)
#    define COMMON_SIMD_LV_ 200
# elif defined(__FMA__)
#    define COMMON_SIMD_LV_ 150
# elif defined(__AVX__)
#    define COMMON_SIMD_LV_ 100
# elif defined(__SSE4_2__)
#    define COMMON_SIMD_LV_ 42
# elif defined(__SSE4_1__)
#    define COMMON_SIMD_LV_ 41
# elif defined(__SSSE3__)
#    define COMMON_SIMD_LV_ 31
# elif defined(__SSE3__)
#    define COMMON_SIMD_LV_ 30
# elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || COMMON_OSBIT >= 64
#    define COMMON_SIMD_LV_ 20
# elif defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#    define COMMON_SIMD_LV_ 10
# else
#    define COMMON_SIMD_LV_ 0
# endif
# ifndef COMMON_SIMD_LV
#    define COMMON_SIMD_LV COMMON_SIMD_LV_
# endif


# if COMMON_SIMD_LV >= 320
#   define COMMON_SIMD_INTRIN AVX512
# elif COMMON_SIMD_LV >= 310
#   define COMMON_SIMD_INTRIN AVX512
# elif COMMON_SIMD_LV >= 200
#   define COMMON_SIMD_INTRIN AVX2
# elif COMMON_SIMD_LV >= 150
#   define COMMON_SIMD_INTRIN FMA
# elif COMMON_SIMD_LV >= 100
#   define COMMON_SIMD_INTRIN AVX
# elif COMMON_SIMD_LV >= 42
#   define COMMON_SIMD_INTRIN SSE4_2
# elif COMMON_SIMD_LV >= 41
#   define COMMON_SIMD_INTRIN SSE4_1
# elif COMMON_SIMD_LV >= 31
#   define COMMON_SIMD_INTRIN SSSE3
# elif COMMON_SIMD_LV >= 30
#   define COMMON_SIMD_INTRIN SSE3
# elif COMMON_SIMD_LV >= 20
#   define COMMON_SIMD_INTRIN SSE2
# elif COMMON_SIMD_LV >= 10
#   define COMMON_SIMD_INTRIN SSE
# else
#   define COMMON_SIMD_INTRIN NONE
# endif

#endif


#if COMMON_ARCH_ARM

# if defined(_M_ARM64) || defined(__aarch64__) || defined(__ARM_ARCH_ISA_A64)
// aarch64 expose more feature
#   define COMMON_SIMD_LV_ 200
# elif defined(__ARM_ARCH) && __ARM_ARCH >= 8
// The ARMv8-A architecture has made many ARMv7-A optional features mandatory, including advanced SIMD (also called NEON).
// aarch32 add extra feature
#   define COMMON_SIMD_LV_ 100
# elif defined(__ARM_NEON)
#   if defined(__ARM_VFPV5__)
#     define COMMON_SIMD_LV_ 50
#   elif defined(__ARM_VFPV4__)
#     define COMMON_SIMD_LV_ 40
#   elif defined(__ARM_VFPV3__)
#     define COMMON_SIMD_LV_ 30
#   elif defined(__ARM_VFPV2__)
#     define COMMON_SIMD_LV_ 20
#   else
#     define COMMON_SIMD_LV_ 10
#   endif
# else
#   define COMMON_SIMD_LV_ 0
# endif
# ifndef COMMON_SIMD_LV
#   define COMMON_SIMD_LV COMMON_SIMD_LV_
# endif

# if COMMON_SIMD_LV >= 200
#   define COMMON_SIMD_INTRIN A64
# elif COMMON_SIMD_LV >= 100
#   define COMMON_SIMD_INTRIN A32
# elif COMMON_SIMD_LV >= 50
#   define COMMON_SIMD_INTRIN NEON_VFPv5
# elif COMMON_SIMD_LV >= 40
#   define COMMON_SIMD_INTRIN NEON_VFPv4
# elif COMMON_SIMD_LV >= 30
#   define COMMON_SIMD_INTRIN NEON_VFPv3
# elif COMMON_SIMD_LV >= 20
#   define COMMON_SIMD_INTRIN NEON_VFPv2
# elif COMMON_SIMD_LV >= 10
#   define COMMON_SIMD_INTRIN NEON
# else
#   define COMMON_SIMD_INTRIN NONE
# endif

#endif


namespace common::simd
{
inline constexpr auto GetSIMDIntrin() noexcept
{
    return STRINGIZE(COMMON_SIMD_INTRIN);
}

struct VecDataInfo
{
    enum class DataTypes : uint8_t { Empty = 0, Custom, Unsigned, Signed, Float, BFloat };
    DataTypes Type;
    uint8_t Bit;
    uint8_t Dim0;
    uint8_t Dim1;

    constexpr operator uint32_t() const noexcept
    {
        return (static_cast<uint32_t>(Type) << 0)  | (static_cast<uint32_t>(Bit)  << 8) 
            |  (static_cast<uint32_t>(Dim0) << 16) | (static_cast<uint32_t>(Dim1) << 24);
    }
    constexpr VecDataInfo Scalar(uint8_t dim = 0) const noexcept
    {
        return { Type, Bit, dim, 0 };
    }

    template<typename T, uint8_t N>
    static constexpr VecDataInfo GetVectorInfo() noexcept
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            const auto bit = gsl::narrow_cast<uint8_t>(sizeof(T));
            return { DataTypes::Float, bit, N, (uint8_t)0 };
        }
        else if constexpr (std::is_integral_v<T>)
        {
            const auto type = std::is_unsigned_v<T> ? DataTypes::Unsigned : DataTypes::Signed;
            const auto bit = gsl::narrow_cast<uint8_t>(sizeof(T));
            return { type, bit, N, (uint8_t)0 };
        }
        else
        {
            static_assert(!::common::AlwaysTrue<T>, "only numeraic type allowed");
        }
    }
};
}

#if defined(COMMON_SIMD_LV_NAMESPACE) && COMMON_SIMD_LV_NAMESPACE
#   define COMMON_SIMD_NAMESPACE PPCAT(common::simd::lv,COMMON_SIMD_LV)
#else
#   define COMMON_SIMD_NAMESPACE common::simd
#endif
