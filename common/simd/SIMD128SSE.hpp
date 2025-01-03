#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"

#if COMMON_SIMD_LV < 41
#   error require at least SSE4.1
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
#endif


namespace COMMON_SIMD_NAMESPACE
{

struct I8x16;
struct U8x16;
struct I16x8;
struct U16x8;
struct F16x8;
struct I32x4;
struct U32x4;
struct F32x4;
struct I64x2;
struct F64x2;


namespace detail
{
template<> forceinline __m128  VECCALL AsType(__m128  from) noexcept { return from; }
template<> forceinline __m128i VECCALL AsType(__m128  from) noexcept { return _mm_castps_si128(from); }
template<> forceinline __m128d VECCALL AsType(__m128  from) noexcept { return _mm_castps_pd(from); }
template<> forceinline __m128  VECCALL AsType(__m128i from) noexcept { return _mm_castsi128_ps(from); }
template<> forceinline __m128i VECCALL AsType(__m128i from) noexcept { return from; }
template<> forceinline __m128d VECCALL AsType(__m128i from) noexcept { return _mm_castsi128_pd(from); }
template<> forceinline __m128  VECCALL AsType(__m128d from) noexcept { return _mm_castpd_ps(from); }
template<> forceinline __m128i VECCALL AsType(__m128d from) noexcept { return _mm_castpd_si128(from); }
template<> forceinline __m128d VECCALL AsType(__m128d from) noexcept { return from; }


forceinline constexpr int CmpTypeImm(CompareType cmp) noexcept
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

forceinline constexpr int RoundModeImm(RoundMode mode) noexcept
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

// always use TZCNT since additional logic wull make it compatible when fallback as BSF. AMD is better in TZCNT
// for gcc/clang, use inline assembly to avoid target mismatch
template<size_t Count>
forceinline std::pair<uint32_t, uint32_t> VECCALL SignBitToIdx(const uint32_t signbits) noexcept
{
#if COMMON_SIMD_LV >= 150 // FMA means Haswell/Gracemont/Piledriver or newer, should garuantee TZCNT(BMI1) support
    if constexpr (Count == 16)
    {
#   if COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
        uint16_t result;
        __asm__("tzcnt %w1, %w0" : "=r"(result) : "r"(signbits) : "cc");
#   else
        uint16_t result = _tzcnt_u16(static_cast<uint16_t>(signbits));
#   endif
        return { result, signbits };
    }
    else if constexpr (Count == 32)
    {
#   if COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
        uint32_t result;
        __asm__("tzcnt %1, %0" : "=r"(result) : "r"(signbits) : "cc");
#   else
        uint32_t result = _tzcnt_u32(signbits);
#   endif
        return { result, signbits };
    }
    else if constexpr (Count == 64)
    {
#   if COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
        uint64_t result;
        __asm__("tzcnt %q1, %q0" : "=r"(result) : "r"((uint64_t)signbits) : "cc");
#   else
        uint64_t result = _tzcnt_u64(signbits);
#   endif
        return { static_cast<uint32_t>(result), signbits };
    }
    else
#endif
    // more flexible support with count, though not needed
    if constexpr (Count < 32)
    {
        constexpr auto extraMask = 1u << Count;
        const auto fixedbits = signbits | extraMask;
#   if COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
        uint32_t result;
        __asm__("tzcnt %1, %0" : "=r"(result) : "r"(fixedbits) : "cc");
#   else
        uint32_t result = _tzcnt_u32(fixedbits);
#   endif
        return { result, signbits };
    }
    else if constexpr (Count < 64)
    {
        constexpr auto extraMask = uint64_t(1u) << Count;
        const auto fixedbits = signbits | extraMask;
#   if COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
        uint64_t result;
        __asm__("tzcnt %q1, %q0" : "=r"(result) : "r"(fixedbits) : "cc");
#   else
        uint64_t result = _tzcnt_u32(fixedbits);
#   endif
        return { static_cast<uint32_t>(result), signbits };
    }
    else
        static_assert(!AlwaysTrue2<Count>);
}



template<typename T, typename E>
struct SSE128Shared : public CommonOperators<T>
{
    // shuffle operations
    forceinline Pack<T, 2> VECCALL Zip(const T& other) const noexcept
    {
        return { static_cast<const T*>(this)->ZipLo(other), static_cast<const T*>(this)->ZipHi(other) };
    }
};

// For integer
template<typename T, typename E, size_t N>
struct SSE128Common : public SSE128Shared<T, E>
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
    forceinline constexpr SSE128Common() noexcept : Data() { }
    forceinline explicit SSE128Common(const E* ptr) noexcept : Data(_mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr))) { }
    forceinline constexpr SSE128Common(const __m128i val) noexcept : Data(val) { }
    forceinline void VECCALL Load(const E* ptr) noexcept { Data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)); }
    forceinline void VECCALL Save(E* ptr) const noexcept { _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), Data); }
    forceinline constexpr operator const __m128i& () const noexcept { return Data; }

    // logic operations
    forceinline T VECCALL And(const T& other) const noexcept
    {
        return _mm_and_si128(Data, other.Data);
    }
    forceinline T VECCALL Or(const T& other) const noexcept
    {
        return _mm_or_si128(Data, other.Data);
    }
    forceinline T VECCALL Xor(const T& other) const noexcept
    {
        return _mm_xor_si128(Data, other.Data);
    }
    forceinline T VECCALL AndNot(const T& other) const noexcept
    {
        return _mm_andnot_si128(Data, other.Data);
    }
    forceinline T VECCALL Not() const noexcept
    {
        return _mm_xor_si128(Data, _mm_set1_epi8(-1));
    }

    // arithmetic operations
    forceinline T VECCALL MulAddLo(const T& muler, const T& adder) const noexcept
    {
        return static_cast<const T*>(this)->template MulLo<true>(muler).Add(adder);
    }
    forceinline T VECCALL MulScalarAddLo(const E muler, const T& adder) const noexcept
    {
        return MulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline T VECCALL MulScalarAddLo(const T& muler, const T& adder) const noexcept
    {
        static_assert(Idx < N, "select index should be in [0,N)");
        return static_cast<const T*>(this)->template MulLo<true>(muler.template Broadcast<Idx>()).Add(adder);
    }
    forceinline T VECCALL MulSubLo(const T& muler, const T& suber) const noexcept
    {
        return static_cast<const T*>(this)->template MulLo<true>(muler).Sub(suber);
    }
    forceinline T VECCALL MulScalarSubLo(const E muler, const T& suber) const noexcept
    {
        return MulSubLo(muler, suber);
    }
    template<size_t Idx>
    forceinline T VECCALL MulScalarSubLo(const T& muler, const T& suber) const noexcept
    {
        static_assert(Idx < N, "select index should be in [0,N)");
        return static_cast<const T*>(this)->template MulLo<true>(muler.template Broadcast<Idx>()).Sub(suber);
    }
    forceinline T VECCALL NMulAddLo(const T& muler, const T& adder) const noexcept
    {
        return adder.Sub(static_cast<const T*>(this)->template MulLo<true>(muler));
    }
    forceinline T VECCALL NMulScalarAddLo(const E muler, const T& adder) const noexcept
    {
        return NMulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline T VECCALL NMulScalarAddLo(const T& muler, const T& adder) const noexcept
    {
        static_assert(Idx < N, "select index should be in [0,N)");
        return adder.Sub(static_cast<const T*>(this)->template MulLo<true>(muler.template Broadcast<Idx>()));
    }
    forceinline T VECCALL NMulSubLo(const T& muler, const T& suber) const noexcept
    {
        static_assert(std::is_signed_v<E>, "not allowed for unsigned vector");
        return static_cast<const T*>(this)->template MulLo<true>(muler).Add(suber).Neg();
    }
    forceinline T VECCALL NMulScalarSubLo(const E muler, const T& suber) const noexcept
    {
        static_assert(std::is_signed_v<E>, "not allowed for unsigned vector");
        return NMulSubLo(muler, suber);
    }
    template<size_t Idx>
    forceinline T VECCALL NMulScalarSubLo(const T& muler, const T& suber) const noexcept
    {
        static_assert(std::is_signed_v<E>, "not allowed for unsigned vector");
        static_assert(Idx < N, "select index should be in [0,N)");
        return static_cast<const T*>(this)->template MulLo<true>(muler.template Broadcast<Idx>()).Add(suber).Neg();
    }

    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        const auto signbits = static_cast<const T*>(this)->ExtractSignBit();
        return SignBitToIdx<Count>(signbits);
    }
    template<uint8_t Cnt>
    forceinline T VECCALL MoveToHi() const noexcept
    {
        static_assert(Cnt <= N, "move count should be in [0,N]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N) return AllZero();
        else return _mm_slli_si128(Data, Cnt * sizeof(E));
    }
    template<uint8_t Cnt>
    forceinline T VECCALL MoveToLo() const noexcept
    {
        static_assert(Cnt <= N, "move count should be in [0,N]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N) return AllZero();
        else return _mm_srli_si128(Data, Cnt * sizeof(E));
    }
    forceinline T VECCALL MoveHiToLo() const noexcept { return MoveToLo<N / 2>(); }
    template<uint8_t Cnt>
    forceinline T VECCALL MoveToLoWith(const T& hi) const noexcept
    {
        static_assert(Cnt <= N, "move count should be in [0,N]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N) return hi;
        else return _mm_alignr_epi8(hi.Data, Data, Cnt * sizeof(E));
    }

    forceinline bool VECCALL IsAllZero() const noexcept
    {
        return _mm_testz_si128(Data, Data) != 0;
    }

    forceinline static T VECCALL AllZero() noexcept { return _mm_setzero_si128(); }
};


template<typename T, typename E>
struct alignas(16) Common64x2 : public SSE128Common<T, E, 2>
{
private:
    using Base = SSE128Common<T, E, 2>;
public:
    using Base::Base;
    forceinline Common64x2(const E val) noexcept : Base(_mm_set1_epi64x(static_cast<int64_t>(val))) { }
    forceinline Common64x2(const E lo, const E hi) noexcept :
        Base(_mm_set_epi64x(static_cast<int64_t>(hi), static_cast<int64_t>(lo))) { }

    // shuffle operations
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        return _mm_movemask_pd(_mm_castsi128_pd(this->Data));
    }
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        return _mm_shuffle_epi32(this->Data, _MM_SHUFFLE(Hi * 2 + 1, Hi * 2, Lo * 2 + 1, Lo * 2));
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo, const uint8_t Hi) const noexcept
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
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return Shuffle<Idx, Idx>();
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm_set_epi64x(0x08090a0b0c0d0e0fULL, 0x0001020304050607ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept { return _mm_unpacklo_epi64(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const noexcept { return _mm_unpackhi_epi64(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return ZipLo(other); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return ZipHi(other); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr (Msk != MaskType::FullEle)
            return _mm_castpd_si128(_mm_blendv_pd(_mm_castsi128_pd(this->Data), _mm_castsi128_pd(other.Data), _mm_castsi128_pd(mask))); // less inst with higher latency
//# if COMMON_SIMD_LV >= 320
//        mask = _mm_srai_epi64(mask, 64);
//# else
//        mask = _mm_srai_epi32(_mm_shuffle_epi32(mask, 0xf5), 32);
//# endif
        else
            return _mm_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        static_assert(Mask <= 0b11, "Only allow 2 bits");
#if COMMON_SIMD_LV >= 200
        return _mm_blend_epi32(this->Data, other.Data, (Mask & 0b1 ? 0b11 : 0b00) | (Mask & 0b10 ? 0b1100 : 0b0000));
#else
        return _mm_blend_epi16(this->Data, other.Data, (Mask & 0b1 ? 0xf : 0x0) | (Mask & 0b10 ? 0xf0 : 0x00));
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm_add_epi64(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm_sub_epi64(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        if constexpr (Enforce) return _mm_mullo_epi64_force(this->Data, other.Data);
        else return _mm_mullo_epi64(this->Data, other.Data);
#else
        const E loA = _mm_extract_epi64(this->Data, 0), hiA = _mm_extract_epi64(this->Data, 1);
        const E loB = _mm_extract_epi64(other.Data, 0), hiB = _mm_extract_epi64(other.Data, 1);
        return { static_cast<E>(loA * loB), static_cast<E>(hiA * hiB) };
#endif
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = MulLo(other); return *static_cast<T*>(this); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm_sll_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
        else
#if COMMON_SIMD_LV >= 320
            return _mm_sra_epi64(this->Data, _mm_cvtsi32_si128(bits));
#else
        {
            if (bits == 0) return this->Data;
            const auto sign32H = _mm_srai_epi32(this->Data, 32); // only high part useful
            const auto signAll = _mm_shuffle_epi32(sign32H, 0b11110101);
            if constexpr (MayOverflow)
            {
                if (bits > 63) return signAll;
            }
            const auto zero64 = _mm_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
            const auto sign64 = _mm_sll_epi64(signAll, _mm_cvtsi32_si128(64 - bits));
            return _mm_or_si128(zero64, sign64);
        }
#endif
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm_sllv_epi64(this->Data, bits.Data);
#else
        const auto lo = _mm_sll_epi64(this->Data, bits.Data);
        const auto hi = _mm_sll_epi64(this->Data, _mm_srli_si128(bits.Data, 8));
        return _mm_blend_epi16(lo, hi, 0xf0);
#endif
    }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm_srlv_epi64(this->Data, bits.Data);
#else
        const auto lo = _mm_srl_epi64(this->Data, bits.Data);
        const auto hi = _mm_srl_epi64(this->Data, _mm_srli_si128(bits.Data, 8));
        return _mm_blend_epi16(lo, hi, 0xf0);
#endif
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept 
    {
        if constexpr (N >= 64)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_slli_epi64(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept 
    {
        if constexpr (N >= 64)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_srli_epi64(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const  noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srli_epi64(this->Data, N);
#if COMMON_SIMD_LV >= 320
        else
            return _mm_srai_epi64(this->Data, N);
#else
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

    forceinline static T VECCALL LoadLo(const E val) noexcept { return _mm_loadu_si64(&val); }
    forceinline static T VECCALL LoadLo(const E* ptr) noexcept { return _mm_loadu_si64(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common32x4 : public SSE128Common<T, E, 4>
{
private:
    using Base = SSE128Common<T, E, 4>;
public:
    using Base::Base;
    forceinline Common32x4(const E val) noexcept : Base(_mm_set1_epi32(static_cast<int32_t>(val))) { }
    forceinline Common32x4(const E lo0, const E lo1, const E lo2, const E hi3) noexcept :
        Base(_mm_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(hi3))) { }

    // shuffle operations
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        return _mm_movemask_ps(_mm_castsi128_ps(this->Data));
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm_shuffle_epi32(this->Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const noexcept
    {
#if COMMON_SIMD_LV >= 100
        return _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(this->Data), _mm_setr_epi32(Lo0, Lo1, Lo2, Hi3)));
#else
        return T(this->Val[Lo0], this->Val[Lo1], this->Val[Lo2], this->Val[Hi3]);
#endif
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return Shuffle<Idx, Idx, Idx, Idx>();
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept { return _mm_unpacklo_epi32(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const noexcept { return _mm_unpackhi_epi32(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0b1010>(_mm_slli_epi64(other.Data, 32)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0b0101>(_mm_srli_epi64(this->Data, 32)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr (Msk != MaskType::FullEle)
            return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(this->Data), _mm_castsi128_ps(other.Data), _mm_castsi128_ps(mask))); // less inst with higher latency
            //mask = _mm_srai_epi32(mask, 32); // make sure all bits are covered
        else
            return _mm_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
#if COMMON_SIMD_LV >= 200
        return _mm_blend_epi32(this->Data, other.Data, Mask);
#else
        return _mm_blend_epi16(this->Data, other.Data, (Mask & 0b1 ? 0b11 : 0) |
            (Mask & 0b10 ? 0b1100 : 0) | (Mask & 0b100 ? 0b110000 : 0) | (Mask & 0b1000 ? 0b11000000 : 0));
#endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm_add_epi32(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm_sub_epi32(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept
    {
        if constexpr (Enforce) return _mm_mullo_epi32_force(this->Data, other.Data);
        else return _mm_mullo_epi32(this->Data, other.Data);
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = MulLo(other); return *static_cast<T*>(this); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm_sll_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm_srl_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi32(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm_sra_epi32(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm_sllv_epi32(this->Data, bits.Data);
#else
        const auto bits64Lo = _mm_cvtepi32_epi64(bits.Data);
        const auto bits64Hi = _mm_cvtepi32_epi64(_mm_srli_si128(bits.Data, 8));
        const auto shift0 = _mm_sll_epi32(this->Data, bits64Lo);
        const auto shift1 = _mm_sll_epi32(this->Data, _mm_srli_si128(bits64Lo, 8));
        const auto shift2 = _mm_sll_epi32(this->Data, bits64Hi);
        const auto shift3 = _mm_sll_epi32(this->Data, _mm_srli_si128(bits64Hi, 8));
        const auto shift01 = _mm_blend_epi16(shift0, shift1, 0x0c);
        const auto shift23 = _mm_blend_epi16(shift2, shift3, 0xc0);
        return _mm_blend_epi16(shift01, shift23, 0xf0);
#endif
    }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm_srlv_epi32(this->Data, bits.Data);
#else
        const auto bits64Lo = _mm_cvtepi32_epi64(bits.Data);
        const auto bits64Hi = _mm_cvtepi32_epi64(_mm_srli_si128(bits.Data, 8));
        const auto shift0 = _mm_srl_epi32(this->Data, bits64Lo);
        const auto shift1 = _mm_srl_epi32(this->Data, _mm_srli_si128(bits64Lo, 8));
        const auto shift2 = _mm_srl_epi32(this->Data, bits64Hi);
        const auto shift3 = _mm_srl_epi32(this->Data, _mm_srli_si128(bits64Hi, 8));
        const auto shift01 = _mm_blend_epi16(shift0, shift1, 0x0c);
        const auto shift23 = _mm_blend_epi16(shift2, shift3, 0xc0);
        return _mm_blend_epi16(shift01, shift23, 0xf0);
#endif
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept 
    {
        if constexpr (N >= 32)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_slli_epi32(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept 
    {
        if constexpr (N >= 32)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_srli_epi32(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>) 
            return _mm_srli_epi32(this->Data, N);
        else 
            return _mm_srai_epi32(this->Data, N);
    }

    forceinline static T VECCALL LoadLo(const E val) noexcept { return _mm_loadu_si32_correct(&val); }
    forceinline static T VECCALL LoadLo(const E* ptr) noexcept { return _mm_loadu_si32_correct(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common16x8 : public SSE128Common<T, E, 8>
{
private:
    using Base = SSE128Common<T, E, 8>;
public:
    using Base::Base;
    forceinline Common16x8(const E val) noexcept : Base(_mm_set1_epi16(val)) { }
    forceinline Common16x8(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7) noexcept :
        Base(_mm_setr_epi16(static_cast<int16_t>(lo0), static_cast<int16_t>(lo1), static_cast<int16_t>(lo2), static_cast<int16_t>(lo3), static_cast<int16_t>(lo4), static_cast<int16_t>(lo5), static_cast<int16_t>(lo6), static_cast<int16_t>(hi7))) { }

    // shuffle operations
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        const auto zero = _mm_setzero_si128();
        const auto lo = _mm_unpacklo_epi16(zero, this->Data), hi = _mm_unpackhi_epi16(zero, this->Data);
        const uint32_t lobit = _mm_movemask_ps(_mm_castsi128_ps(lo)), hibit = _mm_movemask_ps(_mm_castsi128_ps(hi));
        return (hibit << 4) | lobit; // no valid LEA anyway
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm_shuffle_epi8(this->Data, mask);
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const noexcept
    {
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1),
            static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1), static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1),
            static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1), static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1),
            static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1), static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1),
            static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm_shuffle_epi8(this->Data, mask);
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
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
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm_set_epi64x(0x0e0f0c0d0a0b0809ULL, 0x0607040502030001ULL);
        return _mm_shuffle_epi8(this->Data, SwapMask);
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept { return _mm_unpacklo_epi16(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const noexcept { return _mm_unpackhi_epi16(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0b10101010>(_mm_slli_epi32(other.Data, 16)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0b01010101>(_mm_srli_epi32(this->Data, 16)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr(Msk != MaskType::FullEle)
            mask = _mm_srai_epi16(mask, 16); // make sure all bits are covered
        return _mm_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        return _mm_blend_epi16(this->Data, other.Data, Mask);
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm_add_epi16(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm_sub_epi16(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept 
    {
        if constexpr (Enforce) return _mm_mullo_epi16_force(this->Data, other.Data);
        else return _mm_mullo_epi16(this->Data, other.Data);
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = MulLo(other); return *static_cast<T*>(this); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm_sll_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm_srl_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srl_epi16(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm_sra_epi16(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return _mm_sllv_epi16(this->Data, bits.Data);
#elif COMMON_SIMD_LV >= 200
        const auto loMask = _mm_set1_epi32(0x0000ffff);
        const auto bitsLo = _mm_and_si128(bits.Data, loMask);
        const auto lo = _mm_sllv_epi32(this->Data, bitsLo);
        const auto dataHi = _mm_andnot_si128(loMask, this->Data);
        const auto bitsHi = _mm_srli_epi32(bits.Data, 16);
        const auto hi = _mm_sllv_epi32(dataHi, bitsHi);
        return _mm_blend_epi16(lo, hi, 0xaa);
#else
        const auto shifterLo = _mm_setr_epi8(1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, static_cast<char>(1 << 7),
            0, 0, 0, 0, 0, 0, 0, 0);
        const auto shifterHi = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0,
            1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, static_cast<char>(1 << 7));
        const auto loMask = _mm_set1_epi16(0x00ff);
        auto shiftBits = bits.Data;
        if constexpr (MayOverflow)
        {
            shiftBits = _mm_min_epu16(shiftBits, loMask);
        }
        const auto shiftLo = _mm_shuffle_epi8(shifterLo, shiftBits);
        const auto shiftHi = _mm_slli_epi16(_mm_shuffle_epi8(shifterHi, shiftBits), 8);
        const auto shifter = _mm_blendv_epi8(shiftHi, shiftLo, loMask);
        return this->MulLo(shifter);
#endif
    }
    template<bool MayOverflow = true>
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return _mm_srlv_epi16(this->Data, bits.Data);
#elif COMMON_SIMD_LV >= 200
        const auto loMask = _mm_set1_epi32(0x0000ffff);
        const auto dataLo = _mm_and_si128(loMask, this->Data);
        const auto bitsLo = _mm_and_si128(bits.Data, loMask);
        const auto lo = _mm_srlv_epi32(dataLo, bitsLo);
        const auto bitsHi = _mm_srli_epi32(bits.Data, 16);
        const auto hi = _mm_srlv_epi32(this->Data, bitsHi);
        return _mm_blend_epi16(lo, hi, 0xaa);
#else
        const auto shifterHi = _mm_setr_epi8(0, static_cast<char>(1 << 7), 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2, 1 << 1,
            1 << 0, 0, 0, 0, 0, 0, 0, 0);
        const auto shifterLo = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0,
            0, static_cast<char>(1 << 7), 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2, 1 << 1);
        const auto loMask = _mm_set1_epi16(0x00ff);
        auto shiftBits = bits.Data;
        if constexpr (MayOverflow)
        {
            shiftBits = _mm_min_epu16(shiftBits, loMask);
        }
        const auto shiftLo = _mm_shuffle_epi8(shifterLo, shiftBits);
        const auto shiftHi = _mm_slli_epi16(_mm_shuffle_epi8(shifterHi, shiftBits), 8);
        const auto shifter = _mm_blendv_epi8(shiftHi, shiftLo, loMask);
        const auto res = _mm_mulhi_epu16(this->Data, shifter);
        const auto keepMask = _mm_cmpeq_epi16(bits.Data, _mm_setzero_si128());
        return _mm_blendv_epi8(res, this->Data, keepMask);
#endif
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept 
    {
        if constexpr (N >= 16)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_slli_epi16(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N >= 16)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm_srli_epi16(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm_srli_epi16(this->Data, N);
        else
            return _mm_srai_epi16(this->Data, N);
    }

    forceinline static T VECCALL LoadLo(const E val) noexcept { return _mm_loadu_si16_correct(&val); }
    forceinline static T VECCALL LoadLo(const E* ptr) noexcept { return _mm_loadu_si16_correct(ptr); }
};


template<typename T, typename E>
struct alignas(16) Common8x16 : public SSE128Common<T, E, 16>
{
private:
    using Base = SSE128Common<T, E, 16>;
public:
    using Base::Base;
    forceinline Common8x16(const E val) noexcept : Base(_mm_set1_epi8(val)) { }
    forceinline Common8x16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7, const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15) noexcept :
        Base(_mm_setr_epi8(static_cast<int8_t>(lo0), static_cast<int8_t>(lo1), static_cast<int8_t>(lo2), static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4), static_cast<int8_t>(lo5), static_cast<int8_t>(lo6), static_cast<int8_t>(lo7), static_cast<int8_t>(lo8),
            static_cast<int8_t>(lo9), static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), static_cast<int8_t>(lo12), static_cast<int8_t>(lo13),
            static_cast<int8_t>(lo14), static_cast<int8_t>(hi15))) { }

    // shuffle operations
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        return _mm_movemask_epi8(this->Data);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm_shuffle_epi8(this->Data, mask);
    }
    forceinline T VECCALL Shuffle(const U8x16& pos) const noexcept;
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Lo7,
        const uint8_t Lo8, const uint8_t Lo9, const uint8_t Lo10, const uint8_t Lo11, const uint8_t Lo12, const uint8_t Lo13, const uint8_t Lo14, const uint8_t Hi15) const noexcept
    {
        const auto mask = _mm_setr_epi8(static_cast<int8_t>(Lo0), static_cast<int8_t>(Lo1), static_cast<int8_t>(Lo2), static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4), static_cast<int8_t>(Lo5), static_cast<int8_t>(Lo6), static_cast<int8_t>(Lo7), static_cast<int8_t>(Lo8),
            static_cast<int8_t>(Lo9), static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11), static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13),
            static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return Shuffle(mask);
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
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
    forceinline T VECCALL ZipLo(const T& other) const noexcept { return _mm_unpacklo_epi8(this->Data, other.Data); }
    forceinline T VECCALL ZipHi(const T& other) const noexcept { return _mm_unpackhi_epi8(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0xaaaa>(_mm_slli_epi16(other.Data, 8)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0x5555>(_mm_srli_epi16(this->Data, 8)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept { return _mm_blendv_epi8(this->Data, other.Data, mask); }
    template<uint16_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        if constexpr (Mask == 0)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0xffff)
            return other;
        else
        {
#if COMMON_SIMD_LV >= 320
            return _mm_mask_blend_epi8(Mask, this->Data, other.Data);
#elif defined(CMSIMD_LESS_SPACE)
            const auto mask = _mm_insert_epi64(_mm_loadu_si64(&::common::simd::detail::FullMask64[Mask & 0xff]), static_cast<int64_t>(::common::simd::detail::FullMask64[(Mask >> 8) & 0xff]), 1);
            return _mm_blendv_epi8(this->Data, other.Data, mask);
#else
            alignas(__m128i) static constexpr uint64_t mask[2] = { ::common::simd::detail::FullMask64[Mask & 0xff], ::common::simd::detail::FullMask64[(Mask >> 8) & 0xff] };
            return _mm_blendv_epi8(this->Data, other.Data, _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask)));
#endif
        }
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm_add_epi8(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm_sub_epi8(this->Data, other.Data); }
    forceinline T VECCALL operator*(const T& other) const noexcept { return static_cast<T*>(this)->MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = operator*(other); return *static_cast<T*>(this); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const noexcept
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
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
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
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept;
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic(bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const noexcept
    {
        if constexpr (N >= 8)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff << N));
            const auto shift16 = _mm_slli_epi16(this->Data, N);
            return _mm_and_si128(shift16, mask);
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N >= 8)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
        {
            const auto mask = _mm_set1_epi8(static_cast<uint8_t>(0xff >> N));
            const auto shift16 = _mm_srli_epi16(this->Data, N);
            return _mm_and_si128(shift16, mask);
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept;

    forceinline static T VECCALL LoadLo(const E val) noexcept
    {
        const auto tmp = static_cast<uint16_t>(val); 
        return _mm_loadu_si16_correct(&tmp);
    }
    forceinline static T VECCALL LoadLo(const E* ptr) noexcept { return LoadLo(*ptr); }
};

}


struct alignas(16) F64x2 : public detail::SSE128Shared<F64x2, float>
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
    forceinline constexpr F64x2() noexcept : Data() { }
    forceinline explicit F64x2(const double* ptr) noexcept : Data(_mm_loadu_pd(ptr)) { }
    forceinline constexpr F64x2(const __m128d val) noexcept : Data(val) { }
    forceinline F64x2(const double val) noexcept : Data(_mm_set1_pd(val)) { }
    forceinline F64x2(const double lo, const double hi) noexcept : Data(_mm_setr_pd(lo, hi)) { }
    forceinline constexpr operator const __m128d&() const noexcept { return Data; }
    forceinline void VECCALL Load(const double *ptr) noexcept { Data = _mm_loadu_pd(ptr); }
    forceinline void VECCALL Save(double *ptr) const noexcept { _mm_storeu_pd(ptr, Data); }

    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        const auto signbits = ExtractSignBit();
        return detail::SignBitToIdx<Count>(signbits);
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        return _mm_movemask_pd(this->Data);
    }
    template<uint8_t Lo, uint8_t Hi>
    forceinline F64x2 VECCALL Shuffle() const noexcept
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_pd(Data, (Hi << 1) + Lo);
#else
        return _mm_shuffle_pd(Data, Data, (Hi << 1) + Lo);
#endif
    }
    forceinline F64x2 VECCALL Shuffle(const uint8_t Lo, const uint8_t Hi) const noexcept
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
    forceinline F64x2 VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return Shuffle<Idx, Idx>();
    }
    forceinline F64x2 VECCALL ZipLo(const F64x2& other) const noexcept { return _mm_unpacklo_pd(Data, other.Data); }
    forceinline F64x2 VECCALL ZipHi(const F64x2& other) const noexcept { return _mm_unpackhi_pd(Data, other.Data); }
    forceinline F64x2 VECCALL ZipOdd(const F64x2& other) const noexcept { return ZipLo(other); }
    forceinline F64x2 VECCALL ZipEven(const F64x2& other) const noexcept { return ZipHi(other); }
    template<MaskType Msk>
    forceinline F64x2 VECCALL SelectWith(const F64x2& other, F64x2 mask) const noexcept
    {
        return _mm_blendv_pd(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F64x2 VECCALL SelectWith(const F64x2& other) const noexcept
    {
        static_assert(Mask <= 0b11, "Only allow 2 bits");
        return _mm_blend_pd(this->Data, other.Data, Mask);
    }
    template<size_t Cnt>
    forceinline F64x2 VECCALL MoveToHi() const noexcept
    {
        static_assert(Cnt <= 2, "move count  should be in [0,2]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 2) return AllZero();
        else return _mm_castsi128_pd(_mm_slli_si128(_mm_castpd_si128(Data), Cnt * 8));
    }
    template<size_t Cnt>
    forceinline F64x2 VECCALL MoveToLo() const noexcept
    {
        static_assert(Cnt <= 2, "move count  should be in [0,2]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 2) return AllZero();
        else return _mm_castsi128_pd(_mm_srli_si128(_mm_castpd_si128(Data), Cnt * 8));
    }
    template<uint8_t Cnt>
    forceinline F64x2 VECCALL MoveToLoWith(const F64x2& hi) const noexcept
    {
        static_assert(Cnt <= 2, "move count should be in [0,2]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 2) return hi;
        else return _mm_shuffle_pd(Data, hi.Data, 0b01);
    }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F64x2 VECCALL Compare(const F64x2 other) const noexcept
    {
#if COMMON_SIMD_LV >= 100
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm_cmp_pd(Data, other.Data, cmpImm);
#else
             if constexpr (Cmp == CompareType::LessThan)     return _mm_cmplt_pd (Data, other.Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return _mm_cmple_pd (Data, other.Data);
        else if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_pd (Data, other.Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return _mm_cmpneq_pd(Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterEqual) return _mm_cmpge_pd (Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_pd (Data, other.Data);
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
#endif
    }

    // logic operations
    forceinline F64x2 VECCALL And(const F64x2& other) const noexcept
    {
        return _mm_and_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Or(const F64x2& other) const noexcept
    {
        return _mm_or_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Xor(const F64x2& other) const noexcept
    {
        return _mm_xor_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL AndNot(const F64x2& other) const noexcept
    {
        return _mm_andnot_pd(Data, other.Data);
    }
    forceinline F64x2 VECCALL Not() const
    {
        return _mm_xor_pd(Data, _mm_castsi128_pd(_mm_set1_epi64x(-1)));
    }

    // arithmetic operations
    forceinline F64x2 VECCALL Add(const F64x2& other) const noexcept { return _mm_add_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Sub(const F64x2& other) const noexcept { return _mm_sub_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Mul(const F64x2& other) const noexcept { return _mm_mul_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Div(const F64x2& other) const noexcept { return _mm_div_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Neg() const noexcept { return Xor(_mm_castsi128_pd(_mm_set1_epi64x(INT64_MIN))); }
    forceinline F64x2 VECCALL Abs() const noexcept { return And(_mm_castsi128_pd(_mm_set1_epi64x(INT64_MAX))); }
    forceinline F64x2 VECCALL Max(const F64x2& other) const noexcept { return _mm_max_pd(Data, other.Data); }
    forceinline F64x2 VECCALL Min(const F64x2& other) const noexcept { return _mm_min_pd(Data, other.Data); }
    //F64x2 Rcp() const { return _mm_rcp_pd(Data); }
    forceinline F64x2 VECCALL Sqrt() const noexcept { return _mm_sqrt_pd(Data); }
    //F64x2 Rsqrt() const { return _mm_rsqrt_pd(Data); }
    forceinline F64x2 VECCALL MulAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_add_pd(_mm_mul_pd(Data, muler.Data), adder.Data);
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL MulScalarAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x2 VECCALL MulSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_sub_pd(_mm_mul_pd(Data, muler.Data), suber.Data);
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL MulScalarSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F64x2 VECCALL NMulAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_pd(Data, muler.Data, adder.Data);
#else
        return _mm_sub_pd(adder.Data, _mm_mul_pd(Data, muler.Data));
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulScalarAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x2 VECCALL NMulSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_pd(Data, muler.Data, suber.Data);
#else
        return _mm_xor_pd(_mm_add_pd(_mm_mul_pd(Data, muler.Data), suber.Data), _mm_set1_pd(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulScalarSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    forceinline F64x2 VECCALL operator*(const F64x2& other) const noexcept { return Mul(other); }
    forceinline F64x2 VECCALL operator/(const F64x2& other) const noexcept { return Div(other); }
    forceinline F64x2& VECCALL operator*=(const F64x2& other) noexcept { Data = Mul(other); return *this; }
    forceinline F64x2& VECCALL operator/=(const F64x2& other) noexcept { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F64x2, T>(), typename... Args>
    typename CastTyper<F64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F64x2 VECCALL Round() const noexcept
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm_round_pd(Data, imm8);
    }

    forceinline static F64x2 VECCALL AllZero() noexcept { return _mm_setzero_pd(); }
    forceinline static F64x2 VECCALL LoadLo(const double val) noexcept { return _mm_load_sd(&val); }
    forceinline static F64x2 VECCALL LoadLo(const double* ptr) noexcept { return _mm_load_sd(ptr); }
};


struct alignas(16) F32x4 : public detail::SSE128Shared<F32x4, float>
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
    forceinline constexpr F32x4() noexcept : Data() { }
    forceinline explicit F32x4(const float* ptr) noexcept : Data(_mm_loadu_ps(ptr)) { }
    forceinline constexpr F32x4(const __m128 val) noexcept : Data(val) { }
    forceinline F32x4(const float val) noexcept : Data(_mm_set1_ps(val)) { }
    forceinline F32x4(const float lo0, const float lo1, const float lo2, const float hi3) noexcept : Data(_mm_setr_ps(lo0, lo1, lo2, hi3)) { }
    forceinline constexpr operator const __m128&() const noexcept { return Data; }
    forceinline void VECCALL Load(const float *ptr) noexcept { Data = _mm_loadu_ps(ptr); }
    forceinline void VECCALL Save(float *ptr) const noexcept { _mm_storeu_ps(ptr, Data); }

    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        const auto signbits = ExtractSignBit();
        return detail::SignBitToIdx<Count>(signbits);
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        return _mm_movemask_ps(this->Data);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline F32x4 VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
#if COMMON_SIMD_LV >= 100
        return _mm_permute_ps(Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#else
        return _mm_shuffle_ps(Data, Data, _MM_SHUFFLE(Hi3, Lo2, Lo1, Lo0));
#endif
    }
    forceinline F32x4 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const noexcept
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
    forceinline F32x4 VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return Shuffle<Idx, Idx, Idx, Idx>();
    }
    forceinline F32x4 VECCALL ZipLo(const F32x4& other) const noexcept { return _mm_unpacklo_ps(Data, other.Data); }
    forceinline F32x4 VECCALL ZipHi(const F32x4& other) const noexcept { return _mm_unpackhi_ps(Data, other.Data); }
    forceinline F32x4 VECCALL ZipOdd(const F32x4& other) const noexcept
    {
        return SelectWith<0b1010>(_mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(other.Data), 32)));
    }
    forceinline F32x4 VECCALL ZipEven(const F32x4& other) const noexcept
    { 
        return other.template SelectWith<0b0101>(_mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(Data), 32)));
    }
    template<MaskType Msk>
    forceinline F32x4 VECCALL SelectWith(const F32x4& other, F32x4 mask) const noexcept
    {
        return _mm_blendv_ps(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F32x4 VECCALL SelectWith(const F32x4& other) const noexcept
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
        return _mm_blend_ps(this->Data, other.Data, Mask);
    }
    template<uint8_t Cnt>
    forceinline F32x4 VECCALL MoveToHi() const noexcept
    {
        static_assert(Cnt <= 4, "move count  should be in [0,4]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 4) return AllZero();
        else return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(Data), Cnt * 4));
    }
    template<uint8_t Cnt>
    forceinline F32x4 VECCALL MoveToLo() const noexcept
    {
        static_assert(Cnt <= 4, "move count  should be in [0,4]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 4) return AllZero();
        else return _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(Data), Cnt * 4));
    }
    template<uint8_t Cnt>
    forceinline F32x4 VECCALL MoveToLoWith(const F32x4& hi) const noexcept
    {
        static_assert(Cnt <= 4, "move count should be in [0,4]");
        if constexpr (Cnt == 0) return *this;
        else if constexpr (Cnt == 4) return hi;
        else if constexpr (Cnt == 2) return _mm_shuffle_ps(Data, hi.Data, 0b01001110);
        else return _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(hi.Data), _mm_castps_si128(Data), Cnt * 4));
    }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F32x4 VECCALL Compare(const F32x4 other) const noexcept
    {
#if COMMON_SIMD_LV >= 100
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm_cmp_ps(Data, other.Data, cmpImm);
#else
             if constexpr (Cmp == CompareType::LessThan)     return _mm_cmplt_ps (Data, other.Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return _mm_cmple_ps (Data, other.Data);
        else if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_ps (Data, other.Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return _mm_cmpneq_ps(Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterEqual) return _mm_cmpge_ps (Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_ps (Data, other.Data);
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
#endif
    }

    // logic operations
    forceinline F32x4 VECCALL And(const F32x4& other) const noexcept
    {
        return _mm_and_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL Or(const F32x4& other) const noexcept
    {
        return _mm_or_ps(Data, other.Data);
    }
    forceinline F32x4 Xor(const F32x4& other) const noexcept
    {
        return _mm_xor_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL AndNot(const F32x4& other) const noexcept
    {
        return _mm_andnot_ps(Data, other.Data);
    }
    forceinline F32x4 VECCALL Not() const noexcept
    {
        return _mm_xor_ps(Data, _mm_castsi128_ps(_mm_set1_epi32(-1)));
    }

    // arithmetic operations
    forceinline F32x4 VECCALL Add(const F32x4& other) const noexcept { return _mm_add_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Sub(const F32x4& other) const noexcept { return _mm_sub_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Mul(const F32x4& other) const noexcept { return _mm_mul_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Div(const F32x4& other) const noexcept { return _mm_div_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Neg() const noexcept { return Xor(_mm_castsi128_ps(_mm_set1_epi32(INT32_MIN))); }
    forceinline F32x4 VECCALL Abs() const noexcept { return And(_mm_castsi128_ps(_mm_set1_epi32(INT32_MAX))); }
    forceinline F32x4 VECCALL Max(const F32x4& other) const noexcept { return _mm_max_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Min(const F32x4& other) const noexcept { return _mm_min_ps(Data, other.Data); }
    forceinline F32x4 VECCALL Rcp() const noexcept { return _mm_rcp_ps(Data); }
    forceinline F32x4 VECCALL Sqrt() const noexcept { return _mm_sqrt_ps(Data); }
    forceinline F32x4 VECCALL Rsqrt() const noexcept { return _mm_rsqrt_ps(Data); }
    forceinline F32x4 VECCALL MulAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_add_ps(_mm_mul_ps(Data, muler.Data), adder.Data);
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL MulScalarAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x4 VECCALL MulSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_sub_ps(_mm_mul_ps(Data, muler.Data), suber.Data);
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL MulScalarSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F32x4 VECCALL NMulAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmadd_ps(Data, muler.Data, adder.Data);
#else
        return _mm_sub_ps(adder.Data, _mm_mul_ps(Data, muler.Data));
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulScalarAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x4 VECCALL NMulSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm_fnmsub_ps(Data, muler.Data, suber.Data);
#else
        return _mm_xor_ps(_mm_add_ps(_mm_mul_ps(Data, muler.Data), suber.Data), _mm_set1_ps(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulScalarSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    template<DotPos Mul, DotPos Res>
    forceinline F32x4 VECCALL Dot(const F32x4& other) const noexcept
    { 
        return _mm_dp_ps(Data, other.Data, static_cast<uint8_t>(Mul) << 4 | static_cast<uint8_t>(Res));
    }
    template<DotPos Mul>
    forceinline float VECCALL Dot(const F32x4& other) const noexcept
    {
        return _mm_cvtss_f32(Dot<Mul, DotPos::XYZW>(other).Data);
    }

    forceinline F32x4 VECCALL operator*(const F32x4& other) const noexcept { return Mul(other); }
    forceinline F32x4 VECCALL operator/(const F32x4& other) const noexcept { return Div(other); }
    forceinline F32x4& VECCALL operator*=(const F32x4& other) noexcept { Data = Mul(other); return *this; }
    forceinline F32x4& VECCALL operator/=(const F32x4& other) noexcept { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F32x4, T>(), typename... Args>
    typename CastTyper<F32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F32x4 VECCALL Round() const noexcept
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm_round_ps(Data, imm8);
    }

    forceinline static F32x4 VECCALL AllZero() noexcept { return _mm_setzero_ps(); }
    forceinline static F32x4 VECCALL LoadLo(const float val) noexcept { return _mm_load_ss(&val); }
    forceinline static F32x4 VECCALL LoadLo(const float* ptr) noexcept { return _mm_load_ss(ptr); }
};


struct alignas(16) F16x8
{
    using EleType = ::common::fp16_t;
    using VecType = __m128i;
    static constexpr size_t Count = 8;
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,16,8,0 };
    union
    {
        __m128i Data;
        ::common::fp16_t Val[8];
    };
    forceinline constexpr F16x8() noexcept : Data() {}
    forceinline explicit F16x8(const ::common::fp16_t* ptr) noexcept : Data(_mm_loadu_si128((const __m128i*)ptr)) {}
    forceinline constexpr F16x8(const __m128i val) noexcept : Data(val) {}
    forceinline F16x8(const ::common::fp16_t val) noexcept : Data(_mm_set1_epi16(::common::bit_cast<int16_t>(val))) {}
    forceinline F16x8(const ::common::fp16_t lo0, const ::common::fp16_t lo1, const ::common::fp16_t lo2, const ::common::fp16_t lo3, 
        const ::common::fp16_t lo4, const ::common::fp16_t lo5, const ::common::fp16_t lo6, const ::common::fp16_t hi7) noexcept : 
        Data(_mm_setr_epi16(::common::bit_cast<int16_t>(lo0), ::common::bit_cast<int16_t>(lo1), ::common::bit_cast<int16_t>(lo2), ::common::bit_cast<int16_t>(lo3), 
            ::common::bit_cast<int16_t>(lo4), ::common::bit_cast<int16_t>(lo5), ::common::bit_cast<int16_t>(lo6), ::common::bit_cast<int16_t>(hi7))) {}
    forceinline constexpr operator const __m128i& () const noexcept { return Data; }
    forceinline void VECCALL Load(const ::common::fp16_t* ptr) noexcept { Data = _mm_loadu_si128((const __m128i*)ptr); }
    forceinline void VECCALL Save(::common::fp16_t* ptr) const noexcept { _mm_storeu_si128((__m128i*)ptr, Data); }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F16x8, T>(), typename... Args>
    typename CastTyper<F16x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};


struct alignas(16) I64x2 : public detail::Common64x2<I64x2, int64_t>
{
    using Common64x2<I64x2, int64_t>::Common64x2;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I64x2 VECCALL Compare(const I64x2 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi64(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi64(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I64x2 VECCALL Neg() const noexcept { return _mm_sub_epi64(_mm_setzero_si128(), Data); }
    forceinline I64x2 VECCALL SatAdd(const I64x2& other) const noexcept
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
    forceinline I64x2 VECCALL SatSub(const I64x2& other) const noexcept
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
    forceinline I64x2 VECCALL Max(const I64x2& other) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return _mm_max_epi64(Data, other.Data);
#else
        return _mm_blendv_epi8(other.Data, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
#endif
    }
    forceinline I64x2 VECCALL Min(const I64x2& other) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return _mm_min_epi64(Data, other.Data);
#else
        return _mm_blendv_epi8(Data, other.Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
#endif
    }
    forceinline I64x2 VECCALL Abs() const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return _mm_abs_epi64(Data);
#else
        return SelectWith<MaskType::SigBit>(Neg(), *this);
#endif
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I64x2, T>(), typename... Args>
    forceinline typename CastTyper<I64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};


struct alignas(16) U64x2 : public detail::Common64x2<U64x2, uint64_t>
{
    using Common64x2<U64x2, uint64_t>::Common64x2;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U64x2 VECCALL Compare(const U64x2 other) const noexcept
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
    forceinline U64x2 VECCALL SatAdd(const U64x2& other) const noexcept
    {
        //// sig: 1|1 return 1, 0|0 return add, 1|0/0|1 use add
        //// add: 1 return add, 0 return 1
        //const auto sig = _mm_castsi128_pd(Xor(other));
        //const auto add = _mm_castsi128_pd(Add(other));
        //const auto allones = _mm_castsi128_pd(_mm_set1_epi8(-1));
        //const auto notAdd = _mm_xor_pd(add, allones);
        //const auto mask = _mm_blendv_pd(_mm_castsi128_pd(other.Data), notAdd, sig);
        //return _mm_castpd_si128(_mm_blendv_pd(add, allones, mask));
        return Min(other.Not()).Add(other);
    }
    forceinline U64x2 VECCALL SatSub(const U64x2& other) const noexcept
    {
#if COMMON_SIMD_LV >= 320
        return Max(other).Sub(other);
#else
        return Sub(other).And(Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
#endif
    }
    forceinline U64x2 VECCALL Max(const U64x2& other) const noexcept
    { 
#if COMMON_SIMD_LV >= 320
        return _mm_max_epu64(Data, other.Data);
#else
        return _mm_blendv_epi8(other.Data, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
#endif
    }
    forceinline U64x2 VECCALL Min(const U64x2& other) const noexcept
    { 
#if COMMON_SIMD_LV >= 320
        return _mm_min_epu64(Data, other.Data);
#else
        return _mm_blendv_epi8(Data, other.Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
#endif
    }
    forceinline U64x2 VECCALL Abs() const noexcept { return Data; }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U64x2, T>(), typename... Args>
    forceinline typename CastTyper<U64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};


struct alignas(16) I32x4 : public detail::Common32x4<I32x4, int32_t>
{
    using Common32x4<I32x4, int32_t>::Common32x4;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I32x4 VECCALL Compare(const I32x4 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi32(Data, other);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi32(Data, other);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I32x4 VECCALL SatAdd(const I32x4& other) const noexcept 
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
    forceinline I32x4 VECCALL SatSub(const I32x4& other) const noexcept
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
    forceinline I32x4 VECCALL Neg() const noexcept { return _mm_sub_epi32(_mm_setzero_si128(), Data); }
    forceinline I32x4 VECCALL Max(const I32x4& other) const noexcept { return _mm_max_epi32(Data, other.Data); }
    forceinline I32x4 VECCALL Min(const I32x4& other) const noexcept { return _mm_min_epi32(Data, other.Data); }
    forceinline Pack<I64x2, 2> VECCALL MulX(const I32x4& other) const noexcept
    {
        const I64x2 even = _mm_mul_epi32(Data, other.Data);
        const I64x2 odd  = _mm_mul_epi32(As<I64x2>().ShiftRightLogic<32>(), other.As<I64x2>().ShiftRightLogic<32>());
        return { even.ZipLo(odd), even.ZipHi(odd) };
    }
    forceinline I32x4 VECCALL Abs() const noexcept { return _mm_abs_epi32(Data); }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I32x4, T>(), typename... Args>
    forceinline typename CastTyper<I32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I64x2, 2> VECCALL I32x4::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return { _mm_cvtepi32_epi64(Data), _mm_cvtepi32_epi64(MoveHiToLo()) };
}
template<> forceinline Pack<U64x2, 2> VECCALL I32x4::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline F32x4 VECCALL I32x4::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return _mm_cvtepi32_ps(Data);
}
template<> forceinline Pack<F64x2, 2> VECCALL I32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return { _mm_cvtepi32_pd(Data), _mm_cvtepi32_pd(MoveHiToLo()) };
}


struct alignas(16) U32x4 : public detail::Common32x4<U32x4, uint32_t>
{
    using Common32x4<U32x4, uint32_t>::Common32x4;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U32x4 VECCALL Compare(const U32x4 other) const noexcept
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
    forceinline U32x4 VECCALL SatAdd(const U32x4& other) const noexcept
    {
        return Min(other.Not()).Add(other);
    }
    forceinline U32x4 VECCALL SatSub(const U32x4& other) const noexcept
    {
        return Max(other).Sub(other);
    }
    forceinline U32x4 VECCALL Max(const U32x4& other) const noexcept { return _mm_max_epu32(Data, other.Data); }
    forceinline U32x4 VECCALL Min(const U32x4& other) const noexcept { return _mm_min_epu32(Data, other.Data); }
    forceinline U32x4 VECCALL Abs() const { return Data; }
    forceinline Pack<U64x2, 2> VECCALL MulX(const U32x4& other) const noexcept
    {
        const U64x2 even = _mm_mul_epu32(Data, other.Data);
        const U64x2 odd  = _mm_mul_epu32(As<U64x2>().ShiftRightLogic<32>(), other.As<U64x2>().ShiftRightLogic<32>());
        return { even.ZipLo(odd), even.ZipHi(odd) };
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U32x4, T>(), typename... Args>
    forceinline typename CastTyper<U32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I64x2, 2> VECCALL U32x4::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi32(Data, zero), _mm_unpackhi_epi32(Data, zero) };
}
template<> forceinline Pack<U64x2, 2> VECCALL U32x4::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline F32x4 VECCALL U32x4::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
# if COMMON_SIMD_LV >= 320
    return _mm512_castps512_ps128(_mm512_cvtepu32_ps(_mm512_castsi128_si512(Data)));
# else
    // use 2 convert to make sure error within 0.5 ULP, see https://stackoverflow.com/a/40766669
    const auto mul16 = _mm_set1_ps(65536.f);
    const auto lo16 = And(0xffff);
    const auto hi16 = ShiftRightLogic<16>();
    const auto base = hi16.As<I32x4>().Cast<F32x4>();
    const auto addlo = lo16.As<I32x4>().Cast<F32x4>();
    return base.MulAdd(mul16, addlo);
#endif
}
template<> forceinline Pack<F64x2, 2> VECCALL U32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 320
    return { _mm_cvtepu32_pd(Data), _mm_cvtepu32_pd(MoveHiToLo()) };
#else
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
    forceinline I16x8 VECCALL Compare(const I16x8 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi16(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi16(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I16x8 VECCALL SatAdd(const I16x8& other) const noexcept { return _mm_adds_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL SatSub(const I16x8& other) const noexcept { return _mm_subs_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL MulHi(const I16x8& other) const noexcept { return _mm_mulhi_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL Neg() const noexcept { return _mm_sub_epi16(_mm_setzero_si128(), Data); }
    forceinline I16x8 VECCALL Max(const I16x8& other) const noexcept { return _mm_max_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL Min(const I16x8& other) const noexcept { return _mm_min_epi16(Data, other.Data); }
    forceinline I16x8 VECCALL Abs() const noexcept { return _mm_abs_epi16(Data); }
    forceinline Pack<I32x4, 2> MulX(const I16x8& other) const noexcept
    {
        const auto los = MulLo(other), his = MulHi(other);
        return { _mm_unpacklo_epi16(los, his),_mm_unpackhi_epi16(los, his) };
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I16x8, T>(), typename... Args>
    forceinline typename CastTyper<I16x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I32x4, 2> VECCALL I16x8::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return { _mm_cvtepi16_epi32(Data), _mm_cvtepi16_epi32(MoveHiToLo()) };
}
template<> forceinline Pack<U32x4, 2> VECCALL I16x8::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL I16x8::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    const auto ret0 = _mm_cvtepi16_epi64(Data);
    const auto ret1 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi16_epi64(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U64x2, 4> VECCALL I16x8::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL I16x8::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 4> VECCALL I16x8::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F64x2>();
}


struct alignas(16) U16x8 : public detail::Common16x8<U16x8, uint16_t>
{
    using Common16x8<U16x8, uint16_t>::Common16x8;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U16x8 VECCALL Compare(const U16x8 other) const noexcept
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
    forceinline U16x8 VECCALL SatAdd(const U16x8& other) const noexcept { return _mm_adds_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL SatSub(const U16x8& other) const noexcept { return _mm_subs_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL MulHi(const U16x8& other) const noexcept { return _mm_mulhi_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL Max(const U16x8& other) const noexcept { return _mm_max_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL Min(const U16x8& other) const noexcept { return _mm_min_epu16(Data, other.Data); }
    forceinline U16x8 VECCALL Abs() const noexcept { return Data; }
    forceinline Pack<U32x4, 2> VECCALL MulX(const U16x8& other) const noexcept
    {
        const auto los = MulLo(other), his = MulHi(other);
        return { _mm_unpacklo_epi16(los, his), _mm_unpackhi_epi16(los, his) };
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U16x8, T>(), typename... Args>
    forceinline typename CastTyper<U16x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I32x4, 2> VECCALL U16x8::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi16(Data, zero), _mm_unpackhi_epi16(Data, zero) };
}
template<> forceinline Pack<U32x4, 2> VECCALL U16x8::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL U16x8::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi16(Data, zero), hi = _mm_unpackhi_epi16(Data, zero);
    return { _mm_unpacklo_epi32(lo, zero), _mm_unpackhi_epi32(lo, zero), _mm_unpacklo_epi32(hi, zero), _mm_unpackhi_epi32(hi, zero) };
}
template<> forceinline Pack<U64x2, 4> VECCALL U16x8::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL U16x8::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 4> VECCALL U16x8::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F64x2>();
}


struct alignas(16) I8x16 : public detail::Common8x16<I8x16, int8_t>
{
    using Common8x16<I8x16, int8_t>::Common8x16;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I8x16 VECCALL Compare(const I8x16 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm_cmpeq_epi8(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm_cmpgt_epi8(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I8x16 VECCALL SatAdd(const I8x16& other) const noexcept { return _mm_adds_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL SatSub(const I8x16& other) const noexcept { return _mm_subs_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL Neg() const noexcept { return _mm_sub_epi8(_mm_setzero_si128(), Data); }
    forceinline I8x16 VECCALL Max(const I8x16& other) const noexcept { return _mm_max_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL Min(const I8x16& other) const noexcept { return _mm_min_epi8(Data, other.Data); }
    forceinline I8x16 VECCALL Abs() const noexcept { return _mm_abs_epi8(Data); }
    forceinline I8x16 VECCALL MulHi(const I8x16& other) const noexcept
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightLogic<8>(), hi = full[1].ShiftRightLogic<8>();
        return _mm_packus_epi16(lo, hi);
    }
    forceinline I8x16 VECCALL MulLo(const I8x16& other) const noexcept
    { 
        const auto full = MulX(other);
        const I16x8 mask(0x00ff);
        const auto lo = full[0].And(mask), hi = full[1].And(mask);
        return _mm_packus_epi16(lo, hi);
    }
    forceinline Pack<I16x8, 2> VECCALL MulX(const I8x16& other) const noexcept;
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I8x16, T>(), typename... Args>
    forceinline typename CastTyper<I8x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I16x8, 2> VECCALL I8x16::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    return { _mm_cvtepi8_epi16(Data), _mm_cvtepi8_epi16(MoveHiToLo()) };
}
template<> forceinline Pack<U16x8, 2> VECCALL I8x16::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x8>().As<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL I8x16::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    const auto ret0 = _mm_cvtepi8_epi32(Data);
    const auto ret1 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepi8_epi32(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };
}
template<> forceinline Pack<U32x4, 4> VECCALL I8x16::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL I8x16::Cast<I64x2, CastMode::RangeUndef>() const noexcept
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
template<> forceinline Pack<U64x2, 8> VECCALL I8x16::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
forceinline Pack<I16x8, 2> VECCALL I8x16::MulX(const I8x16& other) const noexcept
{
    const auto self16 = Cast<I16x8>(), other16 = other.Cast<I16x8, CastMode::RangeUndef>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
template<> forceinline Pack<F32x4, 4> VECCALL I8x16::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 8> VECCALL I8x16::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F64x2>();
}


struct alignas(16) U8x16 : public detail::Common8x16<U8x16, uint8_t>
{
    using Common8x16<U8x16, uint8_t>::Common8x16;

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U8x16 VECCALL Compare(const U8x16 other) const noexcept
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
    forceinline U8x16 VECCALL SatAdd(const U8x16& other) const noexcept { return _mm_adds_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL SatSub(const U8x16& other) const noexcept { return _mm_subs_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Max(const U8x16& other) const noexcept { return _mm_max_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Min(const U8x16& other) const noexcept { return _mm_min_epu8(Data, other.Data); }
    forceinline U8x16 VECCALL Abs() const noexcept { return Data; }
    forceinline U8x16 VECCALL MulHi(const U8x16& other) const noexcept
    {
        const auto full = MulX(other);
        return _mm_packus_epi16(full[0].ShiftRightLogic<8>(), full[1].ShiftRightLogic<8>());
    }
    forceinline U8x16 VECCALL MulLo(const U8x16& other) const noexcept
    {
        const U16x8 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        const U16x8 mask(0x00ff);
        return U8x16(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    forceinline Pack<U16x8, 2> VECCALL MulX(const U8x16& other) const noexcept;
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U8x16, T>(), typename... Args>
    forceinline typename CastTyper<U8x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I16x8, 2> VECCALL U8x16::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    const auto zero = _mm_setzero_si128();
    return { _mm_unpacklo_epi8(Data, zero), _mm_unpackhi_epi8(Data, zero) };
}
template<> forceinline Pack<U16x8, 2> VECCALL U8x16::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x8>().Cast<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL U8x16::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    /*const auto ret0 = _mm_cvtepu8_epi32(Data);
    const auto ret1 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 4));
    const auto ret2 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 8));
    const auto ret3 = _mm_cvtepu8_epi32(_mm_srli_si128(Data, 12));
    return { ret0, ret1, ret2, ret3 };*/
    // not really slower
    const auto zero = _mm_setzero_si128();
    const auto lo = _mm_unpacklo_epi8(Data, zero), hi = _mm_unpackhi_epi8(Data, zero);
    return { _mm_unpacklo_epi8(lo, zero), _mm_unpackhi_epi8(lo, zero), _mm_unpacklo_epi8(hi, zero), _mm_unpackhi_epi8(hi, zero) };
}
template<> forceinline Pack<U32x4, 4> VECCALL U8x16::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL U8x16::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
//    const auto ret0 = _mm_cvtepu8_epi64(Data);
//    const auto ret1 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 2));
//    const auto ret2 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 4));
//    const auto ret3 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 6));
//    const auto ret4 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 8));
//    const auto ret5 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 10));
//    const auto ret6 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 12));
//    const auto ret7 = _mm_cvtepu8_epi64(_mm_srli_si128(Data, 14));
//    return { ret0, ret1, ret2, ret3, ret4, ret5, ret6, ret7 };
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
template<> forceinline Pack<U64x2, 8> VECCALL U8x16::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().Cast<U64x2>();
}
template<> forceinline Pack<F32x4, 4> VECCALL U8x16::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
template<> forceinline Pack<F64x2, 8> VECCALL U8x16::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F64x2>();
}
forceinline Pack<U16x8, 2> VECCALL U8x16::MulX(const U8x16& other) const noexcept
{
    const auto self16 = Cast<U16x8>(), other16 = other.Cast<U16x8>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}


template<typename T, typename E>
forceinline T VECCALL detail::Common8x16<T, E>::Shuffle(const U8x16& pos) const noexcept
{
    return _mm_shuffle_epi8(this->Data, pos);
}
template<typename T, typename E>
forceinline T VECCALL detail::Common8x16<T, E>::ShiftRightArith(const uint8_t bits) const noexcept
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
forceinline T VECCALL detail::Common8x16<T, E>::ShiftRightArith() const noexcept
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


template<> forceinline U64x2 VECCALL I64x2::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I64x2 VECCALL U64x2::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U32x4 VECCALL I32x4::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I32x4 VECCALL U32x4::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U16x8 VECCALL I16x8::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I16x8 VECCALL U16x8::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U8x16 VECCALL I8x16::Cast<U8x16, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I8x16 VECCALL U8x16::Cast<I8x16, CastMode::RangeUndef>() const noexcept
{
    return Data;
}


template<> forceinline U32x4 VECCALL U64x2::Cast<U32x4, CastMode::RangeTrunc>(const U64x2& arg1) const noexcept
{
    const auto lo =      As<U32x4>().Shuffle<0, 2, 0, 0>();
    const auto hi = arg1.As<U32x4>().Shuffle<0, 2, 0, 0>();
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeTrunc>(const U32x4& arg1) const noexcept
{
    const auto mask = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto lo = _mm_shuffle_epi8(Data, mask);
    const auto hi = _mm_shuffle_epi8(arg1, mask);
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeTrunc>(const U16x8& arg1) const noexcept
{
    const auto mask = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto lo = _mm_shuffle_epi8(Data, mask);
    const auto hi = _mm_shuffle_epi8(arg1, mask);
    return _mm_unpacklo_epi64(lo, hi);
}
template<> forceinline U16x8 VECCALL U64x2::Cast<U16x8, CastMode::RangeTrunc>(const U64x2& arg1, const U64x2& arg2, const U64x2& arg3) const noexcept
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
template<> forceinline U8x16 VECCALL U32x4::Cast<U8x16, CastMode::RangeTrunc>(const U32x4& arg1, const U32x4& arg2, const U32x4& arg3) const noexcept
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
    const U64x2& arg4, const U64x2& arg5, const U64x2& arg6, const U64x2& arg7) const noexcept
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
template<> forceinline I32x4 VECCALL I64x2::Cast<I32x4, CastMode::RangeTrunc>(const I64x2& arg1) const noexcept
{
    return As<U64x2>().Cast<U32x4>(arg1.As<U64x2>()).As<I32x4>();
}
template<> forceinline I16x8 VECCALL I32x4::Cast<I16x8, CastMode::RangeTrunc>(const I32x4& arg1) const noexcept
{
    return As<U32x4>().Cast<U16x8>(arg1.As<U32x4>()).As<I16x8>();
}
template<> forceinline I8x16 VECCALL I16x8::Cast<I8x16, CastMode::RangeTrunc>(const I16x8& arg1) const noexcept
{
    return As<U16x8>().Cast<U8x16>(arg1.As<U16x8>()).As<I8x16>();
}
template<> forceinline I16x8 VECCALL I64x2::Cast<I16x8, CastMode::RangeTrunc>(const I64x2& arg1, const I64x2& arg2, const I64x2& arg3) const noexcept
{
    return As<U64x2>().Cast<U16x8>(arg1.As<U64x2>(), arg2.As<U64x2>(), arg3.As<U64x2>()).As<I16x8>();
}
template<> forceinline I8x16 VECCALL I32x4::Cast<I8x16, CastMode::RangeTrunc>(const I32x4& arg1, const I32x4& arg2, const I32x4& arg3) const noexcept
{
    return As<U32x4>().Cast<U8x16>(arg1.As<U32x4>(), arg2.As<U32x4>(), arg3.As<U32x4>()).As<I8x16>();
}
template<> forceinline I8x16 VECCALL I64x2::Cast<I8x16, CastMode::RangeTrunc>(const I64x2& arg1, const I64x2& arg2, const I64x2& arg3,
    const I64x2& arg4, const I64x2& arg5, const I64x2& arg6, const I64x2& arg7) const noexcept
{
    return As<U64x2>().Cast<U8x16>(arg1.As<U64x2>(), arg2.As<U64x2>(), arg3.As<U64x2>(),
        arg4.As<U64x2>(), arg5.As<U64x2>(), arg6.As<U64x2>(), arg7.As<U64x2>()).As<I8x16>();
}


template<> forceinline Pack<F32x4, 2> VECCALL F16x8::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 100 && (defined(__F16C__) || COMMON_COMPILER_MSVC)
    const auto val = _mm256_cvtph_ps(Data);
    return { _mm256_castps256_ps128(val), _mm256_extractf128_ps(val, 1) };
#else
    constexpr uint16_t SignMask = 0x8000;
    constexpr uint16_t ExpMask  = 0x7c00;
    constexpr uint16_t FracMask = 0x03ff;
    constexpr uint16_t ExpShift = (127 - 15) << (16 - 1 - 8); // e' = (e - 15) + 127 => e' = e + (127 - 15)
    constexpr uint16_t ExpMax32 = 0x7f80;
    // fexp - 127 = trailing bit count
    // shiter = 10 - trailing bit count = 10 + 127 - fexp
    constexpr uint16_t FexpMax = 10 + 127;
    // minexp = (127 - 14), curexp = (127 - 15) = minexp - 1
    // exp = minexp - shifter = (127 - 15) + 1 - (10 + 127 - fexp) = fexp - 24
    // exp -= 10 - (fexp - 127) => -= 10 + 127 - fexp
    // exp = (127 - 15), exp -= FexpMax - fexp ==> exp = (127 - 15) - (FexpMax - fexp) = fexp - 25
    constexpr uint16_t FexpAdjust = (FexpMax - (127 - 15 + 1)) << 7;

    const auto misc0 = U16x8(SignMask, SignMask, ExpMask, ExpMask, FracMask, FracMask, ExpShift, ExpShift).As<U32x4>();
    const auto misc1 = U16x8(ExpMax32, ExpMax32, FexpMax, FexpMax, FexpAdjust, FexpAdjust, 0, 0).As<U32x4>();
        
    const auto signMask   = misc0.Broadcast<0>().As<U16x8>();
    const auto expMask    = misc0.Broadcast<1>().As<U16x8>();
    const auto fracMask   = misc0.Broadcast<2>().As<U16x8>();
    const auto expShift   = misc0.Broadcast<3>().As<U16x8>();
    const auto expMax32   = misc1.Broadcast<0>().As<U16x8>();
    const auto fexpMax    = misc1.Broadcast<1>().As<U16x8>();
    const auto fexpAdjust = misc1.Broadcast<2>().As<U16x8>();

    const U16x8 dat(Data);

    const auto ef = signMask.AndNot(dat);
    const auto e_ = dat.And(expMask);
    const auto f_ = dat.And(fracMask);
    const auto e_normal = e_.ShiftRightLogic<3>().Add(expShift);
    const auto isExpMax = e_.Compare<CompareType::Equal, MaskType::FullEle>(expMask);
    const auto e_nm = e_normal.SelectWith<MaskType::FullEle>(expMax32, isExpMax);
    const auto isExpMin = e_.Compare<CompareType::Equal, MaskType::FullEle>(U16x8::AllZero());
    const auto isDenorm = U16x8(_mm_sign_epi16(isExpMin.Data, ef.Data)); // ef == 0 then become zero
    const auto e_nmz = U16x8(_mm_sign_epi16(e_nm.Data, ef.Data)); // ef == 0 then become zero

    U16x8 e, f;
    IF_LIKELY(isDenorm.IsAllZero())
    { // all normal
        e = e_nmz;
        f = f_;
    }
    else
    { // calc denorm
        const auto f_fp = f_.Cast<F32x4>();
        const auto shufHiMask = _mm_setr_epi8(2, 3, 6, 7, 10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1);
        const auto f_fphi0 = _mm_shuffle_epi8(f_fp[0].As<U32x4>().Data, shufHiMask);
        const auto f_fphi1 = _mm_shuffle_epi8(f_fp[1].As<U32x4>().Data, shufHiMask);
        const auto f_fphi = U16x8(_mm_unpacklo_epi64(f_fphi0, f_fphi1));

        const auto fracShifter = fexpMax.Sub(f_fphi.ShiftRightLogic<7>());
        const auto denormFrac = f_.ShiftLeftLogic<false>(fracShifter).And(fracMask);

        const auto denormExp = f_fphi.Sub(fexpAdjust).And(expMax32);

        e = e_nmz.SelectWith<MaskType::FullEle>(denormExp, isDenorm);
        f = f_.SelectWith<MaskType::FullEle>(denormFrac, isDenorm);
    }
    const auto fhi = f.ShiftRightLogic<3>();
    const auto flo = f.ShiftLeftLogic<13>();

    const auto sign = dat.And(signMask);
    const auto sefh = sign.Or(fhi).Or(e);
    const auto out0 = flo.ZipLo(sefh).As<F32x4>(), out1 = flo.ZipHi(sefh).As<F32x4>();

    return { out0, out1 };
#endif
}


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return _mm_cvttps_epi32(Data);
}
template<> forceinline I16x8 VECCALL F32x4::Cast<I16x8, CastMode::RangeUndef>(const F32x4& arg1) const noexcept
{
    return Cast<I32x4>().Cast<I16x8>(arg1.Cast<I32x4>());
}
template<> forceinline U16x8 VECCALL F32x4::Cast<U16x8, CastMode::RangeUndef>(const F32x4& arg1) const noexcept
{
    return Cast<I16x8>(arg1).As<U16x8>();
}
template<> forceinline F16x8 VECCALL F32x4::Cast<F16x8, CastMode::RangeUndef>(const F32x4& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 100 && (defined(__F16C__) || COMMON_COMPILER_MSVC)
    return _mm256_cvtps_ph(_mm256_set_m128(arg1.Data, Data), _MM_FROUND_TO_NEAREST_INT);
#else
    constexpr uint16_t SignMask = 0x8000;
    constexpr uint16_t ExpMask  = 0x7f80;
    constexpr uint16_t FracMask = 0x03ff;
    constexpr uint16_t ExpShift = (127 - 15) << (16 - 1 - 8); // e' = (e - 15) + 127 => e' = e + (127 - 15)
    constexpr uint16_t ExpMin   = (127 - 15 - 10) << (16 - 1 - 8);
    constexpr uint16_t ExpMax   = (127 + 15 + 1) << (16 - 1 - 8);
    constexpr uint16_t FracHiBit = 0x0400;

    const auto misc0 = U16x8(SignMask, SignMask, ExpMask, ExpMask, FracMask, FracMask, ExpShift, ExpShift).As<U32x4>();
    const auto misc1 = U16x8(ExpMin, ExpMin, ExpMax, ExpMax, FracHiBit, FracHiBit, 1, 1).As<U32x4>();
        
    const auto signMask   = misc0.Broadcast<0>().As<U16x8>();
    const auto expMask    = misc0.Broadcast<1>().As<U16x8>();
    const auto fracMask   = misc0.Broadcast<2>().As<U16x8>();
    const auto expShift   = misc0.Broadcast<3>().As<U16x8>();
    const auto expMin     = misc1.Broadcast<0>().As<U16x8>();
    const auto expMax     = misc1.Broadcast<1>().As<U16x8>();
    const auto fracHiBit  = misc1.Broadcast<2>().As<U16x8>();
    const auto one        = misc1.Broadcast<3>().As<U16x8>();

    const U32x4 dat0 = As<U32x4>(), dat1 = arg1.As<U32x4>();

    const auto shufHiMask = _mm_setr_epi8(2, 3, 6, 7, 10, 11, 14, 15,
        0, 1, 4, 5, 8, 9, 12, 13);
    const auto datHi0 = _mm_shuffle_epi8(dat0.Data, shufHiMask);
    const auto datHi1 = _mm_shuffle_epi8(dat1.Data, shufHiMask);
    const auto datHi = U16x8(_mm_unpacklo_epi64(datHi0, datHi1));
    const auto frac0 = _mm_shuffle_epi8(dat0.ShiftLeftLogic<3>().Data, shufHiMask);
    const auto frac1 = _mm_shuffle_epi8(dat1.ShiftLeftLogic<3>().Data, shufHiMask);
    const auto fracPart = U16x8(_mm_unpacklo_epi64(frac0, frac1));
    const auto frac16 = fracPart.And(fracMask);
    const auto fracTrailing = U16x8(_mm_unpackhi_epi64(frac0, frac1));
    const auto isHalfRound = fracTrailing.Compare<CompareType::Equal, MaskType::FullEle>(signMask);
    const auto frac16LastBit = fracPart.ShiftLeftLogic<15>();
    const auto isRoundUpMSB = fracTrailing.SelectWith<MaskType::FullEle>(frac16LastBit, isHalfRound);
    const auto isRoundUpMask = isRoundUpMSB.As<I16x8>().ShiftRightArith<15>().As<U16x8>();
    const auto e_ = datHi.And(expMask); // [0000h~7f80h][denorm,inf]
    const auto e_hicut = e_.Min(expMax); // [0000h~4780h][denorm,2^16=inf]
    const auto e_shifted = e_hicut.Max(expShift).Sub(expShift); // [0000h~07c0h][0=denorm,31=inf]
    const auto e_normal = e_shifted.ShiftLeftLogic<3>(); // [0=denorm,31=inf]
    const auto mapToZero = e_.As<I16x8>().Compare<CompareType::LessThan, MaskType::FullEle>(expMin.As<I16x8>()).As<U16x8>(); // 2^-25, < 0.5*(min-denorm of fp16)
    const auto isNaNInf = e_.Compare<CompareType::Equal, MaskType::FullEle>(expMask); // NaN or Inf
    const auto mapToInf   = e_hicut.Compare<CompareType::Equal, MaskType::FullEle>(expMax); // 2^16(with high cut), become inf exp
    const auto isOverflow = isNaNInf.AndNot(mapToInf); // !isNaNInf && mapToInf
    const auto shouldFracZero = mapToZero.Or(isOverflow);
    const auto f_ = shouldFracZero.AndNot(frac16); // when shouldFracZero == ff, become 0
    const auto needRoundUpMask = shouldFracZero.AndNot(isRoundUpMask);

    const auto dontRequestDenorm = e_hicut.As<I16x8>().Compare<CompareType::GreaterThan, MaskType::FullEle>(expShift.As<I16x8>()).As<U16x8>();
    const auto notDenorm = mapToZero.Or(dontRequestDenorm); // mapToZero || dontRequestDenorm

    U16x8 e, f, r;
    e = e_normal;
    IF_LIKELY(notDenorm.Add(one).IsAllZero())
    { // all normal
        f = f_;
        r = needRoundUpMask;
    }
    else
    { // calc denorm
        const auto e_denormReq = /*shiftAdj*/expShift.Sub(e_hicut).ShiftRightLogic<16 - 9>(); // [f080h~3800h] -> [1e1h~070h], only [00h~09h] valid
        const auto f_shifted1 = f_.Or(fracHiBit).ShiftRightLogic<false>(e_denormReq);
        const auto denormRoundUpBit = f_shifted1.And(one);
        const auto denormRoundUp = U16x8(_mm_sign_epi16(denormRoundUpBit, signMask)); // bit==1 then become ffff, bit==0 then become 0
        f = f_shifted1.ShiftRightLogic<1>().SelectWith<MaskType::FullEle>(f_, notDenorm); // actually shift [0~9]+1
        r = denormRoundUp.SelectWith<MaskType::FullEle>(needRoundUpMask, notDenorm);
    }
    const auto sign = datHi.And(signMask);
    const auto sef = sign.Or(e).Or(f);
    const auto roundUpVal = _mm_abs_epi16(r); // full mask then 1, 0 then 0
    const auto out = sef.Add(roundUpVal);
    return out.As<F16x8>();
#endif
}
template<> forceinline I8x16 VECCALL F32x4::Cast<I8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    return Cast<I32x4>().Cast<I8x16>(arg1.Cast<I32x4>(), arg2.Cast<I32x4>(), arg3.Cast<I32x4>());
}
template<> forceinline U8x16 VECCALL F32x4::Cast<U8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    return Cast<I8x16>(arg1, arg2, arg3).As<U8x16>();
}
template<> forceinline Pack<F64x2, 2> VECCALL F32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return { _mm_cvtps_pd(Data), _mm_cvtps_pd(As<I32x4>().MoveHiToLo().As<F32x4>()) };
}
template<> forceinline F32x4 VECCALL F64x2::Cast<F32x4, CastMode::RangeUndef>(const F64x2& arg1) const noexcept
{
    return _mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(_mm_cvtpd_ps(Data)), _mm_castps_pd(_mm_cvtpd_ps(arg1.Data))));
}


template<> forceinline F16x8 VECCALL I16x8::Cast<F16x8, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV > 320 && COMMON_SIMD_FP16
    return _mm_castph_si128(_mm_cvtepi16_ph(Data));
#else
    const auto val = Cast<F32x4>();
    return val[0].Cast<F16x8>(val[1]);
#endif
}
template<> forceinline F16x8 VECCALL U16x8::Cast<F16x8, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV > 320 && COMMON_SIMD_FP16
    return _mm_castph_si128(_mm_cvtepu16_ph(Data));
#else
    const auto val = Cast<F32x4>();
    return val[0].Cast<F16x8>(val[1]);
#endif
}


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeSaturate>() const noexcept
{
    const F32x4 minVal = static_cast<float>(INT32_MIN), maxVal = static_cast<float>(INT32_MAX);
    const auto val = Cast<I32x4, CastMode::RangeUndef>();
    // INT32 loses precision, need maunally bit-select
    const auto isLe = Compare<CompareType::LessEqual,    MaskType::FullEle>(minVal).As<I32x4>();
    const auto isGe = Compare<CompareType::GreaterEqual, MaskType::FullEle>(maxVal).As<I32x4>();
    /*const auto isLe = _mm_castps_si128(_mm_cmple_ps(Data, minVal));
    const auto isGe = _mm_castps_si128(_mm_cmpge_ps(Data, maxVal));*/
    const auto satMin = _mm_blendv_epi8(val, I32x4(INT32_MIN), isLe);
    return _mm_blendv_epi8(satMin, I32x4(INT32_MAX), isGe);
}
template<> forceinline I16x8 VECCALL F32x4::Cast<I16x8, CastMode::RangeSaturate>(const F32x4& arg1) const noexcept
{
    const F32x4 minVal = static_cast<float>(INT16_MIN), maxVal = static_cast<float>(INT16_MAX);
    return Min(maxVal).Max(minVal).Cast<I16x8, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline U16x8 VECCALL F32x4::Cast<U16x8, CastMode::RangeSaturate>(const F32x4& arg1) const noexcept
{
    const F32x4 minVal = 0, maxVal = static_cast<float>(UINT16_MAX);
    return Min(maxVal).Max(minVal).Cast<U16x8, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline I8x16 VECCALL F32x4::Cast<I8x16, CastMode::RangeSaturate>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    const F32x4 minVal = static_cast<float>(INT8_MIN), maxVal = static_cast<float>(INT8_MAX);
    return Min(maxVal).Max(minVal).Cast<I8x16, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline U8x16 VECCALL F32x4::Cast<U8x16, CastMode::RangeSaturate>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    const F32x4 minVal = 0, maxVal = static_cast<float>(UINT8_MAX);
    return Min(maxVal).Max(minVal).Cast<U8x16, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline I16x8 VECCALL I32x4::Cast<I16x8, CastMode::RangeSaturate>(const I32x4& arg1) const noexcept
{
    return _mm_packs_epi32(Data, arg1);
}
template<> forceinline U16x8 VECCALL I32x4::Cast<U16x8, CastMode::RangeSaturate>(const I32x4& arg1) const noexcept
{
    return _mm_packus_epi32(Data, arg1);
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeSaturate>(const U32x4& arg1) const noexcept
{
    const auto data_ = Min(UINT16_MAX).As<I32x4>(), arg1_ = arg1.Min(UINT16_MAX).As<I32x4>();
    return data_.Cast<U16x8, CastMode::RangeSaturate>(arg1_);
}
template<> forceinline I8x16 VECCALL I16x8::Cast<I8x16, CastMode::RangeSaturate>(const I16x8& arg1) const noexcept
{
    return _mm_packs_epi16(Data, arg1);
}
template<> forceinline U8x16 VECCALL I16x8::Cast<U8x16, CastMode::RangeSaturate>(const I16x8& arg1) const noexcept
{
    return _mm_packus_epi16(Data, arg1);
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeSaturate>(const U16x8& arg1) const noexcept
{
    const auto data_ = Min(UINT8_MAX).As<I16x8>(), arg1_ = arg1.Min(UINT8_MAX).As<I16x8>();
    return data_.Cast<U8x16, CastMode::RangeSaturate>(arg1_);
}


}
