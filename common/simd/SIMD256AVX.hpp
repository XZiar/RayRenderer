#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"
#include "SIMD128SSE.hpp"

//#define COMMON_SIMD_LV 200
#if COMMON_SIMD_LV < 100
#   error require at least AVX
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
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
    forceinline T VECCALL And(const T& other) const
    {
        return _mm256_and_si256(*static_cast<const T*>(this), other);
    }
    forceinline T VECCALL Or(const T& other) const
    {
        return _mm256_or_si256(*static_cast<const T*>(this), other);
    }
    forceinline T VECCALL Xor(const T& other) const
    {
        return _mm256_xor_si256(*static_cast<const T*>(this), other);
    }
    forceinline T VECCALL AndNot(const T& other) const
    {
        return _mm256_andnot_si256(*static_cast<const T*>(this), other);
    }
    forceinline T VECCALL Not() const
    {
        return _mm256_xor_si256(*static_cast<const T*>(this), _mm256_set1_epi8(-1));
    }
    forceinline T VECCALL MoveHiToLo() const { return _mm256_permute4x64_epi64(*static_cast<const T*>(this), 0b01001110); }
    forceinline T VECCALL ShuffleHiLo() const { return _mm256_permute4x64_epi64(*static_cast<const T*>(this), 0b01001110); }
    forceinline void VECCALL Load(const E *ptr) { *static_cast<E*>(this) = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)); }
    forceinline void VECCALL Save(E *ptr) const { _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), *static_cast<const T*>(this)); }
    forceinline constexpr operator const __m256i&() const noexcept { return static_cast<const T*>(this)->Data; }
};
#endif

}


struct alignas(__m256d) F64x4 : public detail::CommonOperators<F64x4>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,64,4,0 };
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
    F64x4(const Pack<F64x2, 2>& pack) : Data(_mm256_set_m128d(pack[1].Data, pack[0].Data)) {}
    F64x4(const double lo0, const double lo1, const double lo2, const double hi3) :Data(_mm256_setr_pd(lo0, lo1, lo2, hi3)) { }
    forceinline constexpr operator const __m256d&() { return Data; }
    forceinline void VECCALL Load(const double *ptr) { Data = _mm256_loadu_pd(ptr); }
    forceinline void VECCALL Save(double *ptr) const { _mm256_storeu_pd(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline F64x4 VECCALL Shuffle() const
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
            const auto tmpa = _mm256_permute2f128_pd(Data, Data, (lane0 ? 0x1 : 0x0) + (lane2 ? 0x10 : 0x00));
            const auto tmpb = _mm256_permute2f128_pd(Data, Data, (lane1 ? 0x1 : 0x0) + (lane3 ? 0x10 : 0x00));
            return _mm256_shuffle_pd(tmpa, tmpb, (off3 << 3) + (off2 << 2) + (off1 << 1) + off0);
#endif
        }
    }
    forceinline F64x4 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const
    {
        //assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        // _mm256_permutevar_pd checks offset[1] rather than offset[0]
        if (Lo0 < 2 && Lo1 < 2 && Lo2 >= 2 && Hi3 >= 2) // no cross lane
        {
            return _mm256_permutevar_pd(Data, _mm256_setr_epi64x(Lo0 << 1, Lo1 << 1, Lo2 << 1, Hi3 << 1));
        }
        else
        {
#if COMMON_SIMD_LV >= 200
            const auto mask = _mm256_setr_epi32(Lo0*2, Lo0*2+1, Lo1*2, Lo1*2+1, Lo2*2, Lo2*2+1, Hi3*2, Hi3*2+1);
            return _mm256_castps_pd(_mm256_permutevar8x32_ps(_mm256_castpd_ps(Data), mask));
#else
            const auto offMask = _mm256_setr_epi64x(Lo0 << 1, Lo1 << 1, Lo2 << 1, Hi3 << 1);
            const auto laneMask = _mm256_setr_epi64x((int64_t)(Lo0) << 62, (int64_t)(Lo1) << 62, (int64_t)(~Lo2) << 62, (int64_t)(~Hi3) << 62);
            const auto hilo = _mm256_permute2f128_pd(Data, Data, 0b0001);//0123->2301
            const auto lohiShuffle = _mm256_permutevar_pd(Data, offMask);
            const auto hiloShuffle = _mm256_permutevar_pd(hilo, offMask);
            return _mm256_blendv_pd(lohiShuffle, hiloShuffle, _mm256_castsi256_pd(laneMask));
#endif
        }
    }

    // logic operations
    forceinline F64x4 VECCALL And(const F64x4& other) const { return _mm256_and_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Or(const F64x4& other) const { return _mm256_or_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Xor(const F64x4& other) const { return _mm256_xor_pd(Data, other.Data); }
    forceinline F64x4 VECCALL AndNot(const F64x4& other) const { return _mm256_andnot_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Not() const
    {
        alignas(32) static constexpr int64_t i[] = { -1, -1, -1, -1 };
        return _mm256_xor_pd(Data, _mm256_load_pd(reinterpret_cast<const double*>(i)));
    }

    // arithmetic operations
    forceinline F64x4 VECCALL Add(const F64x4& other) const { return _mm256_add_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Sub(const F64x4& other) const { return _mm256_sub_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Mul(const F64x4& other) const { return _mm256_mul_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Div(const F64x4& other) const { return _mm256_div_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Max(const F64x4& other) const { return _mm256_max_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Min(const F64x4& other) const { return _mm256_min_pd(Data, other.Data); }
    //F64x4 Rcp() const { return _mm256_rcp_pd(Data); }
    forceinline F64x4 VECCALL Sqrt() const { return _mm256_sqrt_pd(Data); }
    //F64x4 Rsqrt() const { return _mm256_rsqrt_pd(Data); }

    forceinline F64x4 VECCALL MulAdd(const F64x4& muler, const F64x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm256_add_pd(_mm256_mul_pd(Data, muler.Data), adder.Data);
#endif
    }
    forceinline F64x4 VECCALL MulSub(const F64x4& muler, const F64x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm256_sub_pd(_mm256_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    forceinline F64x4 VECCALL NMulAdd(const F64x4& muler, const F64x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm256_sub_pd(adder.Data, _mm256_mul_pd(Data, muler.Data));
#endif
    }
    forceinline F64x4 VECCALL NMulSub(const F64x4& muler, const F64x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm256_xor_pd(_mm256_add_pd(_mm256_mul_pd(Data, muler.Data), suber.Data), _mm256_set1_pd(-0.));
#endif
    }
    forceinline F64x4 VECCALL operator*(const F64x4& other) const { return Mul(other); }
    forceinline F64x4 VECCALL operator/(const F64x4& other) const { return Div(other); }

};


struct alignas(__m256) F32x8 : public detail::CommonOperators<F32x8>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,32,8,0 };
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
    F32x8(const Pack<F32x4, 2>& pack) : Data(_mm256_set_m128(pack[1].Data, pack[0].Data)) {}
    F32x8(const float lo0, const float lo1, const float lo2, const float lo3, const float lo4, const float lo5, const float lo6, const float hi7)
        :Data(_mm256_setr_ps(lo0, lo1, lo2, lo3, lo4, lo5, lo6, hi7)) { }

    forceinline constexpr operator const __m256&() const noexcept { return Data; }
    forceinline void VECCALL Load(const float *ptr) { Data = _mm256_loadu_ps(ptr); }
    forceinline void VECCALL Save(float *ptr) const { _mm256_storeu_ps(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline F32x8 VECCALL Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        if constexpr (Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Lo3 < 4 &&
            Lo4 - Lo0 == 4 && Lo5 - Lo1 == 4 && Lo6 - Lo2 == 4 && Hi7 - Lo3 == 4) // no cross lane and same shuffle for two lane
        {
            return _mm256_permute_ps(Data, (Lo3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
        }
        else
        {
#if COMMON_SIMD_LV >= 200
            const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
            return _mm256_permutevar8x32_ps(Data, mask);
#else
            constexpr uint8_t lane0 = Lo0 & 0b100, lane1 = Lo1 & 0b100, lane2 = Lo2 & 0b100, lane3 = Lo3 & 0b100,
                lane4 = Lo4 & 0b100, lane5 = Lo5 & 0b100, lane6 = Lo6 & 0b100, lane7 = Hi7 & 0b100;
            constexpr uint8_t off0 = Lo0 & 0b11, off1 = Lo1 & 0b11, off2 = Lo2 & 0b11, off3 = Lo3 & 0b11,
                off4 = Lo4 & 0b11, off5 = Lo5 & 0b11, off6 = Lo6 & 0b11, off7 = Hi7 & 0b11;
            if constexpr (lane0 == lane1 && lane2 == lane3 && lane4 == lane5 && lane6 == lane7 &&
                off0 == off4 && off1 == off5 && off2 == off6 && off3 == off7)
            {
                const auto tmpa = _mm256_permute2f128_ps(Data, Data, (lane0 ? 0x1 : 0x0) + (lane4 ? 0x10 : 0x00));
                const auto tmpb = _mm256_permute2f128_ps(Data, Data, (lane2 ? 0x1 : 0x0) + (lane6 ? 0x10 : 0x00));
                return _mm256_shuffle_ps(tmpa, tmpb, (off3 << 6) + (off2 << 4) + (off1 << 2) + off0);
            }
            else
            {
                const auto tmpa = _mm256_permute2f128_ps(Data, Data, (lane0 ? 0x1 : 0x0) + (lane5 ? 0x10 : 0x00));
                const auto tmpb = _mm256_permute2f128_ps(Data, Data, (lane2 ? 0x1 : 0x0) + (lane7 ? 0x10 : 0x00));
                const auto f0257 = _mm256_shuffle_ps(tmpa, tmpb, (off7 << 6) + (off2 << 4) + (off5 << 2) + off0);
                const auto tmpc = _mm256_permute2f128_ps(Data, Data, (lane1 ? 0x1 : 0x0) + (lane4 ? 0x10 : 0x00));
                const auto tmpd = _mm256_permute2f128_ps(Data, Data, (lane3 ? 0x1 : 0x0) + (lane6 ? 0x10 : 0x00));
                const auto f1346 = _mm256_shuffle_ps(tmpc, tmpd, (off6 << 6) + (off3 << 4) + (off4 << 2) + off1);
                return _mm256_blend_ps(f0257, f1346, 0b01011010);
            }
#endif
        }
    }
    forceinline F32x8 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const
    {
#if COMMON_SIMD_LV >= 200
        const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
        return _mm256_permutevar8x32_ps(Data, mask);
#else
        if (Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Lo3 < 4 && Lo4 >= 4 && Lo5 >= 4 && Lo6 >= 4 && Hi7 >= 4) // no cross lane
        {
            const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
            return _mm256_permutevar_ps(Data, mask);
        }
        else
        {
            const auto offMask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
            const auto laneMask = _mm256_setr_epi32((int64_t)(Lo0) << 29, (int64_t)(Lo1) << 29, (int64_t)(Lo2) << 29, (int64_t)(Lo3) << 29, 
                (int64_t)(~Lo4) << 29, (int64_t)(~Lo5) << 29, (int64_t)(~Lo6) << 29, (int64_t)(~Hi7) << 29);
            const auto hilo = _mm256_permute2f128_ps(Data, Data, 0b0001);//01234567->45670123
            const auto lohiShuffle = _mm256_permutevar_ps(Data, offMask);
            const auto hiloShuffle = _mm256_permutevar_ps(hilo, offMask);
            return _mm256_blendv_ps(lohiShuffle, hiloShuffle, _mm256_castsi256_ps(laneMask));
        }
#endif
}

    // logic operations
    forceinline F32x8 VECCALL And(const F32x8& other) const
    {
        return _mm256_and_ps(Data, other.Data);
    }
    forceinline F32x8 VECCALL Or(const F32x8& other) const
    {
        return _mm256_or_ps(Data, other.Data);
    }
    forceinline F32x8 VECCALL Xor(const F32x8& other) const
    {
        return _mm256_xor_ps(Data, other.Data);
    }
    forceinline F32x8 VECCALL AndNot(const F32x8& other) const
    {
        return _mm256_andnot_ps(Data, other.Data);
    }
    forceinline F32x8 VECCALL Not() const
    {
        alignas(32) static constexpr int64_t i[] = { -1, -1, -1, -1 };
        return _mm256_xor_ps(Data, _mm256_load_ps(reinterpret_cast<const float*>(i)));
    }

    // arithmetic operations
    forceinline F32x8 VECCALL Add(const F32x8& other) const { return _mm256_add_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Sub(const F32x8& other) const { return _mm256_sub_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Mul(const F32x8& other) const { return _mm256_mul_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Div(const F32x8& other) const { return _mm256_div_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Max(const F32x8& other) const { return _mm256_max_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Min(const F32x8& other) const { return _mm256_min_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Rcp() const { return _mm256_rcp_ps(Data); }
    forceinline F32x8 VECCALL Sqrt() const { return _mm256_sqrt_ps(Data); }
    forceinline F32x8 VECCALL Rsqrt() const { return _mm256_rsqrt_ps(Data); }
    forceinline F32x8 VECCALL MulAdd(const F32x8& muler, const F32x8& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm256_add_ps(_mm256_mul_ps(Data, muler.Data), adder.Data);
#endif
    }
    forceinline F32x8 VECCALL MulSub(const F32x8& muler, const F32x8& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm256_sub_ps(_mm256_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    forceinline F32x8 VECCALL NMulAdd(const F32x8& muler, const F32x8& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm256_sub_ps(adder.Data, _mm256_mul_ps(Data, muler.Data));
#endif
    }
    forceinline F32x8 VECCALL NMulSub(const F32x8& muler, const F32x8& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm256_xor_ps(_mm256_add_ps(_mm256_mul_ps(Data, muler.Data), suber.Data), _mm256_set1_ps(-0.));
#endif
    }
    template<DotPos Mul, DotPos Res>
    forceinline F32x8 VECCALL Dot2(const F32x8& other) const 
    { 
        return _mm256_dp_ps(Data, other.Data, static_cast<uint8_t>(Mul) << 4 | static_cast<uint8_t>(Res));
    }
    forceinline F32x8 VECCALL operator*(const F32x8& other) const { return Mul(other); }
    forceinline F32x8 VECCALL operator/(const F32x8& other) const { return Div(other); }

};


#if COMMON_SIMD_LV >= 200
struct alignas(__m256i) I64x4 : public detail::Int256Common<I64x4, int64_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Signed,64,4,0 };
    static constexpr size_t Count = 4;
    union
    {
        __m256i Data;
        int64_t Val[4];
    };
    constexpr I64x4() : Data() { }
    explicit I64x4(const int64_t* ptr) : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    constexpr I64x4(const __m256i val) :Data(val) { }
    I64x4(const int64_t val) :Data(_mm256_set1_epi64x(val)) { }
    I64x4(const Pack<I64x2, 2>& pack) : Data(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}
    I64x4(const int64_t lo0, const int64_t lo1, const int64_t lo2, const int64_t hi3) :Data(_mm256_setr_epi64x(lo0, lo1, lo2, hi3)) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline I64x4 VECCALL Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm256_permute4x64_epi64(Data, (Hi3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
    }
    forceinline I64x4 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const
    {
        //assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        const auto mask = _mm256_setr_epi32(Lo0*2, Lo0*2+1, Lo1*2, Lo1*2+1, Lo2*2, Lo2*2+1, Hi3*2, Hi3*2+1);
        return _mm256_permutevar8x32_epi32(Data, mask);
    }

    // arithmetic operations
    forceinline I64x4 VECCALL Add(const I64x4& other) const { return _mm256_add_epi64(Data, other.Data); }
    forceinline I64x4 VECCALL Sub(const I64x4& other) const { return _mm256_sub_epi64(Data, other.Data); }
    forceinline I64x4 VECCALL Max(const I64x4& other) const { return _mm256_max_epi64(Data, other.Data); }
    forceinline I64x4 VECCALL Min(const I64x4& other) const { return _mm256_min_epi64(Data, other.Data); }
    forceinline I64x4 VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi64(Data, I64x2(bits)); }
    forceinline I64x4 VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi64(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline I64x4 VECCALL ShiftRightArth() const { return _mm256_srai_epi64(Data, N); }
    template<uint8_t N>
    forceinline I64x4 VECCALL ShiftRightLogic() const { return _mm256_srli_epi64(Data, N); }
    template<uint8_t N>
    forceinline I64x4 VECCALL ShiftLeftLogic() const { return _mm256_slli_epi64(Data, N); }
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
    I32Common8(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7)
        :Data(_mm256_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(lo3), static_cast<int32_t>(lo4), static_cast<int32_t>(lo5), static_cast<int32_t>(lo6), static_cast<int32_t>(hi7))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
        return _mm256_permutevar8x32_epi32(Data, mask);
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const
    {
        //static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
        return _mm256_permutevar8x32_epi32(Data, mask);
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm256_add_epi32(Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm256_sub_epi32(Data, other.Data); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi32(Data, I64x2(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm256_srli_epi32(Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const { return _mm256_slli_epi32(Data, N); }
#if COMMON_SIMD_LV >= 41
    forceinline T VECCALL MulLo(const T& other) const { return _mm256_mullo_epi32(Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
#endif
};


struct alignas(__m256i) I32x8 : public I32Common8<I32x8, int32_t>, public detail::Int256Common<I32x8, int32_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Signed,32,8,0 };
    using I32Common8<I32x8, int32_t>::I32Common8;
    I32x8(const Pack<I32x4, 2>& pack) : I32Common8(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline I32x8 VECCALL Max(const I32x8& other) const { return _mm256_max_epi32(Data, other.Data); }
    forceinline I32x8 VECCALL Min(const I32x8& other) const { return _mm256_min_epi32(Data, other.Data); }
    Pack<I64x4, 2> MulX(const I32x8& other) const
    {
        return { I64x4(_mm256_mul_epi32(Data, other.Data)), I64x4(_mm256_mul_epi32(ShuffleHiLo(), other.ShuffleHiLo())) };
    }
    forceinline I32x8 VECCALL operator>>(const uint8_t bits) const { return _mm256_sra_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline I32x8 VECCALL ShiftRightArth() const { return _mm256_srai_epi32(Data, N); }
};

struct alignas(__m256i) U32x8 : public I32Common8<U32x8, uint32_t>, public detail::Int256Common<U32x8, uint32_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Unsigned,32,8,0 };
    using I32Common8<U32x8, uint32_t>::I32Common8;
    U32x8(const Pack<U32x4, 2>& pack) : I32Common8(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline U32x8 VECCALL Max(const U32x8& other) const { return _mm256_max_epu32(Data, other.Data); }
    forceinline U32x8 VECCALL Min(const U32x8& other) const { return _mm256_min_epu32(Data, other.Data); }
    forceinline U32x8 VECCALL operator>>(const uint8_t bits) const { return _mm256_srl_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline U32x8 VECCALL ShiftRightArth() const { return _mm256_srli_epi32(Data, N); }
    Pack<I64x4, 2> MulX(const U32x8& other) const
    {
        return { I64x4(_mm256_mul_epu32(Data, other.Data)), I64x4(_mm256_mul_epu32(ShuffleHiLo(), other.ShuffleHiLo())) };
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
    I16Common16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15)
        :Data(_mm256_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(lo7),
            static_cast<int16_t>(lo8), static_cast<int16_t>(lo9), static_cast<int16_t>(lo10), static_cast<int16_t>(lo11), static_cast<int16_t>(lo12), static_cast<int16_t>(lo13), static_cast<int16_t>(lo14), static_cast<int16_t>(hi15))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        const auto mask = _mm256_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1),
            static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1), static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1),
            static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1), static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1),
            static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1), static_cast<int8_t>(Lo7 * 2), static_cast<int8_t>(Lo7 * 2 + 1),
            static_cast<int8_t>(Lo8 * 2), static_cast<int8_t>(Lo8 * 2 + 1), static_cast<int8_t>(Lo9 * 2), static_cast<int8_t>(Lo9 * 2 + 1), 
            static_cast<int8_t>(Lo10 * 2), static_cast<int8_t>(Lo10 * 2 + 1), static_cast<int8_t>(Lo11 * 2), static_cast<int8_t>(Lo11 * 2 + 1), 
            static_cast<int8_t>(Lo12 * 2), static_cast<int8_t>(Lo12 * 2 + 1), static_cast<int8_t>(Lo13 * 2), static_cast<int8_t>(Lo13 * 2 + 1), 
            static_cast<int8_t>(Lo14 * 2), static_cast<int8_t>(Lo14 * 2 + 1), static_cast<int8_t>(Hi15 * 2), static_cast<int8_t>(Hi15 * 2 + 1));
        return _mm256_shuffle_epi8(Data, mask);
    }
    
    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm256_add_epi16(Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm256_sub_epi16(Data, other.Data); }
    forceinline T VECCALL MulLo(const T& other) const { return _mm256_mullo_epi16(Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm256_sll_epi16(Data, I64x2(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm256_srl_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const { return _mm256_slli_epi16(Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm256_srli_epi16(Data, N); }
    Pack<I32x8, 2> MulX(const T& other) const
    {
        const auto los = MulLo(other), his = static_cast<const T*>(this)->MulHi(other);
        return { _mm256_unpacklo_epi16(los, his),_mm256_unpackhi_epi16(los, his) };
    }
};


struct alignas(__m256i) I16x16 : public I16Common16<I16x16, int16_t>, public detail::Int256Common<I16x16, int16_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Signed,16,16,0 };
    using I16Common16<I16x16, int16_t>::I16Common16;
    I16x16(const Pack<I16x8, 2>& pack) : I16Common16(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline I16x16 VECCALL SatAdd(const I16x16& other) const { return _mm256_adds_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL SatSub(const I16x16& other) const { return _mm256_subs_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL MulHi(const I16x16& other) const { return _mm256_mulhi_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL Max(const I16x16& other) const { return _mm256_max_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL Min(const I16x16& other) const { return _mm256_min_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL operator>>(const uint8_t bits) const { return _mm256_sra_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline I16x16 VECCALL ShiftRightArth() const { return _mm256_srai_epi16(Data, N); }
};


struct alignas(__m256i) U16x16 : public I16Common16<U16x16, int16_t>, public detail::Int256Common<U16x16, uint16_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Unsigned,16,16,0 };
    using I16Common16<U16x16, int16_t>::I16Common16;
    U16x16(const Pack<U16x8, 2>& pack) : I16Common16(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline U16x16 VECCALL SatAdd(const U16x16& other) const { return _mm256_adds_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL SatSub(const U16x16& other) const { return _mm256_subs_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL MulHi(const U16x16& other) const { return _mm256_mulhi_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL Div(const uint16_t other) const { return MulHi(U16x16(static_cast<uint16_t>(UINT16_MAX / other))); }
    forceinline U16x16 VECCALL operator/(const uint16_t other) const { return Div(other); }
    forceinline U16x16 VECCALL Max(const U16x16& other) const { return _mm256_max_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL Min(const U16x16& other) const { return _mm256_min_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL operator>>(const uint8_t bits) const { return _mm256_srl_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline U16x16 VECCALL ShiftRightArth() const { return _mm256_srli_epi16(Data, N); }
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
    I8Common32(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, 
        const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E lo15,
        const E lo16, const E lo17, const E lo18, const E lo19, const E lo20, const E lo21, const E lo22, const E lo23,
        const E lo24, const E lo25, const E lo26, const E lo27, const E lo28, const E lo29, const E lo30, const E hi31)
        :Data(_mm256_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4), static_cast<int8_t>(lo5), static_cast<int8_t>(lo6), static_cast<int8_t>(lo7), static_cast<int8_t>(lo8),
            static_cast<int8_t>(lo9), static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), static_cast<int8_t>(lo12), static_cast<int8_t>(lo13),
            static_cast<int8_t>(lo14), static_cast<int8_t>(lo15), static_cast<int8_t>(lo16), static_cast<int8_t>(lo17), static_cast<int8_t>(lo18),
            static_cast<int8_t>(lo19), static_cast<int8_t>(lo20), static_cast<int8_t>(lo21), static_cast<int8_t>(lo22), static_cast<int8_t>(lo23), 
            static_cast<int8_t>(lo24), static_cast<int8_t>(lo25), static_cast<int8_t>(lo26), static_cast<int8_t>(lo27), static_cast<int8_t>(lo28), 
            static_cast<int8_t>(lo29), static_cast<int8_t>(lo30), static_cast<int8_t>(hi31))) { }

    // shuffle operations
#if COMMON_SIMD_LV >= 31
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Lo15,
        uint8_t Lo16, uint8_t Lo17, uint8_t Lo18, uint8_t Lo19, uint8_t Lo20, uint8_t Lo21, uint8_t Lo22, uint8_t Lo23, uint8_t Lo24, uint8_t Lo25, uint8_t Lo26, uint8_t Lo27, uint8_t Lo28, uint8_t Lo29, uint8_t Lo30, uint8_t Hi31>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 32 && Lo1 < 32 && Lo2 < 32 && Lo3 < 32 && Lo4 < 32 && Lo5 < 32 && Lo6 < 32 && Lo7 < 32
            && Lo8 < 32 && Lo9 < 32 && Lo10 < 32 && Lo11 < 32 && Lo12 < 32 && Lo13 < 32 && Lo14 < 32 && Lo15 < 32
            && Lo16 < 32 && Lo17 < 32 && Lo18 < 32 && Lo19 < 32 && Lo20 < 32 && Lo21 < 32 && Lo22 < 32 && Lo23 < 32
            && Lo24 < 32 && Lo25 < 32 && Lo26 < 32 && Lo27 < 32 && Lo28 < 32 && Lo29 < 32 && Lo30 < 32 && Hi31 < 32, "shuffle index should be in [0,31]");
        const auto mask = _mm256_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Lo15), static_cast<int8_t>(Lo16), static_cast<int8_t>(Lo17), static_cast<int8_t>(Lo18),
            static_cast<int8_t>(Lo19), static_cast<int8_t>(Lo20), static_cast<int8_t>(Lo21), static_cast<int8_t>(Lo22), static_cast<int8_t>(Lo23),
            static_cast<int8_t>(Lo24), static_cast<int8_t>(Lo25), static_cast<int8_t>(Lo26), static_cast<int8_t>(Lo27), static_cast<int8_t>(Lo28),
            static_cast<int8_t>(Lo29), static_cast<int8_t>(Lo30), static_cast<int8_t>(Hi31));
        return _mm256_shuffle_epi8(Data, mask);
    }
#endif

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm256_add_epi8(Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm256_sub_epi8(Data, other.Data); }
};


struct alignas(__m256i) I8x32 : public I8Common32<I8x32, int8_t>, public detail::Int256Common<I8x32, int8_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Signed,8,32,0 };
    using I8Common32<I8x32, int8_t>::I8Common32;
    I8x32(const Pack<I8x16, 2>& pack) : I8Common32(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline I8x32 VECCALL SatAdd(const I8x32& other) const { return _mm256_adds_epi8(Data, other.Data); }
    forceinline I8x32 VECCALL SatSub(const I8x32& other) const { return _mm256_subs_epi8(Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    forceinline I8x32 VECCALL Max(const I8x32& other) const { return _mm256_max_epi8(Data, other.Data); }
    forceinline I8x32 VECCALL Min(const I8x32& other) const { return _mm256_min_epi8(Data, other.Data); }
#endif
    template<typename T>
    typename CastTyper<I8x32, T>::Type VECCALL Cast() const;
#if COMMON_SIMD_LV >= 41
    Pack<I16x16, 2> VECCALL MulX(const I8x32& other) const;
    forceinline I8x32 VECCALL MulHi(const I8x32& other) const
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightArth<8>(), hi = full[1].ShiftRightArth<8>();
        return _mm256_packs_epi16(lo, hi);
    }
    forceinline I8x32 VECCALL MulLo(const I8x32& other) const 
    { 
        const auto full = MulX(other);
        const auto mask = I16x16(INT16_MIN);
        const auto signlo = full[0].And(mask).ShiftRightArth<8>(), signhi = full[1].And(mask).ShiftRightArth<8>();
        const auto lo = full[0] & signlo, hi = full[1] & signhi;
        return _mm256_packs_epi16(lo, hi);
    }
    forceinline I8x32 VECCALL operator*(const I8x32& other) const { return MulLo(other); }
#endif
};
template<> forceinline Pack<I16x16, 2> VECCALL I8x32::Cast<I16x16>() const
{
    return { _mm256_cvtepi8_epi16(_mm256_castsi256_si128(Data)), _mm256_cvtepi8_epi16(_mm256_extracti128_si256(Data, 1)) };
}
forceinline Pack<I16x16, 2> VECCALL I8x32::MulX(const I8x32& other) const
{
    const auto self16 = Cast<I16x16>(), other16 = other.Cast<I16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}

struct alignas(__m256i) U8x32 : public I8Common32<U8x32, uint8_t>, public detail::Int256Common<U8x32, uint8_t>
{
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Unsigned,8,32,0 };
    using I8Common32<U8x32, uint8_t>::I8Common32;
    U8x32(const Pack<U8x16, 2>& pack) : I8Common32(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}

    // arithmetic operations
    forceinline U8x32 VECCALL SatAdd(const U8x32& other) const { return _mm256_adds_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL SatSub(const U8x32& other) const { return _mm256_subs_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL Max(const U8x32& other) const { return _mm256_max_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL Min(const U8x32& other) const { return _mm256_min_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL operator*(const U8x32& other) const { return MulLo(other); }
    template<typename T>
    typename CastTyper<U8x32, T>::Type VECCALL Cast() const;
    forceinline U8x32 VECCALL MulHi(const U8x32& other) const
    {
        const auto full = MulX(other);
        return _mm256_packus_epi16(full[0].ShiftRightLogic<8>(), full[1].ShiftRightLogic<8>());
    }
    forceinline U8x32 VECCALL MulLo(const U8x32& other) const
    {
        const U16x16 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        const U16x16 mask((uint16_t)0x00ff);
        return U8x32(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    Pack<U16x16, 2> VECCALL MulX(const U8x32& other) const;
};


template<> forceinline Pack<I16x16, 2> VECCALL U8x32::Cast<I16x16>() const
{
    return { _mm256_cvtepu8_epi16(_mm256_castsi256_si128(Data)), _mm256_cvtepu8_epi16(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U16x16, 2> VECCALL U8x32::Cast<U16x16>() const
{
    return { _mm256_cvtepu8_epi16(_mm256_castsi256_si128(Data)), _mm256_cvtepu8_epi16(_mm256_extracti128_si256(Data, 1)) };
}
forceinline Pack<U16x16, 2> VECCALL U8x32::MulX(const U8x32& other) const
{
    const auto self16 = Cast<U16x16>(), other16 = other.Cast<U16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}

#endif

}
}
