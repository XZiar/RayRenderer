#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"

//#define COMMON_SIMD_LV 200
#if COMMON_SIMD_LV < 20
#   error require at least SSE2
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
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


namespace detail
{

template<typename T, typename E, size_t N>
struct Int128Common : public CommonOperators<T>
{
    static constexpr size_t Count = N;
    static constexpr VecDataInfo VDInfo =
    {
        std::is_unsigned_v<E> ? VecDataInfo::DataTypes::Unsigned : VecDataInfo::DataTypes::Signed,
        static_cast<uint8_t>(128 / N), N, 0
    };
    union
    {
        __m128i Data;
        E Val[N];
    };
    constexpr Int128Common() : Data() { }
    explicit Int128Common(const E* ptr) : Data(_mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr))) { }
    constexpr Int128Common(const __m128i val) : Data(val) { }

    // logic operations
    forceinline T VECCALL And(const T& other) const
    {
        return _mm_and_si128(Data, other);
    }
    forceinline T VECCALL Or(const T& other) const
    {
        return _mm_or_si128(Data, other);
    }
    forceinline T VECCALL Xor(const T& other) const
    {
        return _mm_xor_si128(Data, other);
    }
    forceinline T VECCALL AndNot(const T& other) const
    {
        return _mm_andnot_si128(Data, other);
    }
    forceinline T VECCALL Not() const
    {
        return _mm_xor_si128(Data, _mm_set1_epi8(-1));
    }
    forceinline T VECCALL MoveHiToLo() const { return _mm_srli_si128(Data, 8); }
    forceinline void VECCALL Load(const E *ptr) { Data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)); }
    forceinline void VECCALL Save(E *ptr) const { _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), Data); }
    forceinline constexpr operator const __m128i&() const noexcept { return Data; }
};

}


struct alignas(16) F64x2 : public detail::CommonOperators<F64x2>
{
    static constexpr size_t Count = 2;
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,64,2,0 };
    union
    {
        __m128d Data;
        double Val[2];
    };
    constexpr F64x2() : Data() { }
    explicit F64x2(const double* ptr) : Data(_mm_loadu_pd(ptr)) { }
    constexpr F64x2(const __m128d val) : Data(val) { }
    F64x2(const double val) : Data(_mm_set1_pd(val)) { }
    F64x2(const double lo, const double hi) : Data(_mm_setr_pd(lo, hi)) { }
    forceinline constexpr operator const __m128d&() { return Data; }
    forceinline void VECCALL Load(const double *ptr) { Data = _mm_loadu_pd(ptr); }
    forceinline void VECCALL Save(double *ptr) const { _mm_storeu_pd(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    forceinline F64x2 VECCALL Shuffle() const
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_pd(Data, (Hi << 1) + Lo);
#else
        return _mm_shuffle_pd(Data, Data, (Hi << 1) + Lo);
#endif
    }
    forceinline F64x2 VECCALL Shuffle(const uint8_t Lo, const uint8_t Hi) const
    {
        //static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        switch ((Hi << 1) + Lo)
        {
        case 0: return Shuffle<0, 0>();
        case 1: return Shuffle<1, 0>();
        case 2: return Shuffle<0, 1>();
        case 3: return Shuffle<1, 1>();
        default: return F64x2(); // should not happen
        }
    }

    // logic operations
    forceinline F64x2 VECCALL And(const F64x2& other) const
    {
        return _mm_and_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Or(const F64x2& other) const
    {
        return _mm_or_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Xor(const F64x2& other) const
    {
        return _mm_xor_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL AndNot(const F64x2& other) const
    {
        return _mm_andnot_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Not() const
    {
        return _mm_xor_pd(Data, _mm_castsi128_pd(_mm_set1_epi64x(-1)));
    }

    // arithmetic operations
    forceinline F64x2 VECCALL Add(const F64x2& other) const { return _mm_add_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Sub(const F64x2& other) const { return _mm_sub_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Mul(const F64x2& other) const { return _mm_mul_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Div(const F64x2& other) const { return _mm_div_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Neg() const { return Xor(_mm_castsi128_pd(_mm_set1_epi64x(INT64_MIN))); }
    forceinline F64x2 VECCALL Abs() const { return And(_mm_castsi128_pd(_mm_set1_epi64x(INT64_MAX))); }
    forceinline F64x2 VECCALL Max(const F64x2& other) const { return _mm_max_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Min(const F64x2& other) const { return _mm_min_pd(Data, other.Data); }
    //F64x2 Rcp() const { return _mm_rcp_pd(Data); }
    forceinline F64x2 VECCALL Sqrt() const { return _mm_sqrt_pd(Data); }
    //F64x2 Rsqrt() const { return _mm_rsqrt_pd(Data); }
    forceinline F64x2 VECCALL MulAdd(const F64x2& muler, const F64x2& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_add_pd(_mm_mul_pd(Data, muler.Data), adder.Data);
#endif
    }
    forceinline F64x2 VECCALL MulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_sub_pd(_mm_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    forceinline F64x2 VECCALL NMulAdd(const F64x2& muler, const F64x2& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_sub_pd(adder.Data, _mm_mul_pd(Data, muler.Data));
#endif
    }
    forceinline F64x2 VECCALL NMulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_xor_pd(_mm_add_pd(_mm_mul_pd(Data, muler.Data), suber.Data), _mm_set1_pd(-0.));
#endif
    }
    forceinline F64x2 VECCALL operator*(const F64x2& other) const { return Mul(other); }
    forceinline F64x2 VECCALL operator/(const F64x2& other) const { return Div(other); }

};


struct alignas(16) F32x4 : public detail::CommonOperators<F32x4>
{
    static constexpr size_t Count = 4;
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,32,4,0 };
    union
    {
        __m128 Data;
        float Val[4];
    };
    constexpr F32x4() : Data() { }
    explicit F32x4(const float* ptr) : Data(_mm_loadu_ps(ptr)) { }
    constexpr F32x4(const __m128 val) : Data(val) { }
    F32x4(const float val) : Data(_mm_set1_ps(val)) { }
    F32x4(const float lo0, const float lo1, const float lo2, const float hi3) : Data(_mm_setr_ps(lo0, lo1, lo2, hi3)) { }
    forceinline constexpr operator const __m128&() const noexcept { return Data; }
    forceinline void VECCALL Load(const float *ptr) { Data = _mm_loadu_ps(ptr); }
    forceinline void VECCALL Save(float *ptr) const { _mm_storeu_ps(ptr, Data); }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline F32x4 VECCALL Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_ps(Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#else
        return _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#endif
    }
    forceinline F32x4 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const
    {
        //static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
#if COMMON_SIMD_LV >= 100
        return _mm_permutevar_ps(Data, _mm_setr_epi32(Lo0, Lo1, Lo2, Hi3));
//SSSE3 may be slower due to too many calculations
//#elif COMMON_SIMD_LV >= 31
//        const auto mask = _mm_setr_epi8(Lo0 * 4, Lo0 * 4 + 1, Lo0 * 4 + 2, Lo0 * 4 + 3, Lo1 * 4, Lo1 * 4 + 1, Lo1 * 4 + 2, Lo1 * 4 + 3,
//            Lo2 * 4, Lo2 * 4 + 1, Lo2 * 4 + 2, Lo2 * 4 + 3, Hi3 * 4, Hi3 * 4 + 1, Hi3 * 4 + 2, Hi3 * 4 + 3);
//        return _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(Data), mask));
#else
        return F32x4(Val[Lo0], Val[Lo1], Val[Lo2], Val[Hi3]);
#endif
    }

    // logic operations
    forceinline F32x4 VECCALL And(const F32x4& other) const
    {
        return _mm_and_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL Or(const F32x4& other) const
    {
        return _mm_or_ps(Data, other.Data);
    }
    forceinline F32x4 Xor(const F32x4& other) const
    {
        return _mm_xor_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL AndNot(const F32x4& other) const
    {
        return _mm_andnot_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL Not() const
    {
        return _mm_xor_ps(Data, _mm_castsi128_ps(_mm_set1_epi32(-1)));
    }

    // arithmetic operations
    forceinline F32x4 VECCALL Add(const F32x4& other) const { return _mm_add_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Sub(const F32x4& other) const { return _mm_sub_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Mul(const F32x4& other) const { return _mm_mul_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Div(const F32x4& other) const { return _mm_div_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Neg() const { return Xor(_mm_castsi128_ps(_mm_set1_epi32(INT32_MIN))); }
    forceinline F32x4 VECCALL Abs() const { return And(_mm_castsi128_ps(_mm_set1_epi32(INT32_MAX))); }
    forceinline F32x4 VECCALL Max(const F32x4& other) const { return _mm_max_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Min(const F32x4& other) const { return _mm_min_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Rcp() const { return _mm_rcp_ps(Data); }
    forceinline F32x4 VECCALL Sqrt() const { return _mm_sqrt_ps(Data); }
    forceinline F32x4 VECCALL Rsqrt() const { return _mm_rsqrt_ps(Data); }
    forceinline F32x4 VECCALL MulAdd(const F32x4& muler, const F32x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_add_ps(_mm_mul_ps(Data, muler.Data), adder.Data);
#endif
    }
    forceinline F32x4 VECCALL MulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_sub_ps(_mm_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    forceinline F32x4 VECCALL NMulAdd(const F32x4& muler, const F32x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_sub_ps(adder.Data, _mm_mul_ps(Data, muler.Data));
#endif
    }
    forceinline F32x4 VECCALL NMulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_xor_ps(_mm_add_ps(_mm_mul_ps(Data, muler.Data), suber.Data), _mm_set1_ps(-0.));
#endif
    }
    template<DotPos Mul, DotPos Res>
    forceinline F32x4 VECCALL Dot(const F32x4& other) const
    { 
#if COMMON_SIMD_LV >= 42
        return _mm_dp_ps(Data, other.Data, static_cast<uint8_t>(Mul) << 4 | static_cast<uint8_t>(Res));
#else
        const float sum = Dot<Mul>(other);
        const auto sumVec = _mm_set_ss(sum);
        constexpr uint8_t imm = (HAS_FIELD(Res, DotPos::X) ? 0 : 0b11) + (HAS_FIELD(Res, DotPos::Y) ? 0 : 0b1100) + 
            (HAS_FIELD(Res, DotPos::Z) ? 0 : 0b110000) + (HAS_FIELD(Res, DotPos::W) ? 0 : 0b11000000);
        return _mm_shuffle_ps(sumVec, sumVec, imm);
#endif
    }
    template<DotPos Mul>
    forceinline float VECCALL Dot(const F32x4& other) const
    {
#if COMMON_SIMD_LV >= 42
        return _mm_cvtss_f32(Dot<Mul, DotPos::XYZW>(other).Data);
#else
        const auto prod = this->Mul(other);
        float sum = 0.f;
        if constexpr (HAS_FIELD(Mul, DotPos::X)) sum += prod[0];
        if constexpr (HAS_FIELD(Mul, DotPos::Y)) sum += prod[1];
        if constexpr (HAS_FIELD(Mul, DotPos::Z)) sum += prod[2];
        if constexpr (HAS_FIELD(Mul, DotPos::W)) sum += prod[3];
        return sum;
#endif
    }
    forceinline F32x4 VECCALL operator*(const F32x4& other) const { return Mul(other); }
    forceinline F32x4 VECCALL operator/(const F32x4& other) const { return Div(other); }

};


template<typename T, typename E>
struct alignas(16) I64Common2 : public detail::Int128Common<T, E, 2>
{
private:
    using Int128Common = detail::Int128Common<T, E, 2>;
public:
    using Int128Common::Int128Common;
    I64Common2(const E val) : Int128Common(_mm_set1_epi64x(static_cast<int64_t>(val))) { }
    I64Common2(const E lo, const E hi) :
        Int128Common(_mm_set_epi64x(static_cast<int64_t>(hi), static_cast<int64_t>(lo))) { }

    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        return _mm_shuffle_epi32(this->Data, _MM_SHUFFLE(Hi * 2 + 1, Hi * 2, Lo * 2 + 1, Lo * 2));
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo, const uint8_t Hi) const
    {
        switch ((Hi << 1) + Lo)
        {
        case 0: return Shuffle<0, 0>();
        case 1: return Shuffle<1, 0>();
        case 2: return Shuffle<0, 1>();
        case 3: return Shuffle<1, 1>();
        default: return T(); // should not happen
        }
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi64(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi64(this->Data, other.Data); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi64(this->Data, I64Common2(bits).Data); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi64(this->Data, I64Common2(bits).Data); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi64(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const { return _mm_slli_epi64(this->Data, N); }
#if COMMON_SIMD_LV >= 41
    forceinline T VECCALL MulLo(const T& other) const 
    {
# if COMMON_SIMD_LV >= 320
        return _mm_mullo_epi64(this->Data, other.Data);
# else
        const E loA = _mm_extract_epi64(this->Data, 0), hiA = _mm_extract_epi64(this->Data, 1);
        const E loB = _mm_extract_epi64(other.Data, 0), hiB = _mm_extract_epi64(other.Data, 1);
        return { static_cast<E>(loA * loB), static_cast<E>(hiA * hiB) };
# endif
    }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
#endif
};


struct alignas(16) I64x2 : public I64Common2<I64x2, int64_t>
{
    using I64Common2<I64x2, int64_t>::I64Common2;

    // arithmetic operations
    forceinline I64x2 VECCALL Neg() const { return _mm_sub_epi64(_mm_setzero_si128(), this->Data); }
#if COMMON_SIMD_LV >= 41
    forceinline I64x2 VECCALL Max(const I64x2& other) const
    {
# if COMMON_SIMD_LV >= 320
        return _mm_max_epi64(this->Data, other.Data);
# else
        const auto isGt = _mm_cmpgt_epi64(this->Data, other.Data);
        return _mm_blendv_epi8(other.Data, this->Data, isGt);
# endif
    }
    forceinline I64x2 VECCALL Min(const I64x2& other) const
    {
# if COMMON_SIMD_LV >= 320
        return _mm_min_epi64(this->Data, other.Data);
# else
        const auto isGt = _mm_cmpgt_epi64(this->Data, other.Data);
        return _mm_blendv_epi8(this->Data, other.Data, isGt);
# endif
    }
    forceinline I64x2 VECCALL Abs() const 
    { 
# if COMMON_SIMD_LV >= 320
        return _mm_abs_epi64(this->Data);
# else
        return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(this->Data), _mm_castsi128_pd(Neg()), _mm_castsi128_pd(this->Data)));
# endif
    }
#endif
#if COMMON_SIMD_LV >= 320
    template<uint8_t N>
    forceinline I64x2 VECCALL ShiftRightArth() const { return _mm_srai_epi64(this->Data, N); }
#endif
    template<typename T>
    typename CastTyper<I64x2, T>::Type VECCALL Cast() const;
};

struct alignas(16) U64x2 : public I64Common2<U64x2, uint64_t>
{
    using I64Common2<U64x2, uint64_t>::I64Common2;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    forceinline U64x2 VECCALL SatAdd(const U64x2& other) const
    {
        //// sig: 1|1 return 1, 0|0 return add, 1|0/0|1 use add
        //// add: 1 return add, 0 return 1
        //const auto sig = _mm_castsi128_pd(Xor(other));
        //const auto add = _mm_castsi128_pd(Add(other));
        //const auto allones = _mm_castsi128_pd(_mm_set1_epi8(-1));
        //const auto notAdd = _mm_xor_pd(add, allones);
        //const auto mask = _mm_blendv_pd(_mm_castsi128_pd(other), notAdd, sig);
        //return _mm_castpd_si128(_mm_blendv_pd(add, allones, mask));
        return Min(other.Not()).Add(other);
    }
    forceinline U64x2 VECCALL SatSub(const U64x2& other) const 
    {
# if COMMON_SIMD_LV >= 320
        return Max(other).Sub(other);
# elif COMMON_SIMD_LV >= 42
        const auto signBit = _mm_set1_epi64x(0x8000000000000000LL);
        const auto isGt = _mm_cmpgt_epi64(_mm_xor_si128(this->Data, signBit), _mm_xor_si128(other, signBit));
        const auto sub = Sub(other);
        return _mm_and_si128(sub, isGt);
        //return _mm_blendv_epi8(_mm_setzero_si128(), sub, isGt);
# else
        // sig: 1|0 return sub, 0|1 return 0, 1|1/0|0 use sub
        // sub: 1 return 0, 0 return sub
        const auto sig = _mm_castsi128_pd(Xor(other));
        const auto sub = _mm_castsi128_pd(Sub(other));
        const auto mask = _mm_blendv_pd(sub, _mm_castsi128_pd(other), sig);
        return _mm_castpd_si128(_mm_blendv_pd(sub, _mm_setzero_pd(), mask));
# endif
    }
    forceinline U64x2 VECCALL Max(const U64x2& other) const
    { 
# if COMMON_SIMD_LV >= 320
        return _mm_max_epu64(this->Data, other.Data);
# elif COMMON_SIMD_LV >= 42
        const auto signBit = _mm_set1_epi64x(0x8000000000000000LL);
        const auto isGt = _mm_cmpgt_epi64(_mm_xor_si128(this->Data, signBit), _mm_xor_si128(other, signBit));
        return _mm_blendv_epi8(other, this->Data, isGt);
# else
        // sig: 1|0 choose A, 0|1 choose B, 1|1/0|0 use sub
        // sub: 1 choose B, 0 choose A
        const auto sig = _mm_castsi128_pd(Xor(other));
        const auto sub = _mm_castsi128_pd(Sub(other));
        const auto mask = _mm_blendv_pd(sub, _mm_castsi128_pd(other), sig);
        return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(this->Data), _mm_castsi128_pd(other), mask));
# endif
    }
    forceinline U64x2 VECCALL Min(const U64x2& other) const 
    { 
# if COMMON_SIMD_LV >= 320
        return _mm_min_epu64(this->Data, other.Data);
# elif COMMON_SIMD_LV >= 42
        const auto signBit = _mm_set1_epi64x(0x8000000000000000LL);
        const auto isGt = _mm_cmpgt_epi64(_mm_xor_si128(this->Data, signBit), _mm_xor_si128(other, signBit));
        return _mm_blendv_epi8(this->Data, other, isGt);
# else
        // sig: 1|0 choose B, 0|1 choose A, 1|1/0|0 use sub
        // sub: 1 choose A, 0 choose B
        const auto sig = _mm_castsi128_pd(Xor(other));
        const auto sub = _mm_castsi128_pd(Sub(other));
        const auto mask = _mm_blendv_pd(sub, _mm_castsi128_pd(other), sig);
        return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(other), _mm_castsi128_pd(this->Data), mask));
# endif
    }
#endif
    forceinline U64x2 VECCALL Abs() const { return this->Data; }
    template<uint8_t N>
    forceinline U64x2 VECCALL ShiftRightArth() const { return ShiftRightLogic<N>(); }
    template<typename T>
    typename CastTyper<U64x2, T>::Type VECCALL Cast() const;
};


template<typename T, typename E>
struct alignas(16) I32Common4 : public detail::Int128Common<T, E, 4>
{
private:
    using Int128Common = detail::Int128Common<T, E, 4>;
public:
    using Int128Common::Int128Common;
    I32Common4(const E val) : Int128Common(_mm_set1_epi32(static_cast<int32_t>(val))) { }
    I32Common4(const E lo0, const E lo1, const E lo2, const E hi3) :
        Int128Common(_mm_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(hi3))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm_shuffle_epi32(this->Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const
    {
#if COMMON_SIMD_LV >= 100
        return _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(this->Data), _mm_setr_epi32(Lo0, Lo1, Lo2, Hi3)));
#else
        return T(this->Val[Lo0], this->Val[Lo1], this->Val[Lo2], this->Val[Hi3]);
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi32(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi32(this->Data, other.Data); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi32(this->Data, I64x2(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi32(this->Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi32(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const { return _mm_slli_epi32(this->Data, N); }
#if COMMON_SIMD_LV >= 41
    forceinline T VECCALL MulLo(const T& other) const { return _mm_mullo_epi32(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
#endif
};


struct alignas(16) I32x4 : public I32Common4<I32x4, int32_t>
{
    using I32Common4<I32x4, int32_t>::I32Common4;

    // arithmetic operations
    forceinline I32x4 VECCALL Neg() const { return _mm_sub_epi32(_mm_setzero_si128(), Data); }
#if COMMON_SIMD_LV >= 41
    forceinline I32x4 VECCALL Max(const I32x4& other) const { return _mm_max_epi32(Data, other.Data); }
    forceinline I32x4 VECCALL Min(const I32x4& other) const { return _mm_min_epi32(Data, other.Data); }
    forceinline Pack<I64x2, 2> VECCALL MulX(const I32x4& other) const
    {
        return { I64x2(_mm_mul_epi32(Data, other.Data)), I64x2(_mm_mul_epi32(MoveHiToLo(), other.MoveHiToLo())) };
    }
#endif
#if COMMON_SIMD_LV >= 31
    forceinline I32x4 VECCALL Abs() const { return _mm_abs_epi32(Data); }
#endif
    forceinline I32x4 VECCALL operator>>(const uint8_t bits) const { return _mm_sra_epi32(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline I32x4 VECCALL ShiftRightArth() const { return _mm_srai_epi32(Data, N); }
    template<typename T>
    typename CastTyper<I32x4, T>::Type VECCALL Cast() const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I64x2, 2> VECCALL I32x4::Cast<I64x2>() const
{
    return { _mm_cvtepi32_epi64(Data), _mm_cvtepi32_epi64(MoveHiToLo()) };
}
template<> forceinline Pack<U64x2, 2> VECCALL I32x4::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}
#endif


struct alignas(16) U32x4 : public I32Common4<U32x4, uint32_t>
{
    using I32Common4<U32x4, uint32_t>::I32Common4;

    // arithmetic operations
#if COMMON_SIMD_LV >= 41
    forceinline U32x4 VECCALL SatAdd(const U32x4& other) const
    {
        return Min(other.Not()).Add(other);
    }
    forceinline U32x4 VECCALL SatSub(const U32x4& other) const
    {
        return Max(other).Sub(other);
    }
    forceinline U32x4 VECCALL Max(const U32x4& other) const { return _mm_max_epu32(Data, other.Data); }
    forceinline U32x4 VECCALL Min(const U32x4& other) const { return _mm_min_epu32(Data, other.Data); }
#endif
    forceinline U32x4 VECCALL Abs() const { return Data; }
    forceinline Pack<U64x2, 2> VECCALL MulX(const U32x4& other) const
    {
        return { U64x2(_mm_mul_epu32(Data, other.Data)), U64x2(_mm_mul_epu32(MoveHiToLo(), other.MoveHiToLo())) };
    }
    forceinline U32x4 VECCALL operator>>(const uint8_t bits) const { return ShiftRightLogic(bits); }
    template<uint8_t N>
    forceinline U32x4 VECCALL ShiftRightArth() const { return ShiftRightLogic<N>(); }
    template<typename T>
    typename CastTyper<U32x4, T>::Type VECCALL Cast() const;
};
template<> forceinline Pack<I64x2, 2> VECCALL U32x4::Cast<I64x2>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi32(Data, zero), _mm_unpackhi_epi32(Data, zero) };
}
template<> forceinline Pack<U64x2, 2> VECCALL U32x4::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}


template<typename T, typename E>
struct alignas(16) I16Common8 : public detail::Int128Common<T, E, 8>
{
private:
    using Int128Common = detail::Int128Common<T, E, 8>;
public:
    using Int128Common::Int128Common;
    I16Common8(const E val) : Int128Common(_mm_set1_epi16(val)) { }
    I16Common8(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7) : 
        Int128Common(_mm_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(hi7))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
#if COMMON_SIMD_LV >= 31
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm_shuffle_epi8(this->Data, mask);
#else
        return T(Val[Lo0], Val[Lo1], Val[Lo2], Val[Lo3], Val[Lo4], Val[Lo5], Val[Lo6], Val[Hi7]);
#endif
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const
    {
#if COMMON_SIMD_LV >= 31
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm_shuffle_epi8(this->Data, mask);
#else
        return T(Val[Lo0], Val[Lo1], Val[Lo2], Val[Lo3], Val[Lo4], Val[Lo5], Val[Lo6], Val[Hi7]);
#endif
    }
    
    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi16(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi16(this->Data, other.Data); }
    forceinline T VECCALL MulLo(const T& other) const { return _mm_mullo_epi16(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const { return _mm_sll_epi16(this->Data, I64x2(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi16(this->Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const { return _mm_slli_epi16(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi16(this->Data, N); }
};


struct alignas(16) I16x8 : public I16Common8<I16x8, int16_t>
{
    using I16Common8<I16x8, int16_t>::I16Common8;

    // arithmetic operations
    forceinline I16x8 VECCALL SatAdd(const I16x8& other) const { return _mm_adds_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL SatSub(const I16x8& other) const { return _mm_subs_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL MulHi(const I16x8& other) const { return _mm_mulhi_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL Neg() const { return _mm_sub_epi16(_mm_setzero_si128(), Data); }
    forceinline I16x8 VECCALL Max(const I16x8& other) const { return _mm_max_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL Min(const I16x8& other) const { return _mm_min_epi16(Data, other.Data); }
#if COMMON_SIMD_LV >= 31
    forceinline I16x8 VECCALL Abs() const { return _mm_abs_epi16(Data); }
#endif
    Pack<I32x4, 2> MulX(const I16x8& other) const
    {
        const auto los = MulLo(other), his = MulHi(other);
        return { _mm_unpacklo_epi16(los, his),_mm_unpackhi_epi16(los, his) };
    }
    forceinline I16x8 VECCALL operator>>(const uint8_t bits) const { return _mm_sra_epi16(Data, I64x2(bits)); }
    template<uint8_t N>
    forceinline I16x8 VECCALL ShiftRightArth() const { return _mm_srai_epi16(Data, N); }
    template<typename T>
    typename CastTyper<I16x8, T>::Type VECCALL Cast() const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I32x4, 2> VECCALL I16x8::Cast<I32x4>() const
{
    return { _mm_cvtepi16_epi32(Data), _mm_cvtepi16_epi32(MoveHiToLo()) };
}
template<> forceinline Pack<U32x4, 2> VECCALL I16x8::Cast<U32x4>() const
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL I16x8::Cast<I64x2>() const
{
    const auto ret0 = _mm_cvtepi16_epi64(Data);
    const auto ret1 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U64x2, 4> VECCALL I16x8::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}
#endif


struct alignas(16) U16x8 : public I16Common8<U16x8, uint16_t>
{
    using I16Common8<U16x8, uint16_t>::I16Common8;

    // arithmetic operations
    forceinline U16x8 VECCALL SatAdd(const U16x8& other) const { return _mm_adds_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL SatSub(const U16x8& other) const { return _mm_subs_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL MulHi(const U16x8& other) const { return _mm_mulhi_epu16(Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    forceinline U16x8 VECCALL Max(const U16x8& other) const { return _mm_max_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL Min(const U16x8& other) const { return _mm_min_epu16(Data, other.Data); }
#endif
    forceinline U16x8 VECCALL Abs() const { return Data; }
    Pack<U32x4, 2> MulX(const U16x8& other) const
    {
        const auto los = MulLo(other), his = MulHi(other);
        return { _mm_unpacklo_epi16(los, his),_mm_unpackhi_epi16(los, his) };
    }
    forceinline U16x8 VECCALL operator>>(const uint8_t bits) const { return ShiftRightLogic(bits); }
    template<uint8_t N>
    forceinline U16x8 VECCALL ShiftRightArth() const { return ShiftRightLogic<N>(); }
    template<typename T>
    typename CastTyper<U16x8, T>::Type VECCALL Cast() const;
};
template<> forceinline Pack<I32x4, 2> VECCALL U16x8::Cast<I32x4>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi16(Data, zero), _mm_unpackhi_epi16(Data, zero) };
}
template<> forceinline Pack<U32x4, 2> VECCALL U16x8::Cast<U32x4>() const
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL U16x8::Cast<I64x2>() const
{
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi16(Data, zero), hi = _mm_unpackhi_epi16(Data, zero);
    return { _mm_unpacklo_epi32(lo, zero), _mm_unpackhi_epi32(lo, zero), _mm_unpacklo_epi32(hi, zero), _mm_unpackhi_epi32(hi, zero) };
}
template<> forceinline Pack<U64x2, 4> VECCALL U16x8::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}


template<typename T, typename E>
struct alignas(16) I8Common16 : public detail::Int128Common<T, E, 16>
{
private:
    using Int128Common = detail::Int128Common<T, E, 16>;
public:
    using Int128Common::Int128Common;
    I8Common16(const E val) : Int128Common(_mm_set1_epi8(val)) { }
    I8Common16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15) :
        Int128Common(_mm_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4), static_cast<int8_t>(lo5), static_cast<int8_t>(lo6), static_cast<int8_t>(lo7), static_cast<int8_t>(lo8),
            static_cast<int8_t>(lo9), static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), static_cast<int8_t>(lo12), static_cast<int8_t>(lo13),
            static_cast<int8_t>(lo14), static_cast<int8_t>(hi15))) { }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL Shuffle() const
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
#if COMMON_SIMD_LV >= 31
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm_shuffle_epi8(this->Data, mask);
#else
        return T(Val[Lo0], Val[Lo1], Val[Lo2], Val[Lo3], Val[Lo4], Val[Lo5], Val[Lo6], Val[Lo7], Val[Lo8], Val[Lo9], Val[Lo10], Val[Lo11], Val[Lo12], Val[Lo13], Val[Lo14], Val[Hi15]);
#endif
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Lo7,
        const uint8_t Lo8, const uint8_t Lo9, const uint8_t Lo10, const uint8_t Lo11, const uint8_t Lo12, const uint8_t Lo13, const uint8_t Lo14, const uint8_t Hi15) const
    {
#if COMMON_SIMD_LV >= 31
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm_shuffle_epi8(this->Data, mask);
#else
        return T(Val[Lo0], Val[Lo1], Val[Lo2], Val[Lo3], Val[Lo4], Val[Lo5], Val[Lo6], Val[Lo7], Val[Lo8], Val[Lo9], Val[Lo10], Val[Lo11], Val[Lo12], Val[Lo13], Val[Lo14], Val[Hi15]);
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi8(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi8(this->Data, other.Data); }
};


struct alignas(16) I8x16 : public I8Common16<I8x16, int8_t>
{
    using I8Common16<I8x16, int8_t>::I8Common16;

    // arithmetic operations
    forceinline I8x16 VECCALL SatAdd(const I8x16& other) const { return _mm_adds_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL SatSub(const I8x16& other) const { return _mm_subs_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL Neg() const { return _mm_sub_epi8(_mm_setzero_si128(), Data); }
#if COMMON_SIMD_LV >= 41
    forceinline I8x16 VECCALL Max(const I8x16& other) const { return _mm_max_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL Min(const I8x16& other) const { return _mm_min_epi8(Data, other.Data); }
#endif
#if COMMON_SIMD_LV >= 31
    forceinline I8x16 VECCALL Abs() const { return _mm_abs_epi8(Data); }
#endif
#if COMMON_SIMD_LV >= 41
    forceinline I8x16 VECCALL MulHi(const I8x16& other) const
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightLogic<8>(), hi = full[1].ShiftRightLogic<8>();
        return _mm_packus_epi16(lo, hi);
    }
    forceinline I8x16 VECCALL MulLo(const I8x16& other) const
    { 
        const auto full = MulX(other);
        const I16x8 mask(0x00ff);
        const auto lo = full[0].And(mask), hi = full[1].And(mask);
        return _mm_packus_epi16(lo, hi);
    }
    forceinline I8x16 VECCALL operator*(const I8x16& other) const { return MulLo(other); }
    Pack<I16x8, 2> VECCALL MulX(const I8x16& other) const;
#endif
    template<typename T>
    typename CastTyper<I8x16, T>::Type VECCALL Cast() const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I16x8, 2> VECCALL I8x16::Cast<I16x8>() const
{
    return { _mm_cvtepi8_epi16(Data), _mm_cvtepi8_epi16(MoveHiToLo()) };
}
template<> forceinline Pack<U16x8, 2> VECCALL I8x16::Cast<U16x8>() const
{
    return Cast<I16x8>().Cast<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL I8x16::Cast<I32x4>() const
{
    const auto ret0 = _mm_cvtepi8_epi32(Data);
    const auto ret1 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U32x4, 4> VECCALL I8x16::Cast<U32x4>() const
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL I8x16::Cast<I64x2>() const
{
    const auto ret0 = _mm_cvtepi8_epi64(Data);
    const auto ret1 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 2));
    const auto ret2 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 4));
    const auto ret3 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 6));
    const auto ret4 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 8));
    const auto ret5 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 10));
    const auto ret6 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 12));
    const auto ret7 = _mm_cvtepi8_epi64(_mm_srli_si128(Data, 14));
    return { ret0, ret1, ret2, ret3, ret4, ret5, ret6, ret7 };
}
template<> forceinline Pack<U64x2, 8> VECCALL I8x16::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}
forceinline Pack<I16x8, 2> VECCALL I8x16::MulX(const I8x16& other) const
{
    const auto self16 = Cast<I16x8>(), other16 = other.Cast<I16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif


struct alignas(16) U8x16 : public I8Common16<U8x16, uint8_t>
{
    using I8Common16<U8x16, uint8_t>::I8Common16;

    // arithmetic operations
    forceinline U8x16 VECCALL SatAdd(const U8x16& other) const { return _mm_adds_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL SatSub(const U8x16& other) const { return _mm_subs_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Max(const U8x16& other) const { return _mm_max_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Min(const U8x16& other) const { return _mm_min_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Abs() const { return Data; }
    forceinline U8x16 VECCALL operator*(const U8x16& other) const { return MulLo(other); }
    forceinline U8x16 VECCALL MulHi(const U8x16& other) const
    {
        const auto full = MulX(other);
        return _mm_packus_epi16(full[0].ShiftRightLogic<8>(), full[1].ShiftRightLogic<8>());
    }
    forceinline U8x16 VECCALL MulLo(const U8x16& other) const
    {
        const U16x8 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        const U16x8 mask(0x00ff);
        return U8x16(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    Pack<U16x8, 2> VECCALL MulX(const U8x16& other) const;
    template<typename T>
    typename CastTyper<U8x16, T>::Type VECCALL Cast() const;
};
template<> forceinline Pack<I16x8, 2> VECCALL U8x16::Cast<I16x8>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi8(Data, zero), _mm_unpackhi_epi8(Data, zero) };
}
template<> forceinline Pack<U16x8, 2> VECCALL U8x16::Cast<U16x8>() const
{
    return Cast<I16x8>().Cast<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL U8x16::Cast<I32x4>() const
{
#if COMMON_SIMD_LV >= 41
    /*const auto ret0 = _mm_cvtepu8_epi32(Data);
    const auto ret1 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };*/
#endif
    // not really slower
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi8(Data, zero), hi = _mm_unpackhi_epi8(Data, zero);
    return { _mm_unpacklo_epi8(lo, zero), _mm_unpackhi_epi8(lo, zero), _mm_unpacklo_epi8(hi, zero), _mm_unpackhi_epi8(hi, zero) };
}
template<> forceinline Pack<U32x4, 4> VECCALL U8x16::Cast<U32x4>() const
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL U8x16::Cast<I64x2>() const
{
//#if COMMON_SIMD_LV >= 41
//    const auto ret0 = _mm_cvtepu8_epi64(Data);
//    const auto ret1 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 2));
//    const auto ret2 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 4));
//    const auto ret3 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 6));
//    const auto ret4 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 8));
//    const auto ret5 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 10));
//    const auto ret6 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 12));
//    const auto ret7 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 14));
//    return { ret0, ret1, ret2, ret3, ret4, ret5, ret6, ret7 };
//#else
//#endif
    // not really slower
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi8(Data, zero), hi = _mm_unpackhi_epi8(Data, zero);
    const auto mid0 = _mm_unpacklo_epi16(lo, zero);
    const auto mid1 = _mm_unpackhi_epi16(lo, zero);
    const auto mid2 = _mm_unpacklo_epi16(hi, zero);
    const auto mid3 = _mm_unpackhi_epi16(hi, zero);
    return
    {
        _mm_unpacklo_epi32(mid0, zero), _mm_unpackhi_epi32(mid0, zero), _mm_unpacklo_epi32(mid1, zero), _mm_unpackhi_epi32(mid1, zero),
        _mm_unpacklo_epi32(mid2, zero), _mm_unpackhi_epi32(mid2, zero), _mm_unpacklo_epi32(mid3, zero), _mm_unpackhi_epi32(mid3, zero)
    };
}
template<> forceinline Pack<U64x2, 8> VECCALL U8x16::Cast<U64x2>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}
forceinline Pack<U16x8, 2> VECCALL U8x16::MulX(const U8x16& other) const
{
    const auto self16 = Cast<U16x8>(), other16 = other.Cast<U16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}


template<> forceinline U64x2 VECCALL I64x2::Cast<U64x2>() const
{
    return Data;
}
template<> forceinline I64x2 VECCALL U64x2::Cast<I64x2>() const
{
    return Data;
}
template<> forceinline U32x4 VECCALL I32x4::Cast<U32x4>() const
{
    return Data;
}
template<> forceinline I32x4 VECCALL U32x4::Cast<I32x4>() const
{
    return Data;
}
template<> forceinline U16x8 VECCALL I16x8::Cast<U16x8>() const
{
    return Data;
}
template<> forceinline I16x8 VECCALL U16x8::Cast<I16x8>() const
{
    return Data;
}
template<> forceinline U8x16 VECCALL I8x16::Cast<U8x16>() const
{
    return Data;
}
template<> forceinline I8x16 VECCALL U8x16::Cast<I8x16>() const
{
    return Data;
}


}
}
