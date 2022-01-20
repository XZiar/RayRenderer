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
template<> forceinline __m128  AsType(__m128  from) noexcept { return from; }
template<> forceinline __m128i AsType(__m128  from) noexcept { return _mm_castps_si128(from); }
template<> forceinline __m128d AsType(__m128  from) noexcept { return _mm_castps_pd(from); }
template<> forceinline __m128  AsType(__m128i from) noexcept { return _mm_castsi128_ps(from); }
template<> forceinline __m128i AsType(__m128i from) noexcept { return from; }
template<> forceinline __m128d AsType(__m128i from) noexcept { return _mm_castsi128_pd(from); }
template<> forceinline __m128  AsType(__m128d from) noexcept { return _mm_castpd_ps(from); }
template<> forceinline __m128i AsType(__m128d from) noexcept { return _mm_castpd_si128(from); }
template<> forceinline __m128d AsType(__m128d from) noexcept { return from; }


inline constexpr int CmpTypeImm(CompareType cmp) noexcept
{
    switch (cmp)
    {
    case CompareType::LessThan:     return _CMP_LT_OQ;
    case CompareType::LessEqual:    return _CMP_LE_OQ;
    case CompareType::Equal:        return _CMP_EQ_OQ;
    case CompareType::NotEqual:     return _CMP_NEQ_OQ;
    case CompareType::GreaterEqual: return _CMP_GE_OQ;
    case CompareType::GreaterThan:  return _CMP_GT_OQ;
    default:                        return _CMP_FALSE_OQ;
    }
}

inline constexpr int RoundModeImm(RoundMode mode) noexcept
{
    switch (mode)
    {
    case RoundMode::ToEven:     return _MM_FROUND_TO_NEAREST_INT;
    case RoundMode::ToZero:     return _MM_FROUND_TO_ZERO;
    case RoundMode::ToPosInf:   return _MM_FROUND_TO_POS_INF;
    case RoundMode::ToNegInf:   return _MM_FROUND_TO_NEG_INF; 
    default:                    return _MM_FROUND_CUR_DIRECTION;
    }
}


template<typename T, typename E, size_t N>
struct SSE128Common : public CommonOperators<T>
{
    using EleType = E;
    using VecType = __m128i;
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
    constexpr SSE128Common() : Data() { }
    explicit SSE128Common(const E* ptr) : Data(_mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr))) { }
    constexpr SSE128Common(const __m128i val) : Data(val) { }
    forceinline void VECCALL Load(const E* ptr) { Data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)); }
    forceinline void VECCALL Save(E* ptr) const { _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), Data); }
    forceinline constexpr operator const __m128i& () const noexcept { return Data; }

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

    forceinline static T AllZero() noexcept { return _mm_setzero_si128(); }
};


template<typename T, typename E>
struct alignas(16) Common64x2 : public SSE128Common<T, E, 2>
{
private:
    using Base = SSE128Common<T, E, 2>;
public:
    using Base::Base;
    Common64x2(const E val) : Base(_mm_set1_epi64x(static_cast<int64_t>(val))) { }
    Common64x2(const E lo, const E hi) :
        Base(_mm_set_epi64x(static_cast<int64_t>(hi), static_cast<int64_t>(lo))) { }

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
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return Shuffle<Idx, Idx>();
    }
#if COMMON_SIMD_LV >= 31
    forceinline T VECCALL SwapEndian() const
    {
        const auto SwapMask = _mm_set_epi64x(0x08090a0b0c0d0e0fULL, 0x0001020304050607ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
#endif
    forceinline T VECCALL ZipLo(const T& other) const { return _mm_unpacklo_epi64(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const { return _mm_unpackhi_epi64(this->Data, other.Data); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const
    {
#if COMMON_SIMD_LV >= 41
        if constexpr (Msk != MaskType::FullEle)
            return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(this->Data), _mm_castsi128_pd(other.Data), _mm_castsi128_pd(mask))); // less inst with higher latency
//# if COMMON_SIMD_LV >= 320
//        mask = _mm_srai_epi64(mask, 64);
//# else
//        mask = _mm_srai_epi32(_mm_shuffle_epi32(mask, 0xf5), 32);
//# endif
        else
            return _mm_blendv_epi8(this->Data, other.Data, mask);
#else
        if constexpr (Msk != MaskType::FullEle)
            mask = _mm_srai_epi32(_mm_shuffle_epi32(mask, 0xf5), 32); // make sure all bits are covered
        return mask.AndNot(this->Data).Or(mask.And(other.Data));
#endif
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const
    {
        static_assert(Mask <= 0b11, "Only allow 2 bits");
#if COMMON_SIMD_LV >= 200
        return _mm_blend_epi32(this->Data, other.Data, (Mask & 0b1 ? 0b11 : 0b00) | (Mask & 0b10 ? 0b1100 : 0b0000));
#elif COMMON_SIMD_LV >= 41
        return _mm_blend_epi16(this->Data, other.Data, (Mask & 0b1 ? 0xf : 0x0) | (Mask & 0b10 ? 0xf0 : 0x00));
#else
        if constexpr (Mask == 0b00)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0b11)
            return other;
        else if constexpr (Mask == 0b01)
            return _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(other.Data), _mm_castsi128_pd(this->Data), 0b10));
        else if constexpr (Mask == 0b10)
            return _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(this->Data), _mm_castsi128_pd(other.Data), 0b10));
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi64(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi64(this->Data, other.Data); }
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
    forceinline T& VECCALL operator*=(const T& other) { this->Data = MulLo(other); return *static_cast<T*>(this); }
#endif
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const { return _mm_sll_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
        else
#if COMMON_SIMD_LV >= 320
            return _mm_sra_epi64(this->Data, _mm_cvtsi32_si128(bits));
#elif COMMON_SIMD_LV >= 41
        {
            if (bits == 0) return this->Data;
            const auto sign32H = _mm_srai_epi32(this->Data, 32); // only high part useful
            const auto signAll = _mm_shuffle_epi32(sign32H, 0b11110101);
            if (bits > 63) return signAll;
            const auto zero64 = _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
            const auto sign64 = _mm_sll_epi64(signAll, _mm_cvtsi32_si128(64 - bits));
            return _mm_or_si128(zero64, sign64);
        }
#endif
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const { return ShiftLeftLogic(bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const { return _mm_slli_epi64(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi64(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const 
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srli_epi64(this->Data, N);
#if COMMON_SIMD_LV >= 320
        else
            return _mm_srai_epi64(this->Data, N);
#elif COMMON_SIMD_LV >= 41
        else if constexpr (N == 0)
        {
            return this->Data;
        }
        else
        {
            const auto sign32H = _mm_srai_epi32(this->Data, 32); // only high part useful
            const auto signAll = _mm_shuffle_epi32(sign32H, 0b11110101);
            if constexpr (N > 63)
                return signAll;
            else
            {
                const auto zero64 = _mm_srli_epi64(this->Data, N);
                const auto sign64 = _mm_slli_epi64(signAll, 64 - N);
                return _mm_or_si128(zero64, sign64);
            }
        }
#endif
    }

    forceinline static T LoadLo(const E val) noexcept { return _mm_loadu_si64(&val); }
    forceinline static T LoadLo(const E* ptr) noexcept { return _mm_loadu_si64(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common32x4 : public SSE128Common<T, E, 4>
{
private:
    using Base = SSE128Common<T, E, 4>;
public:
    using Base::Base;
    Common32x4(const E val) : Base(_mm_set1_epi32(static_cast<int32_t>(val))) { }
    Common32x4(const E lo0, const E lo1, const E lo2, const E hi3) :
        Base(_mm_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(hi3))) { }

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
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return Shuffle<Idx, Idx, Idx, Idx>();
    }
#if COMMON_SIMD_LV >= 31
    forceinline T VECCALL SwapEndian() const
    {
        const auto SwapMask = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
#endif
    forceinline T VECCALL ZipLo(const T& other) const { return _mm_unpacklo_epi32(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const { return _mm_unpackhi_epi32(this->Data, other.Data); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const
    {
#if COMMON_SIMD_LV >= 41
        if constexpr (Msk != MaskType::FullEle)
            return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(this->Data), _mm_castsi128_ps(other.Data), _mm_castsi128_ps(mask))); // less inst with higher latency
            //mask = _mm_srai_epi32(mask, 32); // make sure all bits are covered
        else
            return _mm_blendv_epi8(this->Data, other.Data, mask);
#else
        if constexpr (Msk != MaskType::FullEle)
            mask = _mm_srai_epi32(mask, 32); // make sure all bits are covered
        return mask.AndNot(this->Data).Or(mask.And(other.Data));
#endif
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
#if COMMON_SIMD_LV >= 200
        return _mm_blend_epi32(this->Data, other.Data, Mask);
#elif COMMON_SIMD_LV >= 41
        return _mm_blend_epi16(this->Data, other.Data, (Mask & 0b1 ? 0b11 : 0) |
            (Mask & 0b10 ? 0b1100 : 0) | (Mask & 0b100 ? 0b110000 : 0) | (Mask & 0b1000 ? 0b11000000 : 0));
#else
        if constexpr (Mask == 0b0000)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0b1111)
            return other;
        else if constexpr (Mask == 0b0011)
            return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(other.Data), _mm_castsi128_ps(this->Data), 0b11100100));
        else if constexpr (Mask == 0b1100)
            return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(this->Data), _mm_castsi128_ps(other.Data), 0b11100100));
        else
            return SelectWith<MaskType::FullEle>(other, _mm_setr_epi32(Mask & 0b1 ? -1 : 0, Mask & 0b10 ? -1 : 0, Mask & 0b100 ? -1 : 0, Mask & 0b1000 ? -1 : 0));
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi32(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi32(this->Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    forceinline T VECCALL MulLo(const T& other) const { return _mm_mullo_epi32(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) { this->Data = MulLo(other); return *static_cast<T*>(this); }
#endif
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const { return _mm_sll_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const 
    { 
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi32(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm_sra_epi32(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const { return _mm_slli_epi32(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi32(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const 
    { 
        if constexpr (std::is_unsigned_v<E>) 
            return _mm_srli_epi32(this->Data, N);
        else 
            return _mm_srai_epi32(this->Data, N);
    }

    forceinline static T LoadLo(const E val) noexcept { return _mm_loadu_si32(&val); }
    forceinline static T LoadLo(const E* ptr) noexcept { return _mm_loadu_si32(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common16x8 : public SSE128Common<T, E, 8>
{
private:
    using Base = SSE128Common<T, E, 8>;
public:
    using Base::Base;
    Common16x8(const E val) : Base(_mm_set1_epi16(val)) { }
    Common16x8(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7) : 
        Base(_mm_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(hi7))) { }

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
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const
    {
        static_assert(Idx < 8, "shuffle index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
        auto shifted = this->Data;
        if constexpr (Idx > 0)
            shifted = _mm_srli_si128(this->Data, Idx * 2);
        return _mm_broadcastw_epi16(shifted);
#else
        constexpr auto NewIdx = Idx % 4;
        constexpr auto Idx4 = (NewIdx << 6) + (NewIdx << 4) + (NewIdx << 2) + NewIdx;
        if constexpr (Idx < 4) // lo half
        {
            const auto lo = _mm_shufflelo_epi16(this->Data, Idx4);
            return _mm_unpacklo_epi64(lo, lo);
        }
        else // hi half
        {
            const auto hi = _mm_shufflehi_epi16(this->Data, Idx4);
            return _mm_unpackhi_epi64(hi, hi);
        }
#endif
    }
#if COMMON_SIMD_LV >= 31
    forceinline T VECCALL SwapEndian() const
    {
        const auto SwapMask = _mm_set_epi64x(0x0e0f0c0d0a0b0809ULL, 0x0607040502030001ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
#endif
    forceinline T VECCALL ZipLo(const T& other) const { return _mm_unpacklo_epi16(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const { return _mm_unpackhi_epi16(this->Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const
    {
        if constexpr(Msk != MaskType::FullEle)
            mask = _mm_srai_epi16(mask, 16); // make sure all bits are covered
        return _mm_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const
    {
        return _mm_blend_epi16(this->Data, other.Data, Mask);
    }
#endif

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi16(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi16(this->Data, other.Data); }
    forceinline T VECCALL MulLo(const T& other) const { return _mm_mullo_epi16(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) { this->Data = MulLo(other); return *static_cast<T*>(this); }
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const { return _mm_sll_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const { return _mm_srl_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi16(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm_sra_epi16(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const { return _mm_slli_epi16(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const { return _mm_srli_epi16(this->Data, N); }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srli_epi16(this->Data, N);
        else
            return _mm_srai_epi16(this->Data, N);
    }

    forceinline static T LoadLo(const E val) noexcept { return _mm_loadu_si16(&val); }
    forceinline static T LoadLo(const E* ptr) noexcept { return _mm_loadu_si16(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common8x16 : public SSE128Common<T, E, 16>
{
private:
    using Base = SSE128Common<T, E, 16>;
public:
    using Base::Base;
    Common8x16(const E val) : Base(_mm_set1_epi8(val)) { }
    Common8x16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15) :
        Base(_mm_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
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
#if COMMON_SIMD_LV >= 31
    forceinline T VECCALL Shuffle(const U8x16& pos) const;
#endif
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Lo7,
        const uint8_t Lo8, const uint8_t Lo9, const uint8_t Lo10, const uint8_t Lo11, const uint8_t Lo12, const uint8_t Lo13, const uint8_t Lo14, const uint8_t Hi15) const
    {
#if COMMON_SIMD_LV >= 31
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return Shuffle(mask);
#else
        return T(Val[Lo0], Val[Lo1], Val[Lo2], Val[Lo3], Val[Lo4], Val[Lo5], Val[Lo6], Val[Lo7], Val[Lo8], Val[Lo9], Val[Lo10], Val[Lo11], Val[Lo12], Val[Lo13], Val[Lo14], Val[Hi15]);
#endif
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const
    {
        static_assert(Idx < 16, "shuffle index should be in [0,15]");
#if COMMON_SIMD_LV >= 200
        auto shifted = this->Data;
        if constexpr (Idx > 0)
            shifted = _mm_srli_si128(this->Data, Idx);
        return _mm_broadcastb_epi8(shifted);
#else
        constexpr auto PairIdx = Idx % 8;
        __m128i paired;
        if constexpr (Idx < 8) // lo half
            paired = _mm_unpacklo_epi8(this->Data, this->Data);
        else // hi half
            paired = _mm_unpackhi_epi8(this->Data, this->Data);
        constexpr auto NewIdx = PairIdx % 4;
        constexpr auto Idx4 = (NewIdx << 6) + (NewIdx << 4) + (NewIdx << 2) + NewIdx;
        if constexpr (PairIdx < 4) // lo half
        {
            const auto lo = _mm_shufflelo_epi16(paired, Idx4);
            return _mm_unpacklo_epi64(lo, lo);
        }
        else // hi half
        {
            const auto hi = _mm_shufflehi_epi16(paired, Idx4);
            return _mm_unpackhi_epi64(hi, hi);
        }
#endif
    }
    forceinline T VECCALL ZipLo(const T& other) const { return _mm_unpacklo_epi8(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const { return _mm_unpackhi_epi8(this->Data, other.Data); }
#if COMMON_SIMD_LV >= 41
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const { return _mm_blendv_epi8(this->Data, other.Data, mask); }
    template<uint16_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const
    {
        if constexpr (Mask == 0)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0xffff)
            return other;
        else
        {
#   ifdef CMSIMD_LESS_SPACE
            const auto mask = _mm_insert_epi64(_mm_loadu_si64(&FullMask64[Mask & 0xff]), static_cast<int64_t>(FullMask64[(Mask >> 8) & 0xff]), 1);
            return _mm_blendv_epi8(this->Data, other.Data, mask);
#   else
            constexpr uint64_t mask[2] = { FullMask64[Mask & 0xff], FullMask64[(Mask >> 8) & 0xff] };
            return _mm_blendv_epi8(this->Data, other.Data, _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask)));
#   endif
        }
    }
#endif

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const { return _mm_add_epi8(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const { return _mm_sub_epi8(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const { return static_cast<T*>(this)->MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) { this->Data = operator*(other); return *static_cast<T*>(this); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const
    {
        if (bits >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff << bits));
            const auto shift16 = _mm_sll_epi16(this->Data, _mm_cvtsi32_si128(bits));
            return _mm_and_si128(shift16, mask);
        }
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const 
    { 
        if (bits >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff >> bits));
            const auto shift16 = _mm_srl_epi16(this->Data, _mm_cvtsi32_si128(bits));
            return _mm_and_si128(shift16, mask);
        }
    }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const;
    forceinline T VECCALL operator<<(const uint8_t bits) const { return ShiftLeftLogic(bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const 
    {
        if constexpr (N >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff << N));
            const auto shift16 = _mm_slli_epi16(this->Data, N);
            return _mm_and_si128(shift16, mask);
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const
    {
        if constexpr (N >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff >> N));
            const auto shift16 = _mm_srli_epi16(this->Data, N);
            return _mm_and_si128(shift16, mask);
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const;
};

}


struct alignas(16) F64x2 : public detail::CommonOperators<F64x2>
{
    using EleType = double;
    using VecType = __m128d;
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
    forceinline constexpr operator const __m128d&() const noexcept { return Data; }
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
    template<uint8_t Idx>
    forceinline F64x2 VECCALL Broadcast() const
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return Shuffle<Idx, Idx>();
    }
    forceinline F64x2 VECCALL ZipLo(const F64x2& other) const { return _mm_unpacklo_pd(Data, other); }
    forceinline F64x2 VECCALL ZipHi(const F64x2& other) const { return _mm_unpackhi_pd(Data, other); }
#if COMMON_SIMD_LV >= 41
    template<MaskType Msk>
    forceinline F64x2 VECCALL SelectWith(const F64x2& other, F64x2 mask) const
    {
        return _mm_blendv_pd(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F64x2 VECCALL SelectWith(const F64x2& other) const
    {
        static_assert(Mask <= 0b11, "Only allow 2 bits");
        return _mm_blend_pd(this->Data, other.Data, Mask);
    }
#endif

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F64x2 VECCALL Compare(const F64x2 other) const
    {
#if COMMON_SIMD_LV >= 100
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm_cmp_pd(Data, other, cmpImm);
#else
             if constexpr (Cmp == CompareType::LessThan)     return _mm_cmplt_pd (Data, other);
        else if constexpr (Cmp == CompareType::LessEqual)    return _mm_cmple_pd (Data, other);
        else if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_pd (Data, other);
        else if constexpr (Cmp == CompareType::NotEqual)     return _mm_cmpneq_pd(Data, other);
        else if constexpr (Cmp == CompareType::GreaterEqual) return _mm_cmpge_pd (Data, other);
        else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_pd (Data, other);
        else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
#endif
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
    template<size_t Idx>
    forceinline F64x2 VECCALL MulAdd(const F64x2& muler, const F64x2& adder) const
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x2 VECCALL MulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_sub_pd(_mm_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL MulSub(const F64x2& muler, const F64x2& suber) const
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F64x2 VECCALL NMulAdd(const F64x2& muler, const F64x2& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_sub_pd(adder.Data, _mm_mul_pd(Data, muler.Data));
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulAdd(const F64x2& muler, const F64x2& adder) const
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x2 VECCALL NMulSub(const F64x2& muler, const F64x2& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_xor_pd(_mm_add_pd(_mm_mul_pd(Data, muler.Data), suber.Data), _mm_set1_pd(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulSub(const F64x2& muler, const F64x2& suber) const
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    forceinline F64x2 VECCALL operator*(const F64x2& other) const { return Mul(other); }
    forceinline F64x2 VECCALL operator/(const F64x2& other) const { return Div(other); }
    forceinline F64x2& VECCALL operator*=(const F64x2& other) { Data = Mul(other); return *this; }
    forceinline F64x2& VECCALL operator/=(const F64x2& other) { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = detail::CstMode<F64x2, T>(), typename... Args>
    typename CastTyper<F64x2, T>::Type VECCALL Cast(const Args&... args) const;
#if COMMON_SIMD_LV >= 41
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F64x2 VECCALL Round() const
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm_round_pd(Data, imm8);
    }
#endif

    forceinline static F64x2 AllZero() noexcept { return _mm_setzero_pd(); }
    forceinline static F64x2 LoadLo(const double val) noexcept { return _mm_load_sd(&val); }
    forceinline static F64x2 LoadLo(const double* ptr) noexcept { return _mm_load_sd(ptr); }
};


struct alignas(16) F32x4 : public detail::CommonOperators<F32x4>
{
    using EleType = float;
    using VecType = __m128;
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
    template<uint8_t Idx>
    forceinline F32x4 VECCALL Broadcast() const
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return Shuffle<Idx, Idx, Idx, Idx>();
    }
    forceinline F32x4 VECCALL ZipLo(const F32x4& other) const { return _mm_unpacklo_ps(Data, other); }
    forceinline F32x4 VECCALL ZipHi(const F32x4& other) const { return _mm_unpackhi_ps(Data, other); }
#if COMMON_SIMD_LV >= 41
    template<MaskType Msk>
    forceinline F32x4 VECCALL SelectWith(const F32x4& other, F32x4 mask) const
    {
        return _mm_blendv_ps(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F32x4 VECCALL SelectWith(const F32x4& other) const
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
        return _mm_blend_ps(this->Data, other.Data, Mask);
    }
#endif

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F32x4 VECCALL Compare(const F32x4 other) const
    {
#if COMMON_SIMD_LV >= 100
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm_cmp_ps(Data, other, cmpImm);
#else
             if constexpr (Cmp == CompareType::LessThan)     return _mm_cmplt_ps (Data, other);
        else if constexpr (Cmp == CompareType::LessEqual)    return _mm_cmple_ps (Data, other);
        else if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_ps (Data, other);
        else if constexpr (Cmp == CompareType::NotEqual)     return _mm_cmpneq_ps(Data, other);
        else if constexpr (Cmp == CompareType::GreaterEqual) return _mm_cmpge_ps (Data, other);
        else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_ps (Data, other);
        else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
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
    template<size_t Idx>
    forceinline F32x4 VECCALL MulAdd(const F32x4& muler, const F32x4& adder) const
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x4 VECCALL MulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_sub_ps(_mm_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL MulSub(const F32x4& muler, const F32x4& suber) const
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F32x4 VECCALL NMulAdd(const F32x4& muler, const F32x4& adder) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_sub_ps(adder.Data, _mm_mul_ps(Data, muler.Data));
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulAdd(const F32x4& muler, const F32x4& adder) const
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x4 VECCALL NMulSub(const F32x4& muler, const F32x4& suber) const
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_xor_ps(_mm_add_ps(_mm_mul_ps(Data, muler.Data), suber.Data), _mm_set1_ps(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulSub(const F32x4& muler, const F32x4& suber) const
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    template<DotPos Mul, DotPos Res>
    forceinline F32x4 VECCALL Dot(const F32x4& other) const
    { 
#if COMMON_SIMD_LV >= 41
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
#if COMMON_SIMD_LV >= 41
        return _mm_cvtss_f32(Dot<Mul, DotPos::XYZW>(other).Data);
#else
        const auto prod = Mul(other);
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
    forceinline F32x4& VECCALL operator*=(const F32x4& other) { Data = Mul(other); return *this; }
    forceinline F32x4& VECCALL operator/=(const F32x4& other) { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = detail::CstMode<F32x4, T>(), typename... Args>
    typename CastTyper<F32x4, T>::Type VECCALL Cast(const Args&... args) const;
#if COMMON_SIMD_LV >= 41
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F32x4 VECCALL Round() const
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm_round_ps(Data, imm8);
    }
#endif

    forceinline static F32x4 AllZero() noexcept { return _mm_setzero_ps(); }
    forceinline static F32x4 LoadLo(const float val) noexcept { return _mm_load_ss(&val); }
    forceinline static F32x4 LoadLo(const float* ptr) noexcept { return _mm_load_ss(ptr); }
};


struct alignas(16) I64x2 : public detail::Common64x2<I64x2, int64_t>
{
    using Common64x2<I64x2, int64_t>::Common64x2;

    // compare operations
#if COMMON_SIMD_LV >= 41
    template<CompareType Cmp, MaskType Msk>
    forceinline I64x2 VECCALL Compare(const I64x2 other) const
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi64(Data, other);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi64(Data, other);
            else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }
#endif

    // arithmetic operations
    forceinline I64x2 VECCALL Neg() const { return _mm_sub_epi64(_mm_setzero_si128(), Data); }
    forceinline I64x2 VECCALL SatAdd(const I64x2& other) const
    {
        const auto added = Add(other);
#if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,1,0,0,0,0,1,0 -> 0x42
        const auto overflow = _mm_ternarylogic_epi64(this->Data, other.Data, added, 0x42);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm_mask_add_epi64(added, _mm_movepi64_mask(overflow), ShiftRightLogic<63>(), I64x2(INT64_MAX));
#else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.AndNot(Xor(added)); // !diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<63>().Add(INT64_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
#endif
    }
    forceinline I64x2 VECCALL SatSub(const I64x2& other) const
    {
        const auto added = Sub(other);
#if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,0,0,1,1,0,0,0 -> 0x18
        const auto overflow = _mm_ternarylogic_epi64(this->Data, other.Data, added, 0x18);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm_mask_add_epi64(added, _mm_movepi64_mask(overflow), ShiftRightLogic<63>(), I64x2(INT64_MAX));
#else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.And(Xor(added)); // diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<63>().Add(INT64_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
#endif
    }
#if COMMON_SIMD_LV >= 41
    forceinline I64x2 VECCALL Max(const I64x2& other) const
    {
# if COMMON_SIMD_LV >= 320
        return _mm_max_epi64(Data, other.Data);
# else
        return _mm_blendv_epi8(other.Data, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline I64x2 VECCALL Min(const I64x2& other) const
    {
# if COMMON_SIMD_LV >= 320
        return _mm_min_epi64(Data, other.Data);
# else
        return _mm_blendv_epi8(Data, other.Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
#endif
    forceinline I64x2 VECCALL Abs() const
    {
#if COMMON_SIMD_LV >= 320
        return _mm_abs_epi64(Data);
#else
        return SelectWith<MaskType::SigBit>(Neg(), *this);
#endif
    }
    template<typename T, CastMode Mode = detail::CstMode<I64x2, T>(), typename... Args>
    typename CastTyper<I64x2, T>::Type VECCALL Cast(const Args&... args) const;
};


struct alignas(16) U64x2 : public detail::Common64x2<U64x2, uint64_t>
{
    using Common64x2<U64x2, uint64_t>::Common64x2;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U64x2 VECCALL Compare(const U64x2 other) const
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I64x2>().Compare<Cmp, Msk>(other.As<I64x2>()).template As<U64x2>();
        }
        else
        {
            const U64x2 sigMask(static_cast<uint64_t>(0x8000000000000000));
            return Xor(sigMask).As<I64x2>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I64x2>()).template As<U64x2>();
        }
    }

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
# elif COMMON_SIMD_LV >= 41
        return Sub(other).And(Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
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
        return _mm_max_epu64(Data, other.Data);
# elif COMMON_SIMD_LV >= 41
        return _mm_blendv_epi8(other, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# else
        // sig: 1|0 choose A, 0|1 choose B, 1|1/0|0 use sub
        // sub: 1 choose B, 0 choose A
        const auto sig = _mm_castsi128_pd(Xor(other));
        const auto sub = _mm_castsi128_pd(Sub(other));
        const auto mask = _mm_blendv_pd(sub, _mm_castsi128_pd(other), sig);
        return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(Data), _mm_castsi128_pd(other), mask));
# endif
    }
    forceinline U64x2 VECCALL Min(const U64x2& other) const 
    { 
# if COMMON_SIMD_LV >= 320
        return _mm_min_epu64(Data, other.Data);
# elif COMMON_SIMD_LV >= 41
        return _mm_blendv_epi8(Data, other, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# else
        // sig: 1|0 choose B, 0|1 choose A, 1|1/0|0 use sub
        // sub: 1 choose A, 0 choose B
        const auto sig = _mm_castsi128_pd(Xor(other));
        const auto sub = _mm_castsi128_pd(Sub(other));
        const auto mask = _mm_blendv_pd(sub, _mm_castsi128_pd(other), sig);
        return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(other), _mm_castsi128_pd(Data), mask));
# endif
    }
#endif
    forceinline U64x2 VECCALL Abs() const { return Data; }
    template<typename T, CastMode Mode = detail::CstMode<U64x2, T>(), typename... Args>
    typename CastTyper<U64x2, T>::Type VECCALL Cast(const Args&... args) const;
};


struct alignas(16) I32x4 : public detail::Common32x4<I32x4, int32_t>
{
    using Common32x4<I32x4, int32_t>::Common32x4;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I32x4 VECCALL Compare(const I32x4 other) const
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi32(Data, other);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi32(Data, other);
            else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I32x4 VECCALL SatAdd(const I32x4& other) const 
    {
        const auto added    = Add(other);
#if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,1,0,0,0,0,1,0 -> 0x42
        const auto overflow = _mm_ternarylogic_epi32(this->Data, other.Data, added, 0x42);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm_mask_add_epi32(added, _mm_movepi32_mask(overflow), ShiftRightLogic<31>(), I32x4(INT32_MAX));
#else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.AndNot(Xor(added)); // !diffFlag && (this != added)
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        const auto satVal   = ShiftRightLogic<31>().Add(INT32_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
#endif
    }
    forceinline I32x4 VECCALL SatSub(const I32x4& other) const
    {
        const auto added = Sub(other);
#if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this != other && this != added -> 0,0,0,1,1,0,0,0 -> 0x18
        const auto overflow = _mm_ternarylogic_epi32(this->Data, other.Data, added, 0x18);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm_mask_add_epi32(added, _mm_movepi32_mask(overflow), ShiftRightLogic<31>(), I32x4(INT32_MAX));
#else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.And(Xor(added)); // diffFlag && (this != added)
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<31>().Add(INT32_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
#endif
    }
    forceinline I32x4 VECCALL Neg() const { return _mm_sub_epi32(_mm_setzero_si128(), Data); }
#if COMMON_SIMD_LV >= 41
    forceinline I32x4 VECCALL Max(const I32x4& other) const { return _mm_max_epi32(Data, other.Data); }
    forceinline I32x4 VECCALL Min(const I32x4& other) const { return _mm_min_epi32(Data, other.Data); }
    forceinline Pack<I64x2, 2> VECCALL MulX(const I32x4& other) const
    {
        const I64x2 even = _mm_mul_epi32(Data, other.Data);
        const I64x2 odd  = _mm_mul_epi32(As<I64x2>().ShiftRightLogic<32>(), other.As<I64x2>().ShiftRightLogic<32>());
        return { even.ZipLo(odd), even.ZipHi(odd) };
    }
#endif
#if COMMON_SIMD_LV >= 31
    forceinline I32x4 VECCALL Abs() const { return _mm_abs_epi32(Data); }
#endif
    template<typename T, CastMode Mode = detail::CstMode<I32x4, T>(), typename... Args>
    typename CastTyper<I32x4, T>::Type VECCALL Cast(const Args&... args) const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I64x2, 2> VECCALL I32x4::Cast<I64x2, CastMode::RangeUndef>() const
{
    return { _mm_cvtepi32_epi64(Data), _mm_cvtepi32_epi64(MoveHiToLo()) };
}
template<> forceinline Pack<U64x2, 2> VECCALL I32x4::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().As<U64x2>();
}
#endif
template<> forceinline F32x4 VECCALL I32x4::Cast<F32x4, CastMode::RangeUndef>() const
{
    return _mm_cvtepi32_ps(Data);
}
template<> forceinline Pack<F64x2, 2> VECCALL I32x4::Cast<F64x2, CastMode::RangeUndef>() const
{
    return { _mm_cvtepi32_pd(Data), _mm_cvtepi32_pd(MoveHiToLo()) };
}


struct alignas(16) U32x4 : public detail::Common32x4<U32x4, uint32_t>
{
    using Common32x4<U32x4, uint32_t>::Common32x4;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U32x4 VECCALL Compare(const U32x4 other) const
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I32x4>().Compare<Cmp, Msk>(other.As<I32x4>()).template As<U32x4>();
        }
        else
        {
            const U32x4 sigMask(static_cast<uint32_t>(0x80000000));
            return Xor(sigMask).As<I32x4>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I32x4>()).template As<U32x4>();
        }
    }

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
        const U64x2 even = _mm_mul_epu32(Data, other.Data);
        const U64x2 odd  = _mm_mul_epu32(As<U64x2>().ShiftRightLogic<32>(), other.As<U64x2>().ShiftRightLogic<32>());
        return { even.ZipLo(odd), even.ZipHi(odd) };
    }
    template<typename T, CastMode Mode = detail::CstMode<U32x4, T>(), typename... Args>
    typename CastTyper<U32x4, T>::Type VECCALL Cast(const Args&... args) const;
};
template<> forceinline Pack<I64x2, 2> VECCALL U32x4::Cast<I64x2, CastMode::RangeUndef>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi32(Data, zero), _mm_unpackhi_epi32(Data, zero) };
}
template<> forceinline Pack<U64x2, 2> VECCALL U32x4::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline F32x4 VECCALL U32x4::Cast<F32x4, CastMode::RangeUndef>() const
{
    const auto mul16 = _mm_set1_ps(65536.f);
    const auto lo16  = And(0xffff);
    const auto hi16  = ShiftRightLogic<16>();
    const auto base  = hi16.As<I32x4>().Cast<F32x4>();
    const auto addlo = lo16.As<I32x4>().Cast<F32x4>();
    return base.MulAdd(mul16, addlo);
}
template<> forceinline Pack<F64x2, 2> VECCALL U32x4::Cast<F64x2, CastMode::RangeUndef>() const
{
#if COMMON_SIMD_LV >= 320
    return { _mm_cvtepu32_pd(Data), _mm_cvtepu32_pd(MoveHiToLo()) };
#else
    /*const auto mul16 = _mm_set1_pd(65536.f);
    const auto lo16  = And(0xffff);
    const auto hi16  = ShiftRightLogic<16>();
    const auto base  = hi16.As<I32x4>().Cast<F64x2>();
    const auto addlo = lo16.As<I32x4>().Cast<F64x2>();
    return { base[0].MulAdd(mul16, addlo[0]), base[1].MulAdd(mul16, addlo[1]) };*/
    constexpr double Adder32 = 4294967296.f; // UINT32_MAX+1
    // if [sig], will be treated as negative, need to add Adder32
    const auto sig01 = _mm_castsi128_pd(Shuffle<0, 0, 1, 1>()), sig23 = _mm_castsi128_pd(Shuffle<2, 2, 3, 3>());
    const auto adder1 = _mm_blendv_pd(_mm_setzero_pd(), _mm_set1_pd(Adder32), sig01);
    const auto adder2 = _mm_blendv_pd(_mm_setzero_pd(), _mm_set1_pd(Adder32), sig23);
    const auto f64 = As<I32x4>().Cast<F64x2>();
    return { f64[0].Add(adder1), f64[1].Add(adder2) };
#endif
}


struct alignas(16) I16x8 : public detail::Common16x8<I16x8, int16_t>
{
    using Common16x8<I16x8, int16_t>::Common16x8;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I16x8 VECCALL Compare(const I16x8 other) const
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi16(Data, other);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi16(Data, other);
            else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

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
    template<typename T, CastMode Mode = detail::CstMode<I16x8, T>(), typename... Args>
    typename CastTyper<I16x8, T>::Type VECCALL Cast(const Args&... args) const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I32x4, 2> VECCALL I16x8::Cast<I32x4, CastMode::RangeUndef>() const
{
    return { _mm_cvtepi16_epi32(Data), _mm_cvtepi16_epi32(MoveHiToLo()) };
}
template<> forceinline Pack<U32x4, 2> VECCALL I16x8::Cast<U32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL I16x8::Cast<I64x2, CastMode::RangeUndef>() const
{
    const auto ret0 = _mm_cvtepi16_epi64(Data);
    const auto ret1 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U64x2, 4> VECCALL I16x8::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL I16x8::Cast<F32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 4> VECCALL I16x8::Cast<F64x2, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F64x2>();
}
#endif


struct alignas(16) U16x8 : public detail::Common16x8<U16x8, uint16_t>
{
    using Common16x8<U16x8, uint16_t>::Common16x8;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U16x8 VECCALL Compare(const U16x8 other) const
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I16x8>().Compare<Cmp, Msk>(other.As<I16x8>()).template As<U16x8>();
        }
        else
        {
            const U16x8 sigMask(static_cast<uint16_t>(0x8000));
            return Xor(sigMask).As<I16x8>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I16x8>()).template As<U16x8>();
        }
    }

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
    template<typename T, CastMode Mode = detail::CstMode<U16x8, T>(), typename... Args>
    typename CastTyper<U16x8, T>::Type VECCALL Cast(const Args&... args) const;
};
template<> forceinline Pack<I32x4, 2> VECCALL U16x8::Cast<I32x4, CastMode::RangeUndef>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi16(Data, zero), _mm_unpackhi_epi16(Data, zero) };
}
template<> forceinline Pack<U32x4, 2> VECCALL U16x8::Cast<U32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL U16x8::Cast<I64x2, CastMode::RangeUndef>() const
{
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi16(Data, zero), hi = _mm_unpackhi_epi16(Data, zero);
    return { _mm_unpacklo_epi32(lo, zero), _mm_unpackhi_epi32(lo, zero), _mm_unpacklo_epi32(hi, zero), _mm_unpackhi_epi32(hi, zero) };
}
template<> forceinline Pack<U64x2, 4> VECCALL U16x8::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL U16x8::Cast<F32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 4> VECCALL U16x8::Cast<F64x2, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F64x2>();
}


struct alignas(16) I8x16 : public detail::Common8x16<I8x16, int8_t>
{
    using Common8x16<I8x16, int8_t>::Common8x16;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I8x16 VECCALL Compare(const I8x16 other) const
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi8(Data, other);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi8(Data, other);
            else static_assert(!AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

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
    Pack<I16x8, 2> VECCALL MulX(const I8x16& other) const;
#endif
    template<typename T, CastMode Mode = detail::CstMode<I8x16, T>(), typename... Args>
    typename CastTyper<I8x16, T>::Type VECCALL Cast(const Args&... args) const;
};
#if COMMON_SIMD_LV >= 41
template<> forceinline Pack<I16x8, 2> VECCALL I8x16::Cast<I16x8, CastMode::RangeUndef>() const
{
    return { _mm_cvtepi8_epi16(Data), _mm_cvtepi8_epi16(MoveHiToLo()) };
}
template<> forceinline Pack<U16x8, 2> VECCALL I8x16::Cast<U16x8, CastMode::RangeUndef>() const
{
    return Cast<I16x8>().As<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL I8x16::Cast<I32x4, CastMode::RangeUndef>() const
{
    const auto ret0 = _mm_cvtepi8_epi32(Data);
    const auto ret1 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U32x4, 4> VECCALL I8x16::Cast<U32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL I8x16::Cast<I64x2, CastMode::RangeUndef>() const
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
template<> forceinline Pack<U64x2, 8> VECCALL I8x16::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().As<U64x2>();
}
forceinline Pack<I16x8, 2> VECCALL I8x16::MulX(const I8x16& other) const
{
    const auto self16 = Cast<I16x8>(), other16 = other.Cast<I16x8, CastMode::RangeUndef>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
template<> forceinline Pack<F32x4, 4> VECCALL I8x16::Cast<F32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 8> VECCALL I8x16::Cast<F64x2, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F64x2>();
}
#endif


struct alignas(16) U8x16 : public detail::Common8x16<U8x16, uint8_t>
{
    using Common8x16<U8x16, uint8_t>::Common8x16;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U8x16 VECCALL Compare(const U8x16 other) const
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I8x16>().Compare<Cmp, Msk>(other.As<I8x16>()).template As<U8x16>();
        }
        else
        {
            const U8x16 sigMask(static_cast<uint8_t>(0x80));
            return Xor(sigMask).As<I8x16>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I8x16>()).template As<U8x16>();
        }
    }

    // arithmetic operations
    forceinline U8x16 VECCALL SatAdd(const U8x16& other) const { return _mm_adds_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL SatSub(const U8x16& other) const { return _mm_subs_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Max(const U8x16& other) const { return _mm_max_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Min(const U8x16& other) const { return _mm_min_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Abs() const { return Data; }
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
    template<typename T, CastMode Mode = detail::CstMode<U8x16, T>(), typename... Args>
    typename CastTyper<U8x16, T>::Type VECCALL Cast(const Args&... args) const;
};
template<> forceinline Pack<I16x8, 2> VECCALL U8x16::Cast<I16x8, CastMode::RangeUndef>() const
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi8(Data, zero), _mm_unpackhi_epi8(Data, zero) };
}
template<> forceinline Pack<U16x8, 2> VECCALL U8x16::Cast<U16x8, CastMode::RangeUndef>() const
{
    return Cast<I16x8>().Cast<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL U8x16::Cast<I32x4, CastMode::RangeUndef>() const
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
template<> forceinline Pack<U32x4, 4> VECCALL U8x16::Cast<U32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL U8x16::Cast<I64x2, CastMode::RangeUndef>() const
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
template<> forceinline Pack<U64x2, 8> VECCALL U8x16::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Cast<I64x2>().Cast<U64x2>();
}
template<> forceinline Pack<F32x4, 4> VECCALL U8x16::Cast<F32x4, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 8> VECCALL U8x16::Cast<F64x2, CastMode::RangeUndef>() const
{
    return Cast<I32x4>().Cast<F64x2>();
}
forceinline Pack<U16x8, 2> VECCALL U8x16::MulX(const U8x16& other) const
{
    const auto self16 = Cast<U16x8>(), other16 = other.Cast<U16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}


template<typename T, typename E>
forceinline T VECCALL detail::Common8x16<T, E>::Shuffle(const U8x16& pos) const
{
    return _mm_shuffle_epi8(this->Data, pos);
}
template<typename T, typename E>
forceinline T VECCALL detail::Common8x16<T, E>::ShiftRightArith(const uint8_t bits) const
{
    if constexpr (std::is_unsigned_v<E>)
        return ShiftRightLogic(bits);
    else
    {
        const auto bit16 = static_cast<const T&>(*this).template As<I16x8>();
        const I16x8 keepMask(static_cast<int16_t>(0xff00));
        const auto shiftHi = bit16                             .ShiftRightArith(bits).And(keepMask).template As<T>();
        const auto shiftLo = bit16.template ShiftLeftLogic<8>().ShiftRightArith(bits).And(keepMask);
        return shiftHi.Or(shiftLo.template ShiftRightLogic<8>().template As<T>());
    }
}
template<typename T, typename E>
template<uint8_t N>
forceinline T VECCALL detail::Common8x16<T, E>::ShiftRightArith() const
{
    if constexpr (std::is_unsigned_v<E>)
        return ShiftRightLogic<N>();
    else
    {
        const auto bit16 = static_cast<const T&>(*this).template As<I16x8>();
        const I16x8 keepMask(static_cast<int16_t>(0xff00));
        const auto shiftHi = bit16                             .template ShiftRightArith<N>().And(keepMask).template As<T>();
        const auto shiftLo = bit16.template ShiftLeftLogic<8>().template ShiftRightArith<N>().And(keepMask);
        return shiftHi.Or(shiftLo.template ShiftRightLogic<8>().template As<T>());
    }
}


template<> forceinline U64x2 VECCALL I64x2::Cast<U64x2, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline I64x2 VECCALL U64x2::Cast<I64x2, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline U32x4 VECCALL I32x4::Cast<U32x4, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline I32x4 VECCALL U32x4::Cast<I32x4, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline U16x8 VECCALL I16x8::Cast<U16x8, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline I16x8 VECCALL U16x8::Cast<I16x8, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline U8x16 VECCALL I8x16::Cast<U8x16, CastMode::RangeUndef>() const
{
    return Data;
}
template<> forceinline I8x16 VECCALL U8x16::Cast<I8x16, CastMode::RangeUndef>() const
{
    return Data;
}


template<> forceinline U32x4 VECCALL U64x2::Cast<U32x4, CastMode::RangeTrunc>(const U64x2& arg1) const
{
    const auto lo =      As<U32x4>().Shuffle<0, 2, 0, 0>();
    const auto hi = arg1.As<U32x4>().Shuffle<0, 2, 0, 0>();
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeTrunc>(const U32x4& arg1) const
{
    const auto mask = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto lo = _mm_shuffle_epi8(Data, mask);
    const auto hi = _mm_shuffle_epi8(arg1, mask);
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeTrunc>(const U16x8& arg1) const
{
    const auto mask = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto lo = _mm_shuffle_epi8(Data, mask);
    const auto hi = _mm_shuffle_epi8(arg1, mask);
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U16x8 VECCALL U64x2::Cast<U16x8, CastMode::RangeTrunc>(const U64x2& arg1, const U64x2& arg2, const U64x2& arg3) const
{
    const auto mask = _mm_setr_epi8(0, 1, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto dat0 = _mm_shuffle_epi8(Data, mask);
    const auto dat1 = _mm_shuffle_epi8(arg1, mask);
    const auto dat2 = _mm_shuffle_epi8(arg2, mask);
    const auto dat3 = _mm_shuffle_epi8(arg3, mask);
    const auto dat02 = _mm_unpacklo_epi32(dat0, dat2);
    const auto dat13 = _mm_unpacklo_epi32(dat1, dat3);
    return _mm_unpacklo_epi32(dat02, dat13);
}
template<> forceinline U8x16 VECCALL U32x4::Cast<U8x16, CastMode::RangeTrunc>(const U32x4& arg1, const U32x4& arg2, const U32x4& arg3) const
{
    const auto mask = _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto dat0 = _mm_shuffle_epi8(Data, mask);
    const auto dat1 = _mm_shuffle_epi8(arg1, mask);
    const auto dat2 = _mm_shuffle_epi8(arg2, mask);
    const auto dat3 = _mm_shuffle_epi8(arg3, mask);
    const auto dat02 = _mm_unpacklo_epi32(dat0, dat2);
    const auto dat13 = _mm_unpacklo_epi32(dat1, dat3);
    return _mm_unpacklo_epi32(dat02, dat13);
}
template<> forceinline U8x16 VECCALL U64x2::Cast<U8x16, CastMode::RangeTrunc>(const U64x2& arg1, const U64x2& arg2, const U64x2& arg3,
    const U64x2& arg4, const U64x2& arg5, const U64x2& arg6, const U64x2& arg7) const
{
    const auto mask = _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffff0800);
    const auto dat0 = _mm_shuffle_epi8(Data, mask);//a0000000
    const auto dat1 = _mm_shuffle_epi8(arg1, mask);//b0000000
    const auto dat2 = _mm_shuffle_epi8(arg2, mask);//c0000000
    const auto dat3 = _mm_shuffle_epi8(arg3, mask);//d0000000
    const auto dat4 = _mm_shuffle_epi8(arg4, mask);//e0000000
    const auto dat5 = _mm_shuffle_epi8(arg5, mask);//f0000000
    const auto dat6 = _mm_shuffle_epi8(arg6, mask);//g0000000
    const auto dat7 = _mm_shuffle_epi8(arg7, mask);//h0000000
    const auto dat01 = _mm_unpacklo_epi16(dat0, dat1);//ab000000
    const auto dat23 = _mm_unpacklo_epi16(dat2, dat3);//cd000000
    const auto dat45 = _mm_unpacklo_epi16(dat4, dat5);//ef000000
    const auto dat67 = _mm_unpacklo_epi16(dat6, dat7);//gh000000
    const auto dat0123 = _mm_unpacklo_epi32(dat01, dat23);//abcd0000
    const auto dat4567 = _mm_unpacklo_epi32(dat45, dat67);//efgh0000
    return _mm_unpacklo_epi64(dat0123, dat4567);//abcdefgh
}
template<> forceinline I32x4 VECCALL I64x2::Cast<I32x4, CastMode::RangeTrunc>(const I64x2& arg1) const
{
    return As<U64x2>().Cast<U32x4>(arg1.As<U64x2>()).As<I32x4>();
}
template<> forceinline I16x8 VECCALL I32x4::Cast<I16x8, CastMode::RangeTrunc>(const I32x4& arg1) const
{
    return As<U32x4>().Cast<U16x8>(arg1.As<U32x4>()).As<I16x8>();
}
template<> forceinline I8x16 VECCALL I16x8::Cast<I8x16, CastMode::RangeTrunc>(const I16x8& arg1) const
{
    return As<U16x8>().Cast<U8x16>(arg1.As<U16x8>()).As<I8x16>();
}
template<> forceinline I16x8 VECCALL I64x2::Cast<I16x8, CastMode::RangeTrunc>(const I64x2& arg1, const I64x2& arg2, const I64x2& arg3) const
{
    return As<U64x2>().Cast<U16x8>(arg1.As<U64x2>(), arg2.As<U64x2>(), arg3.As<U64x2>()).As<I16x8>();
}
template<> forceinline I8x16 VECCALL I32x4::Cast<I8x16, CastMode::RangeTrunc>(const I32x4& arg1, const I32x4& arg2, const I32x4& arg3) const
{
    return As<U32x4>().Cast<U8x16>(arg1.As<U32x4>(), arg2.As<U32x4>(), arg3.As<U32x4>()).As<I8x16>();
}
template<> forceinline I8x16 VECCALL I64x2::Cast<I8x16, CastMode::RangeTrunc>(const I64x2& arg1, const I64x2& arg2, const I64x2& arg3,
    const I64x2& arg4, const I64x2& arg5, const I64x2& arg6, const I64x2& arg7) const
{
    return As<U64x2>().Cast<U8x16>(arg1.As<U64x2>(), arg2.As<U64x2>(), arg3.As<U64x2>(),
        arg4.As<U64x2>(), arg5.As<U64x2>(), arg6.As<U64x2>(), arg7.As<U64x2>()).As<I8x16>();
}


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeUndef>() const
{
    return _mm_cvttps_epi32(Data);
}
template<> forceinline I16x8 VECCALL F32x4::Cast<I16x8, CastMode::RangeUndef>(const F32x4& arg1) const
{
    return Cast<I32x4>().Cast<I16x8>(arg1.Cast<I32x4>());
}
template<> forceinline U16x8 VECCALL F32x4::Cast<U16x8, CastMode::RangeUndef>(const F32x4& arg1) const
{
    return Cast<I16x8>(arg1).As<U16x8>();
}
template<> forceinline I8x16 VECCALL F32x4::Cast<I8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const
{
    return Cast<I32x4>().Cast<I8x16>(arg1.Cast<I32x4>(), arg2.Cast<I32x4>(), arg3.Cast<I32x4>());
}
template<> forceinline U8x16 VECCALL F32x4::Cast<U8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const
{
    return Cast<I8x16>(arg1, arg2, arg3).As<U8x16>();
}
template<> forceinline Pack<F64x2, 2> VECCALL F32x4::Cast<F64x2, CastMode::RangeUndef>() const
{
    return { _mm_cvtps_pd(Data), _mm_cvtps_pd(As<I32x4>().MoveHiToLo().As<F32x4>()) };
}
template<> forceinline F32x4 VECCALL F64x2::Cast<F32x4, CastMode::RangeUndef>(const F64x2& arg1) const
{
    return _mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(_mm_cvtpd_ps(Data)), _mm_castps_pd(_mm_cvtpd_ps(arg1.Data))));
}


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeSaturate>() const
{
    const F32x4 minVal = static_cast<float>(INT32_MIN), maxVal = static_cast<float>(INT32_MAX);
    const auto val = Cast<I32x4, CastMode::RangeUndef>();
    // INT32 loses precision, need maunally bit-select
    const auto isLe = Compare<CompareType::LessEqual,    MaskType::FullEle>(minVal).As<I32x4>();
    const auto isGe = Compare<CompareType::GreaterEqual, MaskType::FullEle>(maxVal).As<I32x4>();
    /*const auto isLe = _mm_castps_si128(_mm_cmple_ps(Data, minVal));
    const auto isGe = _mm_castps_si128(_mm_cmpge_ps(Data, maxVal));*/
#if COMMON_SIMD_LV >= 41
    const auto satMin = _mm_blendv_epi8(val, I32x4(INT32_MIN), isLe);
    return _mm_blendv_epi8(satMin, I32x4(INT32_MAX), isGe);
#else
    const auto minItem = I32x4(INT32_MIN).And(isLe), maxItem = I32x4(INT32_MAX).And(isGe);
    const auto keptItem = _mm_andnot_si128(_mm_or_si128(isGe, isLe), val);
    return minItem.Or(maxItem).Or(keptItem);
#endif
}
template<> forceinline I16x8 VECCALL F32x4::Cast<I16x8, CastMode::RangeSaturate>(const F32x4& arg1) const
{
    const F32x4 minVal = static_cast<float>(INT16_MIN), maxVal = static_cast<float>(INT16_MAX);
    return Min(maxVal).Max(minVal).Cast<I16x8, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline U16x8 VECCALL F32x4::Cast<U16x8, CastMode::RangeSaturate>(const F32x4& arg1) const
{
    const F32x4 minVal = 0, maxVal = static_cast<float>(UINT16_MAX);
    return Min(maxVal).Max(minVal).Cast<U16x8, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline I8x16 VECCALL F32x4::Cast<I8x16, CastMode::RangeSaturate>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const
{
    const F32x4 minVal = static_cast<float>(INT8_MIN), maxVal = static_cast<float>(INT8_MAX);
    return Min(maxVal).Max(minVal).Cast<I8x16, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline U8x16 VECCALL F32x4::Cast<U8x16, CastMode::RangeSaturate>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const
{
    const F32x4 minVal = 0, maxVal = static_cast<float>(UINT8_MAX);
    return Min(maxVal).Max(minVal).Cast<U8x16, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline I16x8 VECCALL I32x4::Cast<I16x8, CastMode::RangeSaturate>(const I32x4& arg1) const
{
    return _mm_packs_epi32(Data, arg1);
}
#if COMMON_SIMD_LV >= 41
template<> forceinline U16x8 VECCALL I32x4::Cast<U16x8, CastMode::RangeSaturate>(const I32x4& arg1) const
{
    return _mm_packus_epi32(Data, arg1);
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeSaturate>(const U32x4& arg1) const
{
    const auto data_ = Min(UINT16_MAX).As<I32x4>(), arg1_ = arg1.Min(UINT16_MAX).As<I32x4>();
    return data_.Cast<U16x8, CastMode::RangeSaturate>(arg1_);
}
#endif
template<> forceinline I8x16 VECCALL I16x8::Cast<I8x16, CastMode::RangeSaturate>(const I16x8& arg1) const
{
    return _mm_packs_epi16(Data, arg1);
}
template<> forceinline U8x16 VECCALL I16x8::Cast<U8x16, CastMode::RangeSaturate>(const I16x8& arg1) const
{
    return _mm_packus_epi16(Data, arg1);
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeSaturate>(const U16x8& arg1) const
{
    const auto data_ = Min(UINT8_MAX).As<I16x8>(), arg1_ = arg1.Min(UINT8_MAX).As<I16x8>();
    return data_.Cast<U8x16, CastMode::RangeSaturate>(arg1_);
}


}
}
