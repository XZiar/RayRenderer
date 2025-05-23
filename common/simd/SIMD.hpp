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
#   if COMMON_CLANG_VER < 110000
#     define CMSIMD_WA_BEXTR2       1 // https://reviews.llvm.org/D75894
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
#     define CMSIMD_WA_LSU512       1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90980
#   endif
#   if COMMON_GCC_VER < 100100
#     define CMSIMD_WA_LOADU2       1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91341
#   endif
#   if COMMON_GCC_VER < 110000
#     define CMSIMD_WA_LS           1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95483
#   endif
#   if COMMON_GCC_VER < 110300 || (COMMON_GCC_VER >= 120000 && COMMON_GCC_VER < 120100)
#     define CMSIMD_FIX_LOADUSI     1 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99754
#   endif
#     define CMSIMD_WA_BEXTR2       1
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
# ifdef CMSIMD_WA_BEXTR2
#   define _bextr2_u64 __bextr_u64
#   define _bextr2_u32 __bextr_u32
# endif
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
# ifdef CMSIMD_WA_LSU512
#   undef CMSIMD_WA_LSU512
#   define _mm512_loadu_epi32(mem_addr)  _mm512_loadu_si512((const __m512i*)(mem_addr))
#   define _mm512_loadu_epi64(mem_addr)  _mm512_loadu_si512((const __m512i*)(mem_addr))
#   define _mm512_storeu_epi32(mem_addr, a)  _mm512_storeu_si512((__m512i*)(mem_addr), a)
#   define _mm512_storeu_epi64(mem_addr, a)  _mm512_storeu_si512((__m512i*)(mem_addr), a)
#   define _mm256_storeu_epi32(mem_addr, a)  _mm256_storeu_si256((__m256i*)(mem_addr), a)
#   define _mm256_storeu_epi64(mem_addr, a)  _mm256_storeu_si256((__m256i*)(mem_addr), a)
#   define _mm_storeu_epi32(mem_addr, a)     _mm_storeu_si128((__m128i*)(mem_addr), a)
#   define _mm_storeu_epi64(mem_addr, a)     _mm_storeu_si128((__m128i*)(mem_addr), a)
# endif
# ifdef CMSIMD_WA_LOADUSI64
#   undef CMSIMD_WA_LOADUSI64
#   define _mm_loadu_si64(mem_addr) _mm_loadl_epi64((const __m128i*)(mem_addr))
#   define _mm_storeu_si64(mem_addr, a) _mm_storel_epi64((__m128i*)(mem_addr), (a))
# endif
# ifdef CMSIMD_WA_LS
#   undef CMSIMD_WA_LS
#   define _mm_loadu_si32(mem_addr) _mm_cvtsi32_si128(*(const int*)(mem_addr))
#   define _mm_loadu_si16(mem_addr) _mm_cvtsi32_si128(*(const short*)(mem_addr))
#   define _mm_storeu_si32(mem_addr, a) (void)(*(int*)(mem_addr) = _mm_cvtsi128_si32((a)))
#   define _mm_storeu_si16(mem_addr, a) (void)(*(short*)(mem_addr) = (short)_mm_cvtsi128_si32((a)))
#   define _mm512_loadu_epi8(mem_addr)  _mm512_loadu_si512((const __m512i*)(mem_addr))
#   define _mm512_loadu_epi16(mem_addr) _mm512_loadu_si512((const __m512i*)(mem_addr))
#   define _mm256_loadu_epi8(mem_addr)  _mm256_loadu_si256((const __m256i*)(mem_addr))
#   define _mm256_loadu_epi16(mem_addr) _mm256_loadu_si256((const __m256i*)(mem_addr))
#   define _mm256_loadu_epi32(mem_addr) _mm256_loadu_si256((const __m256i*)(mem_addr))
#   define _mm256_loadu_epi64(mem_addr) _mm256_loadu_si256((const __m256i*)(mem_addr))
#   define _mm_loadu_epi8(mem_addr)     _mm_loadu_si128((const __m128i*)(mem_addr))
#   define _mm_loadu_epi16(mem_addr)    _mm_loadu_si128((const __m128i*)(mem_addr))
#   define _mm_loadu_epi32(mem_addr)    _mm_loadu_si128((const __m128i*)(mem_addr))
#   define _mm_loadu_epi64(mem_addr)    _mm_loadu_si128((const __m128i*)(mem_addr))
#   define _mm512_storeu_epi8(mem_addr, a)  _mm512_storeu_epi32(mem_addr, a)
#   define _mm512_storeu_epi16(mem_addr, a) _mm512_storeu_epi32(mem_addr, a)
#   define _mm256_storeu_epi8(mem_addr, a)  _mm256_storeu_epi32(mem_addr, a)
#   define _mm256_storeu_epi16(mem_addr, a) _mm256_storeu_epi32(mem_addr, a)
#   define _mm_storeu_epi8(mem_addr, a)     _mm_storeu_epi32(mem_addr, a)
#   define _mm_storeu_epi16(mem_addr, a)    _mm_storeu_epi32(mem_addr, a)
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
// enforce mullo, workrounds for GCC
# if COMMON_COMPILER_GCC
#   define _mm_mullo_epi64_force(a, b)    _mm_maskz_mullo_epi64(0xff, a, b)
#   define _mm_mullo_epi32_force(a, b)    (__m128i)__builtin_ia32_pmulld128((__v4si)(a), (__v4si)(b))
#   define _mm_mullo_epi16_force(a, b)    (__m128i)__builtin_ia32_pmullw128((__v8hi)(a), (__v8hi)(b))
#   define _mm256_mullo_epi64_force(a, b) _mm256_maskz_mullo_epi64(0xff, a, b)
#   define _mm256_mullo_epi32_force(a, b) (__m256i)__builtin_ia32_pmulld256((__v8si)(a), (__v8si)(b))
#   define _mm256_mullo_epi16_force(a, b) (__m256i)__builtin_ia32_pmullw256((__v16hi)(a), (__v16hi)(b))
#   define _mm512_mullo_epi64_force(a, b) _mm512_maskz_mullo_epi64(0xff, a, b)
#   define _mm512_mullo_epi32_force(a, b) _mm512_maskz_mullo_epi32(0xffff, a, b)
#   define _mm512_mullo_epi16_force(a, b) _mm512_maskz_mullo_epi16(0xffffffffu, a, b)
# else
#   define _mm_mullo_epi64_force _mm_mullo_epi64
#   define _mm_mullo_epi32_force _mm_mullo_epi32
#   define _mm_mullo_epi16_force _mm_mullo_epi16
#   define _mm256_mullo_epi64_force _mm256_mullo_epi64
#   define _mm256_mullo_epi32_force _mm256_mullo_epi32
#   define _mm256_mullo_epi16_force _mm256_mullo_epi16
#   define _mm512_mullo_epi64_force _mm512_mullo_epi64
#   define _mm512_mullo_epi32_force _mm512_mullo_epi32
#   define _mm512_mullo_epi16_force _mm512_mullo_epi16
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
// workrounds
# if COMMON_COMPILER_MSVC // msvc only typedef __n128 & __n64
#    define vld1q_f16 vld1q_u16
#    define vld2q_f16 vld2q_u16
#    define vld3q_f16 vld3q_u16
#    define vld4q_f16 vld4q_u16
#    define vld1q_f16_x2 vld1q_u16_x2
#    define vld1q_f16_x3 vld1q_u16_x3
#    define vld1q_f16_x4 vld1q_u16_x4
#    define vst1q_f16 vst1q_u16
#    define vst2q_f16 vst2q_u16
#    define vst3q_f16 vst3q_u16
#    define vst4q_f16 vst4q_u16
#    define vst1q_f16_x2 vst1q_u16_x2
#    define vst1q_f16_x3 vst1q_u16_x3
#    define vst1q_f16_x4 vst1q_u16_x4
#    define vdupq_n_f16 vdupq_n_u16
using float16_t = uint16_t;
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

# if defined(__AVX512FP16__)
#   define COMMON_SIMD_FP16 1
# endif
# if defined(__AVX512BF16__)
#   define COMMON_SIMD_BF16 1
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

# if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC) && __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#   define COMMON_SIMD_FP16 1
# endif
# if defined(__ARM_FEATURE_BF16_VECTOR_ARITHMETIC) && __ARM_FEATURE_BF16_VECTOR_ARITHMETIC
#   define COMMON_SIMD_BF16 1
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
forceinline constexpr auto GetSIMDIntrin() noexcept
{
    return STRINGIZE(COMMON_SIMD_INTRIN);
}


// undefined value when val == 0
template<typename T>
forceinline uint32_t CountTralingZero(const T& val) noexcept
{
    static_assert(std::is_integral_v<T>, "only allow integer");
    static_assert(sizeof(T) <= 8, "no more than 64bit");
#if COMMON_COMPILER_MSVC
    unsigned long index = 0;
    if constexpr (sizeof(T) <= 4)
        _BitScanForward(&index, static_cast<unsigned long>(val));
    else
        _BitScanForward64(&index, static_cast<__int64>(val));
    return static_cast<uint32_t>(index);
#else
    if constexpr (sizeof(T) <= 4)
        return __builtin_ctz(static_cast<unsigned int>(val));
    else
        return __builtin_ctzl(static_cast<unsigned long>(val)); // LP64's long is 64bit
#endif
}


// enforce MOVBE support
template<typename T, bool IsLE, typename U>
[[nodiscard]] forceinline constexpr T EndianReader(const U* __restrict const src) noexcept
{
    static_assert(sizeof(U) == 1);
#if COMMON_ARCH_X86 && !COMMON_COMPILER_GCC
    if (!is_constant_evaluated(true))
    {
        if constexpr (detail::is_little_endian == IsLE)
            return *reinterpret_cast<const T*>(src);
        else if constexpr (sizeof(T) == 2)
            return static_cast<T>(_loadbe_i16(src));
        else if constexpr (sizeof(T) == 4)
            return static_cast<T>(_loadbe_i32(src));
        else if constexpr (sizeof(T) == 8)
            return static_cast<T>(_loadbe_i64(src));
        else
            static_assert(!AlwaysTrue<T>, "only 2/4/8 Byte");
    }
#endif
    return ::common::EndianReader<T, IsLE>(src);
}

template<bool IsLE, typename T, typename U>
forceinline constexpr void EndianWriter(U* __restrict const dst, const T val) noexcept
{
    static_assert(sizeof(U) == 1);
#if COMMON_ARCH_X86 && !COMMON_COMPILER_GCC
    if (!is_constant_evaluated(true))
    {
        if constexpr (detail::is_little_endian == IsLE)
            *std::launder(reinterpret_cast<T*>(dst)) = val;
        else if constexpr (sizeof(T) == 2)
            _storebe_i16(dst, static_cast<int16_t>(val));
        else if constexpr (sizeof(T) == 4)
            _storebe_i32(dst, static_cast<int32_t>(val));
        else if constexpr (sizeof(T) == 8)
            _storebe_i64(dst, static_cast<int64_t>(val));
        else
            static_assert(!AlwaysTrue<T>, "only 2/4/8 Byte");
        return;
    }
#endif
    return ::common::EndianWriter<IsLE>(dst, val);
}


#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#endif
// undfined value when count >= 64
forceinline uint64_t Shift128Left(const uint64_t hi, const uint64_t lo, uint8_t count) noexcept
{
    CM_ASSUME(count < 64);
#if COMMON_COMPILER_MSVC && COMMON_ARCH_X86 && COMMON_OSBIT == 64
    return __shiftleft128(lo, hi, count);
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
    const auto val = (static_cast<unsigned __int128>(hi) << 64) | static_cast<unsigned __int128>(lo);
    return static_cast<uint64_t>((val << count) >> 64);
#else
    return (hi << count) | (lo >> (64 - count));
#endif
}
forceinline uint64_t Shift128Right(const uint64_t hi, const uint64_t lo, uint8_t count) noexcept
{
    CM_ASSUME(count < 64);
#if COMMON_COMPILER_MSVC && COMMON_ARCH_X86 && COMMON_OSBIT == 64
    return __shiftright128(lo, hi, count);
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
    const auto val = (static_cast<unsigned __int128>(hi) << 64) | static_cast<unsigned __int128>(lo);
    return static_cast<uint64_t>(val >> count);
#else
    return (hi << (64 - count)) | (lo >> count);
#endif
}
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#endif


#if COMMON_ARCH_X86
template<uint8_t Bits, typename T>
forceinline T ExtractBits(const T src, uint32_t start) noexcept
{
    const uint32_t control = start + (Bits << 8);
    if constexpr (sizeof(T) > sizeof(uint32_t))
        return static_cast<T>(_bextr2_u64(src, control));
    else
        return static_cast<T>(_bextr2_u32(src, control));
}
#endif


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
