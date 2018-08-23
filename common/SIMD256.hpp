#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"

//#define COMMON_SIMD_LV 200
#if COMMON_SIMD_LV < 100
#   error require at least AVX
#endif

namespace common
{
namespace simd
{

struct I8x32;
struct U8x32;
struct I16x16;
struct U16x16;
struct I32x8;
struct U32x8;
struct F32x8;
struct I64x4;
struct F64x4;


namespace detail
{

#if COMMON_SIMD_LV >= 200
template<typename T, typename E>
struct Int256Common : public CommonOperators<T>
{
    // logic operations
    T And(const T& other) const
    {
        return _mm256_and_si256(*static_cast<const T*>(this), other);
    }
    T Or(const T& other) const
    {
        return _mm256_or_si256(*static_cast<const T*>(this), other);
    }
    T Xor(const T& other) const
    {
        return _mm256_xor_si256(*static_cast<const T*>(this), other);
    }
    T AndNot(const T& other) const
    {
        return _mm256_andnot_si256(*static_cast<const T*>(this), other);
    }
    T Not() const
    {
        return _mm256_xor_si256(*static_cast<const T*>(this), _mm256_set1_epi8(-1));
    }
    void Load(const E *ptr) { *static_cast<E*>(this) = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)); }
    void Save(E *ptr) const { _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), *static_cast<const T*>(this)); }
    constexpr operator const __m256i&() const noexcept { return static_cast<const T*>(this)->Data; }
};
#endif

}


struct alignas(__m256d) F64x4 : public detail::CommonOperators<F64x4>
{
    static constexpr size_t Count = 4;
    union
    {
        __m256d Data;
        double Val[4];
    };
    constexpr F64x4() : Data() { }
    explicit F64x4(const double* ptr) : Data(_mm256_loadu_pd(ptr)) { }
    constexpr F64x4(const __m256d val) : Data(val) { }
    F64x4(const double val) : Data(_mm256_set1_pd(val)) { }
    F64x4(const double lo0, const double lo1, const double lo2, const double hi3) :Data(_mm256_setr_pd(lo0, lo1, lo2, hi3)) { }
    constexpr operator const __m256d&() { return Data; }
    void Load(const double *ptr) { Data = _mm256_loadu_pd(ptr); }
    void Save(double *ptr) const { _mm256_storeu_pd(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    F64x4 Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        if constexpr (Lo0 < 2 && Lo1 < 2 && Lo2 >= 2 && Hi3 >= 2) // no cross lane
        {
            return _mm256_permute_pd(Data, (Hi3 << 3) + (Lo2 << 2) + (Lo1 << 1) + Lo0 - 0b11000);
        }
        else
        {
#if COMMON_SIMD_LV >= 200
            return _mm256_permute4x64_pd(Data, (Hi3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
#else
            constexpr uint8_t lane0 = Lo0 & 0b10, lane1 = Lo1 & 0b10, lane2 = Lo2 & 0b10, lane3 = Hi3 & 0b10;
            constexpr uint8_t off0 = Lo0 & 0b1, off1 = Lo1 & 0b1, off2 = Lo2 & 0b1, off3 = Hi3 & 0b1;
            const auto tmpa = _mm256_permute2f128_pd(Data, Data, (lane0 ? 0b1 : 0b0) + (lane2 ? 0b100 : 0b000));
            const auto tmpb = _mm256_permute2f128_pd(Data, Data, (lane1 ? 0b1 : 0b0) + (lane3 ? 0b100 : 0b000));
            return _mm256_shuffle_pd(tmpa, tmpb, (off3 << 3) + (off2 << 2) + (off1 << 1) + off0);
#endif
        }
    }

    // logic operations
    F64x4 And(const F64x4& other) const { return _mm256_and_pd(Data, other.Data); }
    F64x4 Or(const F64x4& other) const { return _mm256_or_pd(Data, other.Data); }
    F64x4 Xor(const F64x4& other) const { return _mm256_xor_pd(Data, other.Data); }
    F64x4 AndNot(const F64x4& other) const { return _mm256_andnot_pd(Data, other.Data); }
    F64x4 Not() const
    {
        static const int64_t i = -1;
        return _mm256_xor_pd(Data, _mm256_set1_pd(*reinterpret_cast<const double*>(&i)));
    }

    // arithmetic operations
    F64x4 Add(const F64x4& other) const { return _mm256_add_pd(Data, other.Data); }
    F64x4 Sub(const F64x4& other) const { return _mm256_sub_pd(Data, other.Data); }
    F64x4 Mul(const F64x4& other) const { return _mm256_mul_pd(Data, other.Data); }
    F64x4 Div(const F64x4& other) const { return _mm256_div_pd(Data, other.Data); }
    F64x4 Max(const F64x4& other) const { return _mm256_max_pd(Data, other.Data); }
    F64x4 Min(const F64x4& other) const { return _mm256_min_pd(Data, other.Data); }
    //F64x4 Rcp() const { return _mm256_rcp_pd(Data); }
    F64x4 Sqrt() const { return _mm256_sqrt_pd(Data); }
    //F64x4 Rsqrt() const { return _mm256_rsqrt_pd(Data); }

    F64x4 MulAdd(const F64x4& muler, const F64x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm256_add_pd(_mm256_mul_pd(Data, muler.Data), adder.Data);
#endif
    }
    F64x4 MulSub(const F64x4& muler, const F64x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm256_sub_pd(_mm256_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    F64x4 NMulAdd(const F64x4& muler, const F64x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm256_sub_pd(adder.Data, _mm256_mul_pd(Data, muler.Data));
#endif
    }
    F64x4 NMulSub(const F64x4& muler, const F64x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm256_xor_pd(_mm256_add_pd(_mm256_mul_pd(Data, muler.Data), suber.Data), _mm256_set1_pd(-0.));
#endif
    }
    F64x4 operator*(const F64x4& other) const { return Mul(other); }
    F64x4 operator/(const F64x4& other) const { return Div(other); }

};


struct alignas(__m256) F32x8 : public detail::CommonOperators<F32x8>
{
    static constexpr size_t Count = 8;
    union
    {
        __m256 Data;
        float Val[8];
    };
    constexpr F32x8() : Data() { }
    explicit F32x8(const float* ptr) : Data(_mm256_loadu_ps(ptr)) { }
    constexpr F32x8(const __m256 val) :Data(val) { }
    F32x8(const float val) :Data(_mm256_set1_ps(val)) { }
    F32x8(const float lo0, const float lo1, const float lo2, const float lo3, const float lo4, const float lo5, const float lo6, const float hi7)
        :Data(_mm256_setr_ps(lo0, lo1, lo2, lo3, lo4, lo5, lo6, hi7)) { }

    constexpr operator const __m256&() const noexcept { return Data; }
    void Load(const float *ptr) { Data = _mm256_loadu_ps(ptr); }
    void Save(float *ptr) const { _mm256_storeu_ps(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    F32x8 Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        if constexpr (Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Lo3 < 4 &&
            Lo4 - Lo0 == 4 && Lo5 - Lo1 == 4 && Lo6 - Lo2 == 4 && Hi7 - Lo3 == 4) // no cross lane
        {
            return _mm256_permute_ps(Data, (Lo3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
        }
        else
        {
#if COMMON_SIMD_LV >= 200
            static const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
            return _mm256_permutevar8x32_ps(Data, mask);
#else
            constexpr uint8_t lane0 = Lo1 & 0b100, lane1 = Lo2 & 0b100, lane2 = Lo3 & 0b100, lane3 = Lo4 & 0b100,
                lane4 = Lo5 & 0b100, lane5 = Lo6 & 0b100, lane6 = Lo7 & 0b100, lane7 = Hi7 & 0b100;
            constexpr uint8_t off0 = Lo1 & 0b11, off1 = Lo2 & 0b11, off2 = Lo3 & 0b11, off3 = Lo4 & 0b11,
                off4 = Lo5 & 0b11, off5 = Lo6 & 0b11, off6 = Lo7 & 0b11, off7 = Hi7 & 0b11;
            if constexpr (lane0 == lane1 && lane2 == lane3 && lane4 == lane5 && lane6 == lane7 &&
                off0 == off4 && off1 == off5 && off2 == off6 && off3 == off7)
            {
                const auto tmpa = _mm256_permute2f128_ps(Data, Data, (lane0 ? 0b1 : 0b0) + (lane4 ? 0b100 : 0b000));
                const auto tmpb = _mm256_permute2f128_ps(Data, Data, (lane2 ? 0b1 : 0b0) + (lane6 ? 0b100 : 0b000));
                return _mm256_shuffle_ps(tmpa, tmpb, (off3 << 6) + (off2 << 4) + (off1 << 2) + off0);
            }
            else
            {
                const auto tmpa = _mm256_permute2f128_ps(Data, Data, (lane0 ? 0b1 : 0b0) + (lane5 ? 0b100 : 0b000));
                const auto tmpb = _mm256_permute2f128_ps(Data, Data, (lane2 ? 0b1 : 0b0) + (lane7 ? 0b100 : 0b000));
                const auto f0257 = _mm256_shuffle_ps(tmpa, tmpb, (off7 << 6) + (off2 << 4) + (off5 << 2) + off0);
                const auto tmpc = _mm256_permute2f128_ps(Data, Data, (lane1 ? 0b1 : 0b0) + (lane4 ? 0b100 : 0b000));
                const auto tmpd = _mm256_permute2f128_ps(Data, Data, (lane3 ? 0b1 : 0b0) + (lane6 ? 0b100 : 0b000));
                const auto f1346 = _mm256_shuffle_ps(tmpc, tmpd, (off6 << 6) + (off3 << 4) + (off4 << 2) + off1);
                return _mm256_blend_ps(f0257, f1346, 0b01011010);
            }
#endif
        }
    }

    // logic operations
    F32x8 And(const F32x8& other) const
    {
        return _mm256_and_ps(Data, other.Data);
    }
    F32x8 Or(const F32x8& other) const
    {
        return _mm256_or_ps(Data, other.Data);
    }
    F32x8 Xor(const F32x8& other) const
    {
        return _mm256_xor_ps(Data, other.Data);
    }
    F32x8 AndNot(const F32x8& other) const
    {
        return _mm256_andnot_ps(Data, other.Data);
    }
    F32x8 Not() const
    {
        static const int32_t i = -1;
        return _mm256_xor_ps(Data, _mm256_set1_ps(*reinterpret_cast<const float*>(&i)));
    }

    // arithmetic operations
    F32x8 Add(const F32x8& other) const { return _mm256_add_ps(Data, other.Data); }
    F32x8 Sub(const F32x8& other) const { return _mm256_sub_ps(Data, other.Data); }
    F32x8 Mul(const F32x8& other) const { return _mm256_mul_ps(Data, other.Data); }
    F32x8 Div(const F32x8& other) const { return _mm256_div_ps(Data, other.Data); }
    F32x8 Max(const F32x8& other) const { return _mm256_max_ps(Data, other.Data); }
    F32x8 Min(const F32x8& other) const { return _mm256_min_ps(Data, other.Data); }
    F32x8 Rcp() const { return _mm256_rcp_ps(Data); }
    F32x8 Sqrt() const { return _mm256_sqrt_ps(Data); }
    F32x8 Rsqrt() const { return _mm256_rsqrt_ps(Data); }
    F32x8 MulAdd(const F32x8& muler, const F32x8& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm256_add_ps(_mm256_mul_ps(Data, muler.Data), adder.Data);
#endif
    }
    F32x8 MulSub(const F32x8& muler, const F32x8& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm256_sub_ps(_mm256_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    F32x8 NMulAdd(const F32x8& muler, const F32x8& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm256_sub_ps(adder.Data, _mm256_mul_ps(Data, muler.Data));
#endif
    }
    F32x8 NMulSub(const F32x8& muler, const F32x8& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm256_xor_ps(_mm256_add_ps(_mm256_mul_ps(Data, muler.Data), suber.Data), _mm256_set1_ps(-0.));
#endif
    }
#if COMMON_SIMD_LV >= 42
    template<VecPos Mul, VecPos Res>
    F32x8 Dot(const F32x8& other) const 
    { 
        return _mm256_dp_ps(Data, other.Data, static_cast<uint8_t>(Mul) << 4 | static_cast<uint8_t>(Res));
    }
#endif
    F32x8 operator*(const F32x8& other) const { return Mul(other); }
    F32x8 operator/(const F32x8& other) const { return Div(other); }

};


#if COMMON_SIMD_LV >= 300
struct alignas(__m256i) I64x4 : public detail::Int256Common<I64x4, int64_t>
{
    static constexpr size_t Count = 4;
    union
    {
        __m256i Data;
        int64_t Ints[4];
    };
    constexpr I64x4() : Data() { }
    explicit I64x4(const int64_t* ptr) : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    constexpr I64x4(const __m256i val) :Data(val) { }
    I64x4(const int64_t val) :Data(_mm256_set1_epi64x(val)) { }
    I64x4(const int64_t lo, const int64_t hi) :Data(_mm256_set_epi64x(hi, lo)) { }

    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    I64x4 Shuffle() const
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        return _mm256_shuffle_epi32(Data, _mm256_SHUFFLE(Hi*2+1, Hi*2, Lo*2+1, Lo*2));
    }

    // arithmetic operations
    I64x4 Add(const I64x4& other) const { return _mm256_add_epi64(Data, other.Data); }
    I64x4 Sub(const I64x4& other) const { return _mm256_sub_epi64(Data, other.Data); }
    I64x4 Max(const I64x4& other) const { return _mm256_max_epi64(Data, other.Data); }
    I64x4 Min(const I64x4& other) const { return _mm256_min_epi64(Data, other.Data); }
    I64x4 ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi64(Data, I64x4(bits)); }
    I64x4 ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi64(Data, I64x4(bits)); }
    template<uint8_t N>
    I64x4 ShiftRightArth() const { return _mm256_srai_epi64(Data, N); }
    template<uint8_t N>
    I64x4 ShiftRightLogic() const { return _mm256_srli_epi64(Data, N); }
    template<uint8_t N>
    I64x4 ShiftLeftLogic() const { return _mm256_slli_epi64(Data, N); }
};


template<typename T, typename E>
struct alignas(__m256i) I32Common8
{
    static constexpr size_t Count = 8;
    union
    {
        __m256i Data;
        E Val[8];
    };
    constexpr I32Common8() : Data() { }
    explicit I32Common8(const E* ptr) : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    constexpr I32Common8(const __m256i val) : Data(val) { }
    I32Common8(const E val) : Data(_mm256_set1_epi32(static_cast<int32_t>(val))) { }
    I32Common8(const E lo0, const E lo1, const E lo2, const E hi3)
        : Data(_mm256_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(hi3))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    T Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm256_shuffle_epi32(Data, _mm256_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
    }

    // arithmetic operations
    T Add(const T& other) const { return _mm256_add_epi32(Data, other.Data); }
    T Sub(const T& other) const { return _mm256_sub_epi32(Data, other.Data); }
    T ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi32(Data, I64x4(bits)); }
    T ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi32(Data, I64x4(bits)); }
    template<uint8_t N>
    T ShiftRightLogic() const { return _mm256_srli_epi32(Data, N); }
    template<uint8_t N>
    T ShiftLeftLogic() const { return _mm256_slli_epi32(Data, N); }
#if COMMON_SIMD_LV >= 41
    T MulLo(const T& other) const { return _mm256_mullo_epi32(Data, other.Data); }
    T operator*(const T& other) const { return MulLo(other); }
#endif
};


struct alignas(__m256i) I32x8 : public I32Common8<I32x8, int32_t>, public detail::Int256Common<I32x8, int32_t>
{
    using I32Common8<I32x8, int32_t>::I32Common8;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    I32x8 Max(const I32x8& other) const { return _mm256_max_epi32(Data, other.Data); }
    I32x8 Min(const I32x8& other) const { return _mm256_min_epi32(Data, other.Data); }
    Pack<I64x4, 2> MulX(const I32x8& other) const
    {
        return { I64x4(_mm256_mul_epi32(Data, other.Data)), I64x4(_mm256_mul_epi32(Shuffle<2, 3, 0, 1>(), other.Shuffle<2, 3, 0, 1>())) };
    }
#endif
    I32x8 operator>>(const uint8_t bits) const { return _mm256_sra_epi32(Data, I64x4(bits)); }
    template<uint8_t N>
    I32x8 ShiftRightArth() const { return _mm256_srai_epi32(Data, N); }
};

struct alignas(__m256i) U32x8 : public I32Common8<U32x8, uint32_t>, public detail::Int256Common<U32x8, uint32_t>
{
    using I32Common8<U32x8, uint32_t>::I32Common8;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    U32x8 Max(const U32x8& other) const { return _mm256_max_epu32(Data, other.Data); }
    U32x8 Min(const U32x8& other) const { return _mm256_min_epu32(Data, other.Data); }
#endif
    U32x8 operator>>(const uint8_t bits) const { return _mm256_srl_epi32(Data, I64x4(bits)); }
    template<uint8_t N>
    U32x8 ShiftRightArth() const { return _mm256_srli_epi32(Data, N); }
    Pack<I64x4, 2> MulX(const U32x8& other) const
    {
        return { I64x4(_mm256_mul_epu32(Data, other.Data)), I64x4(_mm256_mul_epu32(Shuffle<2, 3, 0, 1>(), other.Shuffle<2, 3, 0, 1>())) };
    }
};


template<typename T, typename E>
struct alignas(__m256i) I16Common16
{
    static constexpr size_t Count = 16;
    union
    {
        __m256i Data;
        E Val[16];
    };
    constexpr I16Common16() : Data() { }
    explicit I16Common16(const E* ptr) : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    constexpr I16Common16(const __m256i val) :Data(val) { }
    I16Common16(const E val) :Data(_mm256_set1_epi16(val)) { }
    I16Common16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7)
        :Data(_mm256_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(hi7))) { }

    // shuffle operations
#if COMMON_SIMD_LV >= 31
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    T Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        static const auto mask = _mm256_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm256_shuffle_epi8(Data, mask);
    }
#endif
    
    // arithmetic operations
    T Add(const T& other) const { return _mm256_add_epi16(Data, other.Data); }
    T Sub(const T& other) const { return _mm256_sub_epi16(Data, other.Data); }
    T MulLo(const T& other) const { return _mm256_mullo_epi16(Data, other.Data); }
    T operator*(const T& other) const { return MulLo(other); }
    T ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi16(Data, I64x4(bits)); }
    T ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi16(Data, I64x4(bits)); }
    template<uint8_t N>
    T ShiftLeftLogic() const { return _mm256_slli_epi16(Data, N); }
    template<uint8_t N>
    T ShiftRightLogic() const { return _mm256_srli_epi16(Data, N); }
    Pack<I32x8, 2> MulX(const T& other) const
    {
        const auto los = MulLo(other), his = static_cast<const T*>(this)->MulHi(other);
        return { _mm256_unpacklo_epi16(los, his),_mm256_unpackhi_epi16(los, his) };
    }
};


struct alignas(__m256i) I16x16 : public I16Common16<I16x16, int16_t>, public detail::Int256Common<I16x16, int16_t>
{
    using I16Common16<I16x16, int16_t>::I16Common16;

    // arithmetic operations
    I16x16 SatAdd(const I16x16& other) const { return _mm256_adds_epi16(Data, other.Data); }
    I16x16 SatSub(const I16x16& other) const { return _mm256_subs_epi16(Data, other.Data); }
    I16x16 MulHi(const I16x16& other) const { return _mm256_mulhi_epi16(Data, other.Data); }
    I16x16 Max(const I16x16& other) const { return _mm256_max_epi16(Data, other.Data); }
    I16x16 Min(const I16x16& other) const { return _mm256_min_epi16(Data, other.Data); }
    I16x16 operator>>(const uint8_t bits) const { return _mm256_sra_epi16(Data, I64x4(bits)); }
    template<uint8_t N>
    I16x16 ShiftRightArth() const { return _mm256_srai_epi16(Data, N); }
};


struct alignas(__m256i) U16x16 : public I16Common16<U16x16, int16_t>, public detail::Int256Common<U16x16, uint16_t>
{
    using I16Common16<U16x16, int16_t>::I16Common16;

    // arithmetic operations
    U16x16 SatAdd(const U16x16& other) const { return _mm256_adds_epu16(Data, other.Data); }
    U16x16 SatSub(const U16x16& other) const { return _mm256_subs_epu16(Data, other.Data); }
    U16x16 MulHi(const U16x16& other) const { return _mm256_mulhi_epu16(Data, other.Data); }
    U16x16 Div(const uint16_t other) const { return MulHi(U16x16(static_cast<uint16_t>(UINT16_MAX / other))); }
    U16x16 operator/(const uint16_t other) const { return Div(other); }
#if COMMON_SIMD_LV >= 41
    U16x16 Max(const U16x16& other) const { return _mm256_max_epu16(Data, other.Data); }
    U16x16 Min(const U16x16& other) const { return _mm256_min_epu16(Data, other.Data); }
#endif
    U16x16 operator>>(const uint8_t bits) const { return _mm256_srl_epi16(Data, I64x4(bits)); }
    template<uint8_t N>
    U16x16 ShiftRightArth() const { return _mm256_srli_epi16(Data, N); }
};


template<typename T, typename E>
struct alignas(__m256i) I8Common32
{
    static constexpr size_t Count = 32;
    union
    {
        __m256i Data;
        E Val[32];
    };
    constexpr I8Common32() : Data() { }
    explicit I8Common32(const E* ptr) : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    constexpr I8Common32(const __m256i val) :Data(val) { }
    I8Common32(const E val) :Data(_mm256_set1_epi8(val)) { }
    I8Common32(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15)
        :Data(_mm256_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4), static_cast<int8_t>(lo5), static_cast<int8_t>(lo6), static_cast<int8_t>(lo7), static_cast<int8_t>(lo8),
            static_cast<int8_t>(lo9), static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), static_cast<int8_t>(lo12), static_cast<int8_t>(lo13),
            static_cast<int8_t>(lo14), static_cast<int8_t>(hi15))) { }

    // shuffle operations
#if COMMON_SIMD_LV >= 31
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    T Shuffle() const
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        static const auto mask = _mm256_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm256_shuffle_epi8(Data, mask);
    }
#endif

    // arithmetic operations
    T Add(const T& other) const { return _mm256_add_epi8(Data, other.Data); }
    T Sub(const T& other) const { return _mm256_sub_epi8(Data, other.Data); }
};


struct alignas(__m256i) I8x32 : public I8Common32<I8x32, int8_t>, public detail::Int256Common<I8x32, int8_t>
{
    using I8Common32<I8x32, int8_t>::I8Common32;

    // arithmetic operations
    I8x32 SatAdd(const I8x32& other) const { return _mm256_adds_epi8(Data, other.Data); }
    I8x32 SatSub(const I8x32& other) const { return _mm256_subs_epi8(Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    I8x32 Max(const I8x32& other) const { return _mm256_max_epi8(Data, other.Data); }
    I8x32 Min(const I8x32& other) const { return _mm256_min_epi8(Data, other.Data); }
#endif
    template<typename T>
    typename CastTyper<I8x32, T>::Type Cast() const;
#if COMMON_SIMD_LV >= 41
    Pack<I16x16, 2> MulX(const I8x32& other) const;
    I8x32 MulHi(const I8x32& other) const
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightArth<8>(), hi = full[1].ShiftRightArth<8>();
        return _mm256_packs_epi16(lo, hi);
    }
    I8x32 MulLo(const I8x32& other) const 
    { 
        const auto full = MulX(other);
        const auto mask = I16x16(INT16_MIN);
        const auto signlo = full[0].And(mask).ShiftRightArth<8>(), signhi = full[1].And(mask).ShiftRightArth<8>();
        const auto lo = full[0] & signlo, hi = full[1] & signhi;
        return _mm256_packs_epi16(lo, hi);
    }
    I8x32 operator*(const I8x32& other) const { return MulLo(other); }
#endif
};
#if COMMON_SIMD_LV >= 41
template<> Pack<I16x16, 2> I8x32::Cast<I16x16>() const
{
    return { _mm256_cvtepi8_epi16(Data), _mm256_cvtepi8_epi16(_mm256_shuffle_epi32(Data, _mm256_SHUFFLE(3,2,3,2))) };
}
Pack<I16x16, 2> I8x32::MulX(const I8x32& other) const
{
    const auto self16 = Cast<I16x16>(), other16 = other.Cast<I16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif

struct alignas(__m256i) U8x32 : public I8Common32<U8x32, uint8_t>, public detail::Int256Common<U8x32, uint8_t>
{
    using I8Common32<U8x32, uint8_t>::I8Common32;

    // arithmetic operations
    U8x32 SatAdd(const U8x32& other) const { return _mm256_adds_epu8(Data, other.Data); }
    U8x32 SatSub(const U8x32& other) const { return _mm256_subs_epu8(Data, other.Data); }
    U8x32 Max(const U8x32& other) const { return _mm256_max_epu8(Data, other.Data); }
    U8x32 Min(const U8x32& other) const { return _mm256_min_epu8(Data, other.Data); }
    U8x32 operator*(const U8x32& other) const { return MulLo(other); }
    template<typename T>
    typename CastTyper<U8x32, T>::Type Cast() const;
    U8x32 MulHi(const U8x32& other) const
    {
        const auto full = MulX(other);
        return _mm256_packus_epi16(full[0].ShiftRightLogic<8>(), full[1].ShiftRightLogic<8>());
    }
    U8x32 MulLo(const U8x32& other) const
    {
        const U16x16 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        static const U16x16 mask((int16_t)0xff);
        return U8x32(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    Pack<U16x16, 2> MulX(const U8x32& other) const;
};


template<> Pack<I16x16, 2> U8x32::Cast<I16x16>() const
{
    const auto tmp = _mm256_setzero_si256();
    return { _mm256_unpacklo_epi8(*this, tmp), _mm256_unpackhi_epi8(*this, tmp) };
}
template<> Pack<U16x16, 2> U8x32::Cast<U16x16>() const
{
    const auto tmp = _mm256_setzero_si256();
    return { _mm256_unpacklo_epi8(*this, tmp), _mm256_unpackhi_epi8(*this, tmp) };
}
Pack<U16x16, 2> U8x32::MulX(const U8x32& other) const
{
    const auto self16 = Cast<U16x16>(), other16 = other.Cast<U16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif

}
}
