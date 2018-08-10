#pragma once
#include "SIMD.hpp"

#define COMMON_SIMD_LV 200
#if COMMON_SIMD_LV < 20
#   error require at least SSE2
#endif

namespace common
{
namespace simd
{

struct I8x16;
struct U8x16;
struct I16x8;
struct U16x8;
struct I32x4;
struct U32x4;
struct F32x4;
struct I64x2;
struct F64x2;

template<typename T, size_t N>
struct alignas(T) Pack
{
    T Data[N];
    Pack(const T& val0, const T& val1) : Data{ val0,val1 } {}
    constexpr T& operator[](const size_t idx) { return Data[idx]; }
    constexpr const T& operator[](const size_t idx) const { return Data[idx]; }
};

template<typename From, typename To>
struct CastTyper
{
    using Type = std::conditional_t<(From::Count > To::Count), Pack<To, From::Count / To::Count>, To>;
};

namespace detail
{

template<typename T>
struct CommonOperators
{
    // logic operations
    T operator&(const T& other) const { return static_cast<const T*>(this)->And(other); }
    T operator|(const T& other) const { return static_cast<const T*>(this)->Or(other); }
    T operator^(const T& other) const { return static_cast<const T*>(this)->Xor(other); }
    T operator~() const { return static_cast<const T*>(this)->Not(); }
    // arithmetic operations
    T operator+(const T& other) const { return static_cast<const T*>(this)->Add(other); }
    T operator-(const T& other) const { return static_cast<const T*>(this)->Sub(other); }
};

template<typename T, typename E>
struct IntCommon : public CommonOperators<T>
{
    // logic operations
    T And(const T& other) const
    {
        return _mm_and_si128(*static_cast<const T*>(this), other);
    }
    T Or(const T& other) const
    {
        return _mm_or_si128(*static_cast<const T*>(this), other);
    }
    T Xor(const T& other) const
    {
        return _mm_xor_si128(*static_cast<const T*>(this), other);
    }
    T AndNot(const T& other) const
    {
        return _mm_andnot_si128(*static_cast<const T*>(this), other);
    }
    T Not() const
    {
        return _mm_xor_si128(*static_cast<const T*>(this), _mm_set1_epi8(-1));
    }
    template<typename T1 = E>
    void Save(T1 *ptr) { _mm_storeu_si128(reinterpret_cast<T1*>(ptr), *static_cast<const T*>(this)); }
};

}


struct alignas(__m128d) F64x2 : public detail::CommonOperators<F64x2>
{
    static constexpr size_t Count = 2;
    union
    {
        __m128d Data;
        double Val[2];
    };
    F64x2() :Data(_mm_undefined_pd()) { }
    constexpr F64x2(const __m128d val) : Data(val) { }
    F64x2(const double val) : Data(_mm_set1_pd(val)) { }
    F64x2(const double lo, const double hi) : Data(_mm_setr_pd(lo, hi)) { }
    constexpr operator const __m128d&() { return Data; }
    template<typename T = double>
    void Save(T *ptr) const { _mm_storeu_pd(reinterpret_cast<double*>(ptr), Data); }

    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    F64x2 Shuffle() const
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_pd(Data, (Hi << 1) + Lo);
#else
        return _mm_shuffle_pd(Data, Data, (Hi << 1) + Lo);
#endif
    }

    // logic operations
    F64x2 And(const F64x2& other) const
    {
        return _mm_and_pd(Data, other.Data);
    }
    F64x2 Or(const F64x2& other) const
    {
        return _mm_or_pd(Data, other.Data);
    }
    F64x2 Xor(const F64x2& other) const
    {
        return _mm_xor_pd(Data, other.Data);
    }
    F64x2 AndNot(const F64x2& other) const
    {
        return _mm_andnot_pd(Data, other.Data);
    }
    F64x2 Not() const
    {
        static const int64_t i = -1;
        return _mm_xor_pd(Data, _mm_set1_pd(*reinterpret_cast<const double*>(&i)));
    }

    // arithmetic operations
    F64x2 Add(const F64x2& other) const { return _mm_add_pd(Data, other.Data); }
    F64x2 Sub(const F64x2& other) const { return _mm_sub_pd(Data, other.Data); }
    F64x2 Mul(const F64x2& other) const { return _mm_mul_pd(Data, other.Data); }
    F64x2 Div(const F64x2& other) const { return _mm_div_pd(Data, other.Data); }
    F64x2 Max(const F64x2& other) const { return _mm_max_pd(Data, other.Data); }
    F64x2 Min(const F64x2& other) const { return _mm_min_pd(Data, other.Data); }
    //F64x2 Rcp() const { return _mm_rcp_pd(Data); }
    F64x2 Sqrt() const { return _mm_sqrt_pd(Data); }
    //F64x2 Rsqrt() const { return _mm_rsqrt_pd(Data); }
    F64x2 MulAdd(const F64x2& muler, const F64x2& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_add_pd(_mm_mul_pd(Data, muler.Data), adder.Data);
#endif
    }
    F64x2 MulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_sub_pd(_mm_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    F64x2 NMulAdd(const F64x2& muler, const F64x2& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_sub_pd(adder.Data, _mm_mul_pd(Data, muler.Data));
#endif
    }
    F64x2 NMulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_xor_pd(_mm_add_pd(_mm_mul_pd(Data, muler.Data), suber.Data), _mm_set1_pd(-0.));
#endif
    }
    F64x2 operator*(const F64x2& other) const { return Mul(other); }
    F64x2 operator/(const F64x2& other) const { return Div(other); }

};


struct alignas(__m128) F32x4 : public detail::CommonOperators<F32x4>
{
    static constexpr size_t Count = 4;
    union
    {
        __m128 Data;
        float Val[2];
    };
    F32x4() :Data(_mm_undefined_ps()) { }
    constexpr F32x4(const __m128 val) :Data(val) { }
    F32x4(const float val) :Data(_mm_set1_ps(val)) { }
    F32x4(const float lo0, const float lo1, const float lo2, const float hi3) :Data(_mm_setr_ps(lo0, lo1, lo2, hi3)) { }
    constexpr operator const __m128&() { return Data; }
    template<typename T = float>
    void Save(T *ptr) const { _mm_storeu_ps(reinterpret_cast<float*>(ptr), Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    F32x4 Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_ps(Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#else
        return _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#endif
    }

    // logic operations
    F32x4 And(const F32x4& other) const
    {
        return _mm_and_ps(Data, other.Data);
    }
    F32x4 Or(const F32x4& other) const
    {
        return _mm_or_ps(Data, other.Data);
    }
    F32x4 Xor(const F32x4& other) const
    {
        return _mm_xor_ps(Data, other.Data);
    }
    F32x4 AndNot(const F32x4& other) const
    {
        return _mm_andnot_ps(Data, other.Data);
    }
    F32x4 Not() const
    {
        static const int32_t i = -1;
        return _mm_xor_ps(Data, _mm_set1_ps(*reinterpret_cast<const float*>(&i)));
    }

    // arithmetic operations
    F32x4 Add(const F32x4& other) const { return _mm_add_ps(Data, other.Data); }
    F32x4 Sub(const F32x4& other) const { return _mm_sub_ps(Data, other.Data); }
    F32x4 Mul(const F32x4& other) const { return _mm_mul_ps(Data, other.Data); }
    F32x4 Div(const F32x4& other) const { return _mm_div_ps(Data, other.Data); }
    F32x4 Max(const F32x4& other) const { return _mm_max_ps(Data, other.Data); }
    F32x4 Min(const F32x4& other) const { return _mm_min_ps(Data, other.Data); }
    F32x4 Rcp() const { return _mm_rcp_ps(Data); }
    F32x4 Sqrt() const { return _mm_sqrt_ps(Data); }
    F32x4 Rsqrt() const { return _mm_rsqrt_ps(Data); }
    F32x4 MulAdd(const F32x4& muler, const F32x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_add_ps(_mm_mul_ps(Data, muler.Data), adder.Data);
#endif
    }
    F32x4 MulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_sub_ps(_mm_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    F32x4 NMulAdd(const F32x4& muler, const F32x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_sub_ps(adder.Data, _mm_mul_ps(Data, muler.Data));
#endif
    }
    F32x4 NMulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_xor_ps(_mm_add_ps(_mm_mul_ps(Data, muler.Data), suber.Data), _mm_set1_ps(-0.));
#endif
    }
    F32x4 operator*(const F32x4& other) const { return Mul(other); }
    F32x4 operator/(const F32x4& other) const { return Div(other); }

};


struct alignas(__m128i) I64x2 : public detail::IntCommon<I64x2, int64_t>
{
    static constexpr size_t Count = 2;
    union
    {
        __m128i Data;
        int64_t Ints[2];
    };
    I64x2() :Data(_mm_undefined_si128()) { }
    constexpr I64x2(const __m128i val) :Data(val) { }
    I64x2(const int64_t val) :Data(_mm_set1_epi64x(val)) { }
    I64x2(const int64_t lo, const int64_t hi) :Data(_mm_set_epi64x(hi, lo)) { }

    operator const __m128i&() { return Data; }

    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    I64x2 Shuffle() const
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        return _mm_shuffle_epi32(Data, _MM_SHUFFLE(Hi*2+1, Hi*2, Lo*2+1, Lo*2));
    }

    // arithmetic operations
    I64x2 Add(const I64x2& other) const { return _mm_add_epi64(Data, other.Data); }
    I64x2 Sub(const I64x2& other) const { return _mm_sub_epi64(Data, other.Data); }
    I64x2 Max(const I64x2& other) const { return _mm_max_epi64(Data, other.Data); }
    I64x2 Min(const I64x2& other) const { return _mm_min_epi64(Data, other.Data); }
    I64x2 ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi64(Data, I64x2(bits)); }
    I64x2 ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi64(Data, I64x2(bits)); }
    template<uint8_t N>
    I64x2 ShiftRightArth() const { return _mm_srai_epi64(Data, N); }
    template<uint8_t N>
    I64x2 ShiftRightLogic() const { return _mm_srli_epi64(Data, N); }
    template<uint8_t N>
    I64x2 ShiftLeftLogic() const { return _mm_slli_epi64(Data, N); }
};


template<typename T, typename E>
struct alignas(__m128i) I32Common
{
    static constexpr size_t Count = 4;
    union
    {
        __m128i Data;
        E Val[4];
    };
    I32Common() :Data(_mm_undefined_si128()) { }
    constexpr I32Common(const __m128i val) : Data(val) { }
    I32Common(const E val) : Data(_mm_set1_epi32(static_cast<int32_t>(val))) { }
    I32Common(const E lo0, const E lo1, const E lo2, const E hi3)
        : Data(_mm_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(hi3))) { }
    constexpr operator const __m128i&() const { return Data; }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    T Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm_shuffle_epi32(Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
    }

    // arithmetic operations
    T Add(const T& other) const { return _mm_add_epi32(Data, other.Data); }
    T Sub(const T& other) const { return _mm_sub_epi32(Data, other.Data); }
    T operator+(const T& other) const { return Add(other); }
    T operator-(const T& other) const { return Sub(other); }
    T ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi32(Data, I64x2(bits)); }
    T ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    T ShiftRightLogic() const { return _mm_srli_epi32(Data, N); }
    template<uint8_t N>
    T ShiftLeftLogic() const { return _mm_slli_epi32(Data, N); }
#if COMMON_SIMD_LV >= 41
    T MulLo(const T& other) const { return _mm_mullo_epi32(Data, other.Data); }
    T operator*(const T& other) const { return MulLo(other); }
#endif
};


struct alignas(__m128i) I32x4 : public I32Common<I32x4, int32_t>, public detail::IntCommon<I32x4, int32_t>
{
    using I32Common<I32x4, int32_t>::I32Common;
    using I32Common<I32x4, int32_t>::operator const __m128i &;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    I32x4 Max(const I32x4& other) const { return _mm_max_epi32(Data, other.Data); }
    I32x4 Min(const I32x4& other) const { return _mm_min_epi32(Data, other.Data); }
    Pack<I64x2, 2> MulX(const I32x4& other) const
    {
        return { I64x2(_mm_mul_epi32(Data, other.Data)), I64x2(_mm_mul_epi32(Shuffle<2, 3, 0, 1>(), other.Shuffle<2, 3, 0, 1>())) };
    }
#endif
    I32x4 operator>>(const uint8_t bits) const { return _mm_sra_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    I32x4 ShiftRightArth() const { return _mm_srai_epi32(Data, N); }
};

struct alignas(__m128i) U32x4 : public I32Common<U32x4, uint32_t>, public detail::IntCommon<U32x4, uint32_t>
{
    using I32Common<U32x4, uint32_t>::I32Common;
    using I32Common<U32x4, uint32_t>::operator const __m128i &;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    U32x4 Max(const U32x4& other) const { return _mm_max_epu32(Data, other.Data); }
    U32x4 Min(const U32x4& other) const { return _mm_min_epu32(Data, other.Data); }
#endif
    U32x4 operator>>(const uint8_t bits) const { return _mm_srl_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    U32x4 ShiftRightArth() const { return _mm_srli_epi32(Data, N); }
    Pack<I64x2, 2> MulX(const U32x4& other) const
    {
        return { I64x2(_mm_mul_epu32(Data, other.Data)), I64x2(_mm_mul_epu32(Shuffle<2, 3, 0, 1>(), other.Shuffle<2, 3, 0, 1>())) };
    }
};


template<typename T, typename E>
struct alignas(__m128i) I16Common
{
    static constexpr size_t Count = 8;
    union
    {
        __m128i Data;
        E Val[8];
    };
    I16Common() :Data(_mm_undefined_si128()) { }
    constexpr I16Common(const __m128i val) :Data(val) { }
    I16Common(const E val) :Data(_mm_set1_epi16(val)) { }
    I16Common(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7)
        :Data(_mm_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(hi7))) { }
    constexpr operator const __m128i&() const { return Data; }

    // shuffle operations
#if COMMON_SIMD_LV >= 31
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    T Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        static const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm_shuffle_epi8(Data, mask);
    }
#endif
    
    // arithmetic operations
    T Add(const T& other) const { return _mm_add_epi16(Data, other.Data); }
    T Sub(const T& other) const { return _mm_sub_epi16(Data, other.Data); }
    T MulLo(const T& other) const { return _mm_mullo_epi16(Data, other.Data); }
    T operator*(const T& other) const { return MulLo(other); }
    T ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi16(Data, I64x2(bits)); }
    T ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    T ShiftLeftLogic() const { return _mm_slli_epi16(Data, N); }
    template<uint8_t N>
    T ShiftRightLogic() const { return _mm_srli_epi16(Data, N); }
    Pack<I32x4, 2> MulX(const T& other) const
    {
        const auto los = MulLo(other), his = static_cast<const T*>(this)->MulHi(other);
        return { _mm_unpacklo_epi16(los, his),_mm_unpackhi_epi16(los, his) };
    }
};


struct alignas(__m128i) I16x8 : public I16Common<I16x8, int16_t>, public detail::IntCommon<I16x8, int16_t>
{
    using I16Common<I16x8, int16_t>::I16Common;
    using I16Common<I16x8, int16_t>::operator const __m128i &;

    // arithmetic operations
    I16x8 SatAdd(const I16x8& other) const { return _mm_adds_epi16(Data, other.Data); }
    I16x8 SatSub(const I16x8& other) const { return _mm_subs_epi16(Data, other.Data); }
    I16x8 MulHi(const I16x8& other) const { return _mm_mulhi_epi16(Data, other.Data); }
    I16x8 Max(const I16x8& other) const { return _mm_max_epi16(Data, other.Data); }
    I16x8 Min(const I16x8& other) const { return _mm_min_epi16(Data, other.Data); }
    I16x8 operator>>(const uint8_t bits) const { return _mm_sra_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    I16x8 ShiftRightArth() const { return _mm_srai_epi16(Data, N); }
};


struct alignas(__m128i) U16x8 : public I16Common<U16x8, int16_t>, public detail::IntCommon<U16x8, uint16_t>
{
    using I16Common<U16x8, int16_t>::I16Common;
    using I16Common<U16x8, int16_t>::operator const __m128i &;

    // arithmetic operations
    U16x8 SatAdd(const U16x8& other) const { return _mm_adds_epu16(Data, other.Data); }
    U16x8 SatSub(const U16x8& other) const { return _mm_subs_epu16(Data, other.Data); }
    U16x8 MulHi(const U16x8& other) const { return _mm_mulhi_epu16(Data, other.Data); }
    U16x8 Div(const uint16_t other) const { return MulHi(U16x8(static_cast<uint16_t>(UINT16_MAX / other))); }
    U16x8 operator/(const uint16_t other) const { return Div(other); }
#if COMMON_SIMD_LV >= 41
    U16x8 Max(const U16x8& other) const { return _mm_max_epu16(Data, other.Data); }
    U16x8 Min(const U16x8& other) const { return _mm_min_epu16(Data, other.Data); }
#endif
    U16x8 operator>>(const uint8_t bits) const { return _mm_srl_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    U16x8 ShiftRightArth() const { return _mm_srli_epi16(Data, N); }
};


template<typename T, typename E>
struct alignas(__m128i) I8Common
{
    static constexpr size_t Count = 16;
    union
    {
        __m128i Data;
        E Val[16];
    };
    I8Common() :Data(_mm_undefined_si128()) { }
    constexpr I8Common(const __m128i val) :Data(val) { }
    I8Common(const E val) :Data(_mm_set1_epi8(val)) { }
    I8Common(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15)
        :Data(_mm_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4), static_cast<int8_t>(lo5), static_cast<int8_t>(lo6), static_cast<int8_t>(lo7), static_cast<int8_t>(lo8),
            static_cast<int8_t>(lo9), static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), static_cast<int8_t>(lo12), static_cast<int8_t>(lo13),
            static_cast<int8_t>(lo14), static_cast<int8_t>(hi15))) { }
    constexpr operator const __m128i&() const { return Data; }

    // shuffle operations
#if COMMON_SIMD_LV >= 31
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    T Shuffle() const
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        static const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm_shuffle_epi8(Data, mask);
    }
#endif

    // arithmetic operations
    T Add(const T& other) const { return _mm_add_epi8(Data, other.Data); }
    T Sub(const T& other) const { return _mm_sub_epi8(Data, other.Data); }
};


struct alignas(__m128i) I8x16 : public I8Common<I8x16, int8_t>, public detail::IntCommon<I8x16, int8_t>
{
    using I8Common<I8x16, int8_t>::I8Common;
    using I8Common<I8x16, int8_t>::operator const __m128i &;

    // arithmetic operations
    I8x16 SatAdd(const I8x16& other) const { return _mm_adds_epi8(Data, other.Data); }
    I8x16 SatSub(const I8x16& other) const { return _mm_subs_epi8(Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    I8x16 Max(const I8x16& other) const { return _mm_max_epi8(Data, other.Data); }
    I8x16 Min(const I8x16& other) const { return _mm_min_epi8(Data, other.Data); }
#endif
    I8x16 operator*(const I8x16& other) const { return MulLo(other); }
    template<typename T>
    typename CastTyper<I8x16, T>::Type Cast() const;
#if COMMON_SIMD_LV >= 41
    I8x16 MulHi(const I8x16& other) const 
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightArth<8>(), hi = full[1].ShiftRightArth<8>();
        return _mm_packs_epi16(lo, hi);
    }
    I8x16 MulLo(const I8x16& other) const 
    { 
        const auto full = MulX(other);
        const auto mask = I16x8(INT16_MIN);
        const auto signlo = full[0].And(mask).ShiftRightArth<8>(), signhi = full[1].And(mask).ShiftRightArth<8>();
        const auto lo = full[0] & signlo, hi = full[1] & signhi;
        return _mm_packs_epi16(lo, hi);
    }
    Pack<I16x8, 2> MulX(const I8x16& other) const;
#endif
};
#if COMMON_SIMD_LV >= 41
template<> Pack<I16x8, 2> I8x16::Cast<I16x8>() const
{
    return { _mm_cvtepi8_epi16(Data), _mm_cvtepi8_epi16(_mm_shuffle_epi32(Data, _MM_SHUFFLE(3,2,3,2))) };
}
Pack<I16x8, 2> I8x16::MulX(const I8x16& other) const
{
    const auto self16 = Cast<I16x8>(), other16 = other.Cast<I16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif

struct alignas(__m128i) U8x16 : public I8Common<U8x16, uint8_t>, public detail::IntCommon<U8x16, uint8_t>
{
    using I8Common<U8x16, uint8_t>::I8Common;
    using I8Common<U8x16, uint8_t>::operator const __m128i &;

    // arithmetic operations
    U8x16 SatAdd(const U8x16& other) const { return _mm_adds_epu8(Data, other.Data); }
    U8x16 SatSub(const U8x16& other) const { return _mm_subs_epu8(Data, other.Data); }
    U8x16 Max(const U8x16& other) const { return _mm_max_epu8(Data, other.Data); }
    U8x16 Min(const U8x16& other) const { return _mm_min_epu8(Data, other.Data); }
    U8x16 operator*(const U8x16& other) const { return MulLo(other); }
    template<typename T>
    typename CastTyper<U8x16, T>::Type Cast() const;
    U8x16 MulHi(const U8x16& other) const
    {
        const auto full = MulX(other);
        return _mm_packus_epi16(full[0].ShiftRightLogic<8>(), full[1].ShiftRightLogic<8>());
    }
    U8x16 MulLo(const U8x16& other) const
    {
        const U16x8 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        static const U16x8 mask((int16_t)0xff);
        return U8x16(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    Pack<U16x8, 2> MulX(const U8x16& other) const;
};


template<> Pack<I16x8, 2> U8x16::Cast<I16x8>() const
{
    const auto tmp = _mm_setzero_si128();
    return { _mm_unpacklo_epi8(*this, tmp), _mm_unpackhi_epi8(*this, tmp) };
}
template<> Pack<U16x8, 2> U8x16::Cast<U16x8>() const
{
    const auto tmp = _mm_setzero_si128();
    return { _mm_unpacklo_epi8(*this, tmp), _mm_unpackhi_epi8(*this, tmp) };
}
Pack<U16x8, 2> U8x16::MulX(const U8x16& other) const
{
    const auto self16 = Cast<U16x8>(), other16 = other.Cast<U16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}


}
}
