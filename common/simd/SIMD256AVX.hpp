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


namespace COMMON_SIMD_NAMESPACE
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
template<> forceinline __m256  AsType(__m256  from) noexcept { return from; }
template<> forceinline __m256i AsType(__m256  from) noexcept { return _mm256_castps_si256(from); }
template<> forceinline __m256d AsType(__m256  from) noexcept { return _mm256_castps_pd(from); }
template<> forceinline __m256  AsType(__m256i from) noexcept { return _mm256_castsi256_ps(from); }
template<> forceinline __m256i AsType(__m256i from) noexcept { return from; }
template<> forceinline __m256d AsType(__m256i from) noexcept { return _mm256_castsi256_pd(from); }
template<> forceinline __m256  AsType(__m256d from) noexcept { return _mm256_castpd_ps(from); }
template<> forceinline __m256i AsType(__m256d from) noexcept { return _mm256_castpd_si256(from); }
template<> forceinline __m256d AsType(__m256d from) noexcept { return from; }


inline constexpr uint32_t TrimMask(uint32_t mask, uint32_t bits, uint32_t stride)
{
    uint32_t ret = 0;
    for (uint32_t i = 0, j = 0; j < bits; i++, j += stride)
        if ((mask >> j) & 0x1)
            ret |= 0x1u << i;
    return ret;
}

template<uint8_t Limit = 4>
inline constexpr uint8_t GetPermuteMask(uint8_t lo, uint8_t hi)
{
    return static_cast<uint8_t>((lo >= Limit ? 0xf : lo) | ((hi >= Limit ? 0xf : hi) << 4));
}


template<typename T, typename E>
struct AVX256Shared : public CommonOperators<T>
{
    static constexpr uint8_t LaneCount = 2;
    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL PermuteLane(const T& other) const noexcept
    {
        static_assert(Lo <= 4 && Hi <= 4, "permute index should be in [0,4]");
        if constexpr (Lo == 4 && Hi == 4) return T::AllZero();
        else if constexpr (Lo == 0 && Hi == 1) return *static_cast<const T*>(this);
        else if constexpr (Lo == 2 && Hi == 3) return other;
        else if constexpr ((Lo == 0 || Lo == 2) && Hi == 4) // clear Hi
        {
            const auto target = Lo == 0 ? static_cast<const T*>(this)->Data : other.Data;
            if constexpr (std::is_same_v<E, double>)
                return _mm256_zextpd128_pd256(_mm256_castpd256_pd128(target));
            else if constexpr (std::is_same_v<E, float>)
                return _mm256_zextps128_ps256(_mm256_castps256_ps128(target));
            else
                return _mm256_zextsi128_si256(_mm256_castsi256_si128(target));
        }
        else if constexpr ((Lo == 0 && Hi == 3) || (Lo == 2 && Hi == 1)) // blend
        {
            const auto& a = Lo == 0 ? static_cast<const T*>(this)->Data : other.Data;
            const auto& b = Lo == 0 ? other.Data : static_cast<const T*>(this)->Data;
            if constexpr (std::is_same_v<E, double>)
                return _mm256_blend_pd(a, b, 0b1100);
            else if constexpr (std::is_same_v<E, float>)
                return _mm256_blend_ps(a, b, 0xf0);
            else
#if COMMON_SIMD_LV >= 200
                return _mm256_blend_epi32(a, b, 0xf0);
#else
                return _mm256_castps_si256(_mm256_blend_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), 0xf0));
#endif
        }
        else
        {
            constexpr auto imm8 = detail::GetPermuteMask(Lo, Hi);
            if constexpr (std::is_same_v<E, double>)
                return _mm256_permute2f128_pd(static_cast<const T*>(this)->Data, other.Data, imm8);
            else if constexpr (std::is_same_v<E, float>)
                return _mm256_permute2f128_ps(static_cast<const T*>(this)->Data, other.Data, imm8);
            else
#if COMMON_SIMD_LV >= 200
                return _mm256_permute2x128_si256(static_cast<const T*>(this)->Data, other.Data, imm8);
#else
                return _mm256_permute2f128_si256(static_cast<const T*>(this)->Data, other.Data, imm8);
#endif
        }
    }
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL PermuteLane() const noexcept
    {
        static_assert(Lo <= 2 && Hi <= 2, "permute index should be in [0,2]");
        constexpr auto imm8 = detail::GetPermuteMask<2>(Lo, Hi);
        if constexpr (std::is_same_v<E, double>)
            return _mm256_permute2f128_pd(static_cast<const T*>(this)->Data, static_cast<const T*>(this)->Data, imm8);
        else if constexpr (std::is_same_v<E, float>)
            return _mm256_permute2f128_ps(static_cast<const T*>(this)->Data, static_cast<const T*>(this)->Data, imm8);
        else
#if COMMON_SIMD_LV >= 200
            return _mm256_permute2x128_si256(static_cast<const T*>(this)->Data, static_cast<const T*>(this)->Data, imm8);
#else
            return _mm256_permute2f128_si256(static_cast<const T*>(this)->Data, static_cast<const T*>(this)->Data, imm8);
#endif
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept
    {
        const auto lolane = static_cast<const T*>(this)->ZipLoLane(other), hilane = static_cast<const T*>(this)->ZipHiLane(other);
        return lolane.template PermuteLane<0, 2>(hilane);
    }
    forceinline T VECCALL ZipHi(const T& other) const noexcept
    {
        const auto lolane = static_cast<const T*>(this)->ZipLoLane(other), hilane = static_cast<const T*>(this)->ZipHiLane(other);
        return lolane.template PermuteLane<1, 3>(hilane);
    }
    forceinline Pack<T, 2> VECCALL Zip(const T& other) const noexcept
    {
        const auto lolane = static_cast<const T*>(this)->ZipLoLane(other), hilane = static_cast<const T*>(this)->ZipHiLane(other);
        return { lolane.template PermuteLane<0, 2>(hilane), lolane.template PermuteLane<1, 3>(hilane) };
    }
};


// For integer
template<typename T, typename L, typename E, size_t N>
struct AVX256Common : public AVX256Shared<T, E>
{
    static_assert(std::is_same_v<typename L::VecType, __m128i>);
    static_assert(std::is_same_v<typename L::EleType, E> && L::Count * 2 == N);
    using EleType = E;
    using VecType = __m256i;
    static constexpr size_t Count = N;
    static constexpr VecDataInfo VDInfo =
    {
        std::is_unsigned_v<E> ? VecDataInfo::DataTypes::Unsigned : VecDataInfo::DataTypes::Signed,
        static_cast<uint8_t>(256 / N), N, 0
    };
    union
    {
        __m256i Data;
        E Val[N];
    };
    forceinline constexpr AVX256Common() noexcept : Data() { }
    forceinline explicit AVX256Common(const E* ptr) noexcept : Data(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) { }
    forceinline constexpr AVX256Common(const __m256i val) noexcept : Data(val) { }
    forceinline AVX256Common(const Pack<L, 2>& pack) noexcept : Data(_mm256_set_m128i(pack[1].Data, pack[0].Data)) {}
    forceinline AVX256Common(const L& lo, const L& hi) noexcept : Data(_mm256_set_m128i(hi.Data, lo.Data)) {}
    forceinline void VECCALL Load(const E* ptr) noexcept { Data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)); }
    forceinline void VECCALL Save(E* ptr) const noexcept { _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), Data); }
    forceinline constexpr operator const __m256i& () const noexcept { return Data; }

#if COMMON_SIMD_LV >= 200
    // logic operations
    forceinline T VECCALL And(const T& other) const noexcept
    {
        return _mm256_and_si256(Data, other.Data);
    }
    forceinline T VECCALL Or(const T& other) const noexcept
    {
        return _mm256_or_si256(Data, other.Data);
    }
    forceinline T VECCALL Xor(const T& other) const noexcept
    {
        return _mm256_xor_si256(Data, other.Data);
    }
    forceinline T VECCALL AndNot(const T& other) const noexcept
    {
        return _mm256_andnot_si256(Data, other.Data);
    }
    forceinline T VECCALL Not() const noexcept
    {
        return _mm256_xor_si256(Data, _mm256_set1_epi8(-1));
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
    template<uint8_t Cnt>
    forceinline T VECCALL MoveLaneToHi() const noexcept
    {
        static_assert(Cnt <= N / 2, "move count should be in [0,N/2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N / 2) return AllZero();
        else return _mm256_slli_si256(Data, Cnt * sizeof(E));
    }
    template<uint8_t Cnt>
    forceinline T VECCALL MoveLaneToLo() const noexcept
    {
        static_assert(Cnt <= N / 2, "move count should be in [0,N/2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N / 2) return AllZero();
        else return _mm256_srli_si256(Data, Cnt * sizeof(E));
    }
    forceinline T VECCALL MoveLaneHiToLo() const noexcept { return MoveLaneToLo<N / 4>(); }
    template<uint8_t Cnt>
    forceinline T VECCALL MoveLaneToLoWith(const T& hi) const noexcept
    {
        static_assert(Cnt <= N / 2, "move count should be in [0,N/2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N / 2) return hi;
        else return _mm256_alignr_epi8(hi.Data, Data, Cnt * sizeof(E));
    }
    forceinline T VECCALL MoveHiToLo() const noexcept { return _mm256_permute4x64_epi64(Data, 0b01001110); }
    forceinline T VECCALL ShuffleHiLo() const noexcept { return _mm256_permute4x64_epi64(Data, 0b01001110); }
#endif
    forceinline L VECCALL GetLoLane() const noexcept
    {
        return _mm256_castsi256_si128(Data);
//#if COMMON_SIMD_LV >= 200
//        return _mm256_extracti128_si256(Data, 0);
//#else
//        return _mm256_extractf128_si256(Data, 0);
//#endif
    }
    forceinline L VECCALL GetHiLane() const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm256_extracti128_si256(Data, 1);
#else
        return _mm256_extractf128_si256(Data, 1);
#endif
    }
    forceinline bool VECCALL IsAllZero() const noexcept
    {
        return _mm256_testz_si256(Data, Data) != 0;
    }

    forceinline static T VECCALL AllZero() noexcept { return _mm256_setzero_si256(); }
    forceinline static T VECCALL LoadLoLane(const L& lane) noexcept { return _mm256_zextsi128_si256(lane); }
    forceinline static T VECCALL LoadLo(const E val) noexcept { return LoadLoLane(L::LoadLo(val)); }
    forceinline static T VECCALL LoadLo(const E* ptr) noexcept { return LoadLoLane(L::LoadLo(ptr)); }
    forceinline static T VECCALL BroadcastLane(const L& lane) noexcept
    {
#if COMMON_SIMD_LV >= 200
        return _mm256_broadcastsi128_si256(lane);
#else
        return { lane, lane };
#endif
    }
    forceinline static T VECCALL Combine(const L& lo, const L& hi) noexcept { return Pack<L, 2>{lo, hi}; }
};


template<typename T, typename L, typename E>
struct alignas(32) Common64x4 : public detail::AVX256Common<T, L, E, 4>
{
private:
    using AVX256Common = detail::AVX256Common<T, L, E, 4>;
public:
    using AVX256Common::AVX256Common;
    using AVX256Common::BroadcastLane;
    forceinline Common64x4(const E val) noexcept : AVX256Common(_mm256_set1_epi64x(static_cast<int64_t>(val))) { }
    forceinline Common64x4(const E lo0, const E lo1, const E lo2, const E hi3) noexcept :
        AVX256Common(_mm256_setr_epi64x(static_cast<int64_t>(lo0), static_cast<int64_t>(lo1), static_cast<int64_t>(lo2), static_cast<int64_t>(hi3))) { }

#if COMMON_SIMD_LV >= 200
    // shuffle operations
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL ShuffleLane() const noexcept
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        constexpr uint8_t Lo2 = Lo * 2, Hi2 = Hi * 2;
        return _mm256_shuffle_epi32(this->Data, ((Hi2 + 1) << 6) + (Hi2 << 4) + ((Lo2 + 1) << 2) + Lo2);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm256_permute4x64_epi64(this->Data, (Hi3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const noexcept
    {
        //assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        const auto mask = _mm256_setr_epi32(Lo0 * 2, Lo0 * 2 + 1, Lo1 * 2, Lo1 * 2 + 1, Lo2 * 2, Lo2 * 2 + 1, Hi3 * 2, Hi3 * 2 + 1);
        return _mm256_permutevar8x32_epi32(this->Data, mask);
    }
    template<uint8_t Idx>
    forceinline T VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return ShuffleLane<Idx, Idx>();
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return Shuffle<Idx, Idx, Idx, Idx>();
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm256_set_epi64x(0x08090a0b0c0d0e0fULL, 0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL, 0x0001020304050607ULL);
        return _mm256_shuffle_epi8(this->Data, SwapMask);
    }

    forceinline T VECCALL ZipLoLane(const T& other) const noexcept { return _mm256_unpacklo_epi64(this->Data, other.Data); }
    forceinline T VECCALL ZipHiLane(const T& other) const noexcept { return _mm256_unpackhi_epi64(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return ZipLoLane(other); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return ZipHiLane(other); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr (Msk != MaskType::FullEle)
            return _mm256_castpd_si256(_mm256_blendv_pd(_mm256_castsi256_pd(this->Data), _mm256_castsi256_pd(other.Data), _mm256_castsi256_pd(mask))); // less inst with higher latency
//# if COMMON_SIMD_LV >= 320
//        mask = _mm256_srai_epi64(mask, 64);
//# else
//        mask = _mm256_srai_epi32(_mm256_shuffle_epi32(mask, 0xf5), 32);
//# endif
        else
            return _mm256_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
        return _mm256_blend_epi32(this->Data, other.Data, (Mask & 0b1 ? 0b11 : 0) |
            (Mask & 0b10 ? 0b1100 : 0) | (Mask & 0b100 ? 0b110000 : 0) | (Mask & 0b1000 ? 0b11000000 : 0));
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm256_add_epi64(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm256_sub_epi64(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        if constexpr (Enforce) return _mm256_mullo_epi64_force(this->Data, other.Data);
        else return _mm256_mullo_epi64(this->Data, other.Data);
# else
        const E 
            lo0A = _mm256_extract_epi64(this->Data, 0), lo1A = _mm256_extract_epi64(this->Data, 1), 
            lo2A = _mm256_extract_epi64(this->Data, 2), hi3A = _mm256_extract_epi64(this->Data, 3),
            lo0B = _mm256_extract_epi64(other.Data, 0), lo1B = _mm256_extract_epi64(other.Data, 1), 
            lo2B = _mm256_extract_epi64(other.Data, 2), hi3B = _mm256_extract_epi64(other.Data, 3);
        return { static_cast<E>(lo0A * lo0B), static_cast<E>(lo1A * lo1B), static_cast<E>(lo2A * lo2B), static_cast<E>(hi3A * hi3B) };
# endif
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = MulLo(other); return *static_cast<T*>(this); }
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm256_sll_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm256_srl_epi64(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm256_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
        else
# if COMMON_SIMD_LV >= 320
            return _mm256_sra_epi64(this->Data, _mm_cvtsi32_si128(bits));
# else
        {
            if (bits == 0) return this->Data;
            const auto sign32H = _mm256_srai_epi32(this->Data, 32); // only high part useful
            const auto signAll = _mm256_shuffle_epi32(sign32H, 0b11110101);
            if (bits > 63) return signAll;
            const auto zero64 = _mm256_srl_epi64(this->Data, _mm_cvtsi32_si128(bits));
            const auto sign64 = _mm256_sll_epi64(signAll, _mm_cvtsi32_si128(64 - bits));
            return _mm256_or_si256(zero64, sign64);
        }
# endif
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
        return _mm256_sllv_epi64(this->Data, bits.Data);
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
        return _mm256_srlv_epi64(this->Data, bits.Data);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const noexcept
    {
        if constexpr (N >= 64)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_slli_epi64(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N >= 64)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_srli_epi64(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm256_srli_epi64(this->Data, N);
# if COMMON_SIMD_LV >= 320
        else
            return _mm256_srai_epi64(this->Data, N);
# else
        else if constexpr (N == 0)
        {
            return this->Data;
        }
        else
        {
            const auto sign32H = _mm256_srai_epi32(this->Data, 32); // only high part useful
            const auto signAll = _mm256_shuffle_epi32(sign32H, 0b11110101);
            if constexpr (N > 63)
                return signAll;
            else
            {
                const auto zero64 = _mm256_srli_epi64(this->Data, N);
                const auto sign64 = _mm256_slli_epi64(signAll, 64 - N);
                return _mm256_or_si256(zero64, sign64);
            }
        }
# endif
    }
#endif
};


template<typename T, typename L, typename E>
struct alignas(32) Common32x8 : public detail::AVX256Common<T, L, E, 8>
{
private:
    using AVX256Common = detail::AVX256Common<T, L, E, 8>;
public:
    using AVX256Common::AVX256Common;
    using AVX256Common::BroadcastLane;
    forceinline Common32x8(const E val) noexcept : AVX256Common(_mm256_set1_epi32(static_cast<int32_t>(val))) { }
    forceinline Common32x8(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E hi7) noexcept :
        AVX256Common(_mm256_setr_epi32(static_cast<int32_t>(lo0), static_cast<int32_t>(lo1), static_cast<int32_t>(lo2), static_cast<int32_t>(lo3), static_cast<int32_t>(lo4), static_cast<int32_t>(lo5), static_cast<int32_t>(lo6), static_cast<int32_t>(hi7))) { }

#if COMMON_SIMD_LV >= 200
    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline T VECCALL ShuffleLane() const noexcept // no cross lane
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm256_shuffle_epi32(this->Data, (Hi3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        if constexpr (Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Lo3 < 4 &&
            Lo4 - Lo0 == 4 && Lo5 - Lo1 == 4 && Lo6 - Lo2 == 4 && Hi7 - Lo3 == 4) // no cross lane and same shuffle for two lane
        {
            return ShuffleLane<Lo0, Lo1, Lo2, Lo3>();
        }
        else
        {
            const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
            return _mm256_permutevar8x32_epi32(this->Data, mask);
        }
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const noexcept
    {
        //static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        const auto mask = _mm256_setr_epi32(Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7);
        return _mm256_permutevar8x32_epi32(this->Data, mask);
    }
    template<uint8_t Idx>
    forceinline T VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return ShuffleLane<Idx, Idx, Idx, Idx>();
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 8, "shuffle index should be in [0,7]");
        return Shuffle<Idx, Idx, Idx, Idx, Idx, Idx, Idx, Idx>();
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm256_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL, 0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
        return _mm256_shuffle_epi8(this->Data, SwapMask);
    }

    forceinline T VECCALL ZipLoLane(const T& other) const noexcept { return _mm256_unpacklo_epi32(this->Data, other.Data); }
    forceinline T VECCALL ZipHiLane(const T& other) const noexcept { return _mm256_unpackhi_epi32(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0xaa>(_mm256_slli_epi64(other.Data, 32)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0x55>(_mm256_srli_epi64(this->Data, 32)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr (Msk != MaskType::FullEle)
            return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(this->Data), _mm256_castsi256_ps(other.Data), _mm256_castsi256_ps(mask))); // less inst with higher latency
            // mask = _mm256_srai_epi32(mask, 32);
        else
            return _mm256_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        return _mm256_blend_epi32(this->Data, other.Data, Mask);
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm256_add_epi32(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm256_sub_epi32(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept
    {
        if constexpr (Enforce) return _mm256_mullo_epi32_force(this->Data, other.Data);
        else return _mm256_mullo_epi32(this->Data, other.Data);
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T& VECCALL operator*=(const T& other) noexcept { this->Data = MulLo(other); return *static_cast<T*>(this); }
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm256_sll_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm256_srl_epi32(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return _mm256_srl_epi32(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm256_sra_epi32(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
        return _mm256_sllv_epi32(this->Data, bits.Data);
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
        return _mm256_srlv_epi32(this->Data, bits.Data);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const noexcept
    {
        if constexpr (N >= 32)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_slli_epi32(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N >= 32)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_srli_epi32(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>) 
            return _mm256_srli_epi32(this->Data, N);
        else 
            return _mm256_srai_epi32(this->Data, N);
    }
#endif
};


template<typename T, typename L, typename E>
struct alignas(32) Common16x16 : public detail::AVX256Common<T, L, E, 16>
{
private:
    using AVX256Common = detail::AVX256Common<T, L, E, 16>;
public:
    using AVX256Common::AVX256Common;
    using AVX256Common::BroadcastLane;
    forceinline Common16x16(const E val) noexcept : AVX256Common(_mm256_set1_epi16(static_cast<int16_t>(val))) { }
    forceinline Common16x16(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7,
        const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E hi15) noexcept :
        AVX256Common(_mm256_setr_epi16(
            static_cast<int16_t>(lo0),  static_cast<int16_t>(lo1),  static_cast<int16_t>(lo2),  static_cast<int16_t>(lo3), 
            static_cast<int16_t>(lo4),  static_cast<int16_t>(lo5),  static_cast<int16_t>(lo6),  static_cast<int16_t>(lo7),
            static_cast<int16_t>(lo8),  static_cast<int16_t>(lo9),  static_cast<int16_t>(lo10), static_cast<int16_t>(lo11), 
            static_cast<int16_t>(lo12), static_cast<int16_t>(lo13), static_cast<int16_t>(lo14), static_cast<int16_t>(hi15))) { }

#if COMMON_SIMD_LV >= 200
    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL ShuffleLane() const noexcept // no cross lane
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        const auto mask = _mm256_setr_epi8(
            static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1), static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1),
            static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1), static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1),
            static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1), static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1),
            static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1), static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1),
            static_cast<int8_t>(Lo0 * 2), static_cast<int8_t>(Lo0 * 2 + 1), static_cast<int8_t>(Lo1 * 2), static_cast<int8_t>(Lo1 * 2 + 1),
            static_cast<int8_t>(Lo2 * 2), static_cast<int8_t>(Lo2 * 2 + 1), static_cast<int8_t>(Lo3 * 2), static_cast<int8_t>(Lo3 * 2 + 1),
            static_cast<int8_t>(Lo4 * 2), static_cast<int8_t>(Lo4 * 2 + 1), static_cast<int8_t>(Lo5 * 2), static_cast<int8_t>(Lo5 * 2 + 1),
            static_cast<int8_t>(Lo6 * 2), static_cast<int8_t>(Lo6 * 2 + 1), static_cast<int8_t>(Hi7 * 2), static_cast<int8_t>(Hi7 * 2 + 1));
        return _mm256_shuffle_epi8(this->Data, mask);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        if constexpr (Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Lo7 < 8
            && Lo8 - Lo0 == 8 && Lo9 - Lo1 == 8 && Lo10 - Lo2 == 8 && Lo11 - Lo3 == 8 && Lo12 - Lo4 == 8 && Lo13 - Lo5 == 8 && Lo14 - Lo6 == 8 && Hi15 - Lo7 == 8) // no cross lane and same shuffle for two lane
        {
            return ShuffleLane<Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Lo7>();
        }
        else
        {
            static_assert(!::common::AlwaysTrue2<Lo0>, "Unimplemented");
            return *this;
        }
    }
    template<uint8_t Idx>
    forceinline T VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 8, "shuffle index should be in [0,7]");
        constexpr auto NewIdx = Idx % 4;
        constexpr auto Idx4 = (NewIdx << 6) + (NewIdx << 4) + (NewIdx << 2) + NewIdx;
        if constexpr (Idx < 4) // lo half
        {
            const auto lo = _mm256_shufflelo_epi16(this->Data, Idx4);
            return _mm256_unpacklo_epi64(lo, lo);
        }
        else // hi half
        {
            const auto hi = _mm256_shufflehi_epi16(this->Data, Idx4);
            return _mm256_unpackhi_epi64(hi, hi);
        }
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 16, "shuffle index should be in [0,15]");
        if constexpr (Idx < 8)
        {
            auto lo = this->GetLoLane();
            if constexpr (Idx != 0)
                lo = _mm_srli_si128(lo, Idx * 2);
            return _mm256_broadcastw_epi16(lo);
        }
        else
        {
            const auto lane = BroadcastLane<Idx - 8>();
            return _mm256_permute2x128_si256(lane, lane, 0x11);
        }
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        const auto SwapMask = _mm256_set_epi64x(0x0e0f0c0d0a0b0809ULL, 0x0607040502030001ULL, 0x0e0f0c0d0a0b0809ULL, 0x0607040502030001ULL);
        return _mm256_shuffle_epi8(this->Data, SwapMask);
    }

    forceinline T VECCALL ZipLoLane(const T& other) const noexcept { return _mm256_unpacklo_epi16(this->Data, other.Data); }
    forceinline T VECCALL ZipHiLane(const T& other) const noexcept { return _mm256_unpackhi_epi16(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0xaaaa>(_mm256_slli_epi32(other.Data, 16)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0x5555>(_mm256_srli_epi32(this->Data, 16)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept
    {
        if constexpr (Msk != MaskType::FullEle)
            mask = _mm256_srai_epi16(mask, 16); // make sure all bits are covered
        return _mm256_blendv_epi8(this->Data, other.Data, mask);
    }
    template<uint16_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        if constexpr ((Mask & 0xff) == (Mask >> 8))
            return _mm256_blend_epi16(this->Data, other.Data, static_cast<uint8_t>(Mask));
        else if constexpr ((Mask & 0xaaaa) == ((Mask & 0x5555) << 1))
        {
            constexpr uint8_t mask = static_cast<uint8_t>(TrimMask(Mask, 16, 2));
            return _mm256_blend_epi32(this->Data, other.Data, mask);
        }
        else
        {
# if COMMON_SIMD_LV >= 320
            return _mm256_mask_blend_epi16(Mask, this->Data, other.Data);
# else
            const auto lo = _mm_blend_epi16(_mm256_castsi256_si128(this->Data), _mm256_castsi256_si128(other.Data), static_cast<uint8_t>(Mask));
            const auto hi = _mm256_blend_epi16(this->Data, other.Data, static_cast<uint8_t>(Mask >> 8));
            return _mm256_insertf128_si256(hi, lo, 0);
#endif
        }
    }
    
    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm256_add_epi16(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm256_sub_epi16(this->Data, other.Data); }
    template<bool Enforce = false>
    forceinline T VECCALL MulLo(const T& other) const noexcept
    {
        if constexpr (Enforce) return _mm256_mullo_epi16_force(this->Data, other.Data);
        else return _mm256_mullo_epi16(this->Data, other.Data);
    }
    forceinline T VECCALL operator*(const T& other) const noexcept { return MulLo(other); }
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept { return _mm256_sll_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept { return _mm256_srl_epi16(this->Data, _mm_cvtsi32_si128(bits)); }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm256_srl_epi16(this->Data, _mm_cvtsi32_si128(bits));
        else
            return _mm256_sra_epi16(this->Data, _mm_cvtsi32_si128(bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_sllv_epi16(this->Data, bits.Data);
# else
        const auto loMask = _mm256_set1_epi32(0x0000ffff);
        const auto bitsLo = _mm256_and_si256(bits.Data, loMask);
        const auto lo = _mm256_sllv_epi32(this->Data, bitsLo);
        const auto dataHi = _mm256_andnot_si256(loMask, this->Data);
        const auto bitsHi = _mm256_srli_epi32(bits.Data, 16);
        const auto hi = _mm256_sllv_epi32(dataHi, bitsHi);
        return _mm256_blend_epi16(lo, hi, 0xaa);
# endif
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_srlv_epi16(this->Data, bits.Data);
# else
        const auto loMask = _mm256_set1_epi32(0x0000ffff);
        const auto dataLo = _mm256_and_si256(loMask, this->Data);
        const auto bitsLo = _mm256_and_si256(bits.Data, loMask);
        const auto lo = _mm256_srlv_epi32(dataLo, bitsLo);
        const auto bitsHi = _mm256_srli_epi32(bits.Data, 16);
        const auto hi = _mm256_srlv_epi32(this->Data, bitsHi);
        return _mm256_blend_epi16(lo, hi, 0xaa);
# endif
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic() const noexcept
    {
        if constexpr (N >= 16)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_slli_epi16(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N >= 16)
            return T::AllZero();
        else if constexpr (N == 0)
            return this->Data;
        else
            return _mm256_srli_epi16(this->Data, N);
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    {
        if constexpr (std::is_unsigned_v<E>)
            return _mm256_srli_epi16(this->Data, N);
        else
            return _mm256_srai_epi16(this->Data, N);
    }
#endif
};


template<typename T, typename L, typename E>
struct alignas(32) Common8x32 : public detail::AVX256Common<T, L, E, 32>
{
private:
    using AVX256Common = detail::AVX256Common<T, L, E, 32>;
public:
    using AVX256Common::AVX256Common;
    using AVX256Common::BroadcastLane;
    forceinline Common8x32(const E val) noexcept : AVX256Common(_mm256_set1_epi8(static_cast<int8_t>(val))) { }
    forceinline Common8x32(const E lo0, const E lo1, const E lo2, const E lo3, const E lo4, const E lo5, const E lo6, const E lo7,
        const E lo8, const E lo9, const E lo10, const E lo11, const E lo12, const E lo13, const E lo14, const E lo15,
        const E lo16, const E lo17, const E lo18, const E lo19, const E lo20, const E lo21, const E lo22, const E lo23,
        const E lo24, const E lo25, const E lo26, const E lo27, const E lo28, const E lo29, const E lo30, const E hi31) noexcept :
        AVX256Common(_mm256_setr_epi8(
            static_cast<int8_t>(lo0),  static_cast<int8_t>(lo1),  static_cast<int8_t>(lo2),  static_cast<int8_t>(lo3),
            static_cast<int8_t>(lo4),  static_cast<int8_t>(lo5),  static_cast<int8_t>(lo6),  static_cast<int8_t>(lo7), 
            static_cast<int8_t>(lo8),  static_cast<int8_t>(lo9),  static_cast<int8_t>(lo10), static_cast<int8_t>(lo11), 
            static_cast<int8_t>(lo12), static_cast<int8_t>(lo13), static_cast<int8_t>(lo14), static_cast<int8_t>(lo15), 
            static_cast<int8_t>(lo16), static_cast<int8_t>(lo17), static_cast<int8_t>(lo18), static_cast<int8_t>(lo19), 
            static_cast<int8_t>(lo20), static_cast<int8_t>(lo21), static_cast<int8_t>(lo22), static_cast<int8_t>(lo23), 
            static_cast<int8_t>(lo24), static_cast<int8_t>(lo25), static_cast<int8_t>(lo26), static_cast<int8_t>(lo27), 
            static_cast<int8_t>(lo28), static_cast<int8_t>(lo29), static_cast<int8_t>(lo30), static_cast<int8_t>(hi31))) { }

#if COMMON_SIMD_LV >= 200
    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL ShuffleLane() const noexcept // no cross lane
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        const auto mask = _mm256_setr_epi8(
            static_cast<int8_t>(Lo0),  static_cast<int8_t>(Lo1),  static_cast<int8_t>(Lo2),  static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4),  static_cast<int8_t>(Lo5),  static_cast<int8_t>(Lo6),  static_cast<int8_t>(Lo7),
            static_cast<int8_t>(Lo8),  static_cast<int8_t>(Lo9),  static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11),
            static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13), static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15),
            static_cast<int8_t>(Lo0),  static_cast<int8_t>(Lo1),  static_cast<int8_t>(Lo2),  static_cast<int8_t>(Lo3),
            static_cast<int8_t>(Lo4),  static_cast<int8_t>(Lo5),  static_cast<int8_t>(Lo6),  static_cast<int8_t>(Lo7),
            static_cast<int8_t>(Lo8),  static_cast<int8_t>(Lo9),  static_cast<int8_t>(Lo10), static_cast<int8_t>(Lo11),
            static_cast<int8_t>(Lo12), static_cast<int8_t>(Lo13), static_cast<int8_t>(Lo14), static_cast<int8_t>(Hi15));
        return _mm256_shuffle_epi8(this->Data, mask);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Lo15,
        uint8_t Lo16, uint8_t Lo17, uint8_t Lo18, uint8_t Lo19, uint8_t Lo20, uint8_t Lo21, uint8_t Lo22, uint8_t Lo23, uint8_t Lo24, uint8_t Lo25, uint8_t Lo26, uint8_t Lo27, uint8_t Lo28, uint8_t Lo29, uint8_t Lo30, uint8_t Hi31>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 32 && Lo1 < 32 && Lo2 < 32 && Lo3 < 32 && Lo4 < 32 && Lo5 < 32 && Lo6 < 32 && Lo7 < 32
            && Lo8 < 32 && Lo9 < 32 && Lo10 < 32 && Lo11 < 32 && Lo12 < 32 && Lo13 < 32 && Lo14 < 32 && Lo15 < 32
            && Lo16 < 32 && Lo17 < 32 && Lo18 < 32 && Lo19 < 32 && Lo20 < 32 && Lo21 < 32 && Lo22 < 32 && Lo23 < 32
            && Lo24 < 32 && Lo25 < 32 && Lo26 < 32 && Lo27 < 32 && Lo28 < 32 && Lo29 < 32 && Lo30 < 32 && Hi31 < 32, "shuffle index should be in [0,31]");
        if constexpr (Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Lo15 < 16
            && Lo16 - Lo0 == 16 && Lo17 - Lo1 == 16 && Lo18 - Lo2 == 16 && Lo19 - Lo3 == 16 && Lo20 - Lo4 == 16 && Lo21 - Lo5 == 16 && Lo22 - Lo6 == 16 && Lo23 - Lo7 == 16
            && Lo24 - Lo8 == 16 && Lo25 - Lo9 == 16 && Lo26 - Lo10 == 16 && Lo27 - Lo11 == 16 && Lo28 - Lo12 == 16 && Lo29 - Lo13 == 16 && Lo30 - Lo14 == 16 && Hi31 - Lo15 == 16) // no cross lane and same shuffle for two lane
        {
            return ShuffleLane<Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Lo7, Lo8, Lo9, Lo10, Lo11, Lo12, Lo13, Lo14, Lo15>();
        }
        else
        {
            static_assert(!::common::AlwaysTrue2<Lo0>, "Unimplemented");
            return *this;
        }
    }
    template<uint8_t Idx>
    forceinline T VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 16, "shuffle index should be in [0,15]");
        constexpr auto PairIdx = Idx % 8;
        __m256i paired;
        if constexpr (Idx < 8) // lo half
            paired = _mm256_unpacklo_epi8(this->Data, this->Data);
        else // hi half
            paired = _mm256_unpackhi_epi8(this->Data, this->Data);
        constexpr auto NewIdx = PairIdx % 4;
        constexpr auto Idx4 = (NewIdx << 6) + (NewIdx << 4) + (NewIdx << 2) + NewIdx;
        if constexpr (PairIdx < 4) // lo half
        {
            const auto lo = _mm256_shufflelo_epi16(paired, Idx4);
            return _mm256_unpacklo_epi64(lo, lo);
        }
        else // hi half
        {
            const auto hi = _mm256_shufflehi_epi16(paired, Idx4);
            return _mm256_unpackhi_epi64(hi, hi);
        }
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 32, "shuffle index should be in [0,31]");
        if constexpr (Idx < 16)
        {
            auto lo = this->GetLoLane();
            if constexpr (Idx != 0)
                lo = _mm_srli_si128(lo, Idx);
            return _mm256_broadcastb_epi8(lo);
        }
        else
        {
            const auto lane = BroadcastLane<Idx - 16>();
            return _mm256_permute2x128_si256(lane, lane, 0x11);
        }
    }

    forceinline T VECCALL ZipLoLane(const T& other) const noexcept { return _mm256_unpacklo_epi8(this->Data, other.Data); }
    forceinline T VECCALL ZipHiLane(const T& other) const noexcept { return _mm256_unpackhi_epi8(this->Data, other.Data); }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept { return SelectWith<0xaaaaaaaa>(_mm256_slli_epi16(other.Data, 8)); }
    forceinline T VECCALL ZipEven(const T& other) const noexcept { return other.template SelectWith<0x55555555>(_mm256_srli_epi16(this->Data, 8)); }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, T mask) const noexcept { return _mm256_blendv_epi8(this->Data, other, mask); }
    template<uint32_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        if constexpr (Mask == 0)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0xffffffffu)
            return other;
# if COMMON_SIMD_LV >= 320
        return _mm256_mask_blend_epi8(Mask, this->Data, other.Data);
# else
        else if constexpr ((Mask & 0xaaaaaaaau) == ((Mask & 0x55555555) << 1)) // actually 16bit
        {
            constexpr uint16_t mask16 = static_cast<uint16_t>(TrimMask(Mask, 32, 2));
            if constexpr ((mask16 & 0xff) == (mask16 >> 8))
                return _mm256_blend_epi16(this->Data, other.Data, static_cast<uint8_t>(mask16));
            else if constexpr ((mask16 & 0xaaaa) == ((mask16 & 0x5555) << 1))
            {
                constexpr uint8_t mask = static_cast<uint8_t>(TrimMask(mask16, 16, 2));
                return _mm256_blend_epi32(this->Data, other.Data, mask);
            }
            else
            {
                const auto lo = _mm_blend_epi16(_mm256_castsi256_si128(this->Data), _mm256_castsi256_si128(other.Data), static_cast<uint8_t>(mask16));
                const auto hi = _mm256_blend_epi16(this->Data, other.Data, static_cast<uint8_t>(mask16 >> 8));
                return _mm256_insertf128_si256(hi, lo, 0);
            }
        }
        else
        {
#   ifdef CMSIMD_LESS_SPACE
            const auto maskLo = _mm_insert_epi64(_mm_loadu_si64(&::common::simd::detail::FullMask64[ Mask        & 0xff]), static_cast<int64_t>(::common::simd::detail::FullMask64[(Mask >>  8) & 0xff]), 1);
            const auto maskHi = _mm_insert_epi64(_mm_loadu_si64(&::common::simd::detail::FullMask64[(Mask >> 16) & 0xff]), static_cast<int64_t>(::common::simd::detail::FullMask64[(Mask >> 24) & 0xff]), 1);
            return _mm256_blendv_epi8(this->Data, other.Data, _mm256_set_m128i(maskHi, maskLo));
#   else
            alignas(__m256i) static constexpr uint64_t mask[4] =
            { 
                ::common::simd::detail::FullMask64[Mask         & 0xff], ::common::simd::detail::FullMask64[(Mask >> 8)  & 0xff], 
                ::common::simd::detail::FullMask64[(Mask >> 16) & 0xff], ::common::simd::detail::FullMask64[(Mask >> 24) & 0xff]
            };
            return _mm256_blendv_epi8(this->Data, other.Data, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(mask)));
#   endif
        }
# endif
    }

    // arithmetic operations
    forceinline T VECCALL Add(const T& other) const noexcept { return _mm256_add_epi8(this->Data, other.Data); }
    forceinline T VECCALL Sub(const T& other) const noexcept { return _mm256_sub_epi8(this->Data, other.Data); }
    forceinline T VECCALL ShiftLeftLogic(const uint8_t bits) const noexcept
    {
        if (bits >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm256_set1_epi8(static_cast<uint8_t>(0xff << bits));
            const auto shift16 = _mm256_sll_epi16(this->Data, _mm_cvtsi32_si128(bits));
            return _mm256_and_si256(shift16, mask);
        }
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
    {
        if (bits >= 8)
            return T::AllZero();
        else
        {
            const auto mask = _mm256_set1_epi8(static_cast<uint8_t>(0xff >> bits));
            const auto shift16 = _mm256_srl_epi16(this->Data, _mm_cvtsi32_si128(bits));
            return _mm256_and_si256(shift16, mask);
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
            const auto mask = _mm256_set1_epi8(static_cast<uint8_t>(0xff << N));
            const auto shift16 = _mm256_slli_epi16(this->Data, N);
            return _mm256_and_si256(shift16, mask);
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
            const auto mask = _mm256_set1_epi8(static_cast<uint8_t>(0xff >> N));
            const auto shift16 = _mm256_srli_epi16(this->Data, N);
            return _mm256_and_si256(shift16, mask);
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept;
#endif
};


}


struct alignas(__m256d) F64x4 : public detail::AVX256Shared<F64x4, double>
{
    using EleType = double;
    using VecType = __m256d;
    static constexpr size_t Count = 4;
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,64,4,0 };
    union
    {
        __m256d Data;
        double Val[4];
    };
    forceinline constexpr F64x4() noexcept : Data() { }
    forceinline explicit F64x4(const double* ptr) noexcept : Data(_mm256_loadu_pd(ptr)) { }
    forceinline constexpr F64x4(const __m256d val) noexcept : Data(val) { }
    forceinline F64x4(const double val) noexcept : Data(_mm256_set1_pd(val)) { }
    forceinline F64x4(const Pack<F64x2, 2>& pack) noexcept : Data(_mm256_set_m128d(pack[1].Data, pack[0].Data)) {}
    forceinline F64x4(const double lo0, const double lo1, const double lo2, const double hi3) noexcept : Data(_mm256_setr_pd(lo0, lo1, lo2, hi3)) { }
    forceinline constexpr operator const __m256d&() const noexcept { return Data; }
    forceinline void VECCALL Load(const double *ptr) noexcept { Data = _mm256_loadu_pd(ptr); }
    forceinline void VECCALL Save(double *ptr) const noexcept { _mm256_storeu_pd(ptr, Data); }

    forceinline static F64x4 VECCALL AllZero() noexcept { return _mm256_setzero_pd(); }
    forceinline static F64x4 VECCALL LoadLoLane(const F64x2& lane) noexcept { return _mm256_zextpd128_pd256(lane); }
    forceinline static F64x4 VECCALL LoadLo(const double val) noexcept { return LoadLoLane(F64x2::LoadLo(val)); }
    forceinline static F64x4 VECCALL LoadLo(const double* ptr) noexcept { return LoadLoLane(F64x2::LoadLo(ptr)); }
    forceinline static F64x4 VECCALL BroadcastLane(const F64x2& lane) noexcept { return _mm256_broadcast_pd(&lane.Data); }
    forceinline static F64x4 VECCALL Combine(const F64x2& lo, const F64x2& hi) noexcept { return Pack<F64x2, 2>{lo, hi}; }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline F64x4 VECCALL Shuffle() const noexcept
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
            const auto tmpa = PermuteLane<lane0 ? 1 : 0, lane2 ? 1 : 0>();
            const auto tmpb = PermuteLane<lane1 ? 1 : 0, lane3 ? 1 : 0>();
            return _mm256_shuffle_pd(tmpa, tmpb, (off3 << 3) + (off2 << 2) + (off1 << 1) + off0);
#endif
        }
    }
    template<uint8_t Lo, uint8_t Hi>
    forceinline F64x4 VECCALL ShuffleLane() const noexcept // no cross lane
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        return _mm256_permute_pd(Data, (Hi << 3) + (Lo << 2) + (Hi << 1) + Lo);
    }
    forceinline F64x4 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const noexcept
    {
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
            const auto hilo = PermuteLane<1, 0>(); // 0123->2301
            const auto lohiShuffle = _mm256_permutevar_pd(Data, offMask);
            const auto hiloShuffle = _mm256_permutevar_pd(hilo, offMask);
            return _mm256_blendv_pd(lohiShuffle, hiloShuffle, _mm256_castsi256_pd(laneMask));
#endif
        }
    }
    template<uint8_t Idx>
    forceinline F64x4 VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        return ShuffleLane<Idx, Idx>();
    }
    template<uint8_t Idx>
    forceinline F64x4 VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
        return Shuffle<Idx, Idx, Idx, Idx>();
#else
        const auto lanes = BroadcastLane<Idx % 2>();
        constexpr uint8_t lohi = Idx >= 2 ? 1 : 0;
        return lanes.template PermuteLane<lohi, lohi>();
#endif
    }
    forceinline F64x4 VECCALL ZipLoLane(const F64x4& other) const noexcept { return _mm256_unpacklo_pd(this->Data, other.Data); }
    forceinline F64x4 VECCALL ZipHiLane(const F64x4& other) const noexcept { return _mm256_unpackhi_pd(this->Data, other.Data); }
    forceinline F64x4 VECCALL ZipOdd(const F64x4& other) const noexcept { return ZipLoLane(other); }
    forceinline F64x4 VECCALL ZipEven(const F64x4& other) const noexcept { return ZipHiLane(other); }
    template<MaskType Msk>
    forceinline F64x4 VECCALL SelectWith(const F64x4& other, F64x4 mask) const noexcept
    {
        return _mm256_blendv_pd(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F64x4 VECCALL SelectWith(const F64x4& other) const noexcept
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
        return _mm256_blend_pd(this->Data, other.Data, Mask);
    }
#if COMMON_SIMD_LV >= 200
    template<uint8_t Cnt>
    forceinline F64x4 VECCALL MoveLaneToHi() const noexcept
    {
        static_assert(Cnt <= 2, "move count should be in [0,2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 2) return AllZero();
        else return _mm256_castsi256_pd(_mm256_slli_si256(_mm256_castpd_si256(Data), Cnt * 8));
    }
    template<uint8_t Cnt>
    forceinline F64x4 VECCALL MoveLaneToLo() const noexcept
    {
        static_assert(Cnt <= 2, "move count should be in [0,2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 2) return AllZero();
        else return _mm256_castsi256_pd(_mm256_srli_si256(_mm256_castpd_si256(Data), Cnt * 8));
    }
    forceinline F64x4 VECCALL MoveLaneHiToLo() const noexcept { return MoveLaneToLo<1>(); }
    template<uint8_t Cnt>
    forceinline F64x4 VECCALL MoveLaneToLoWith(const F64x4& hi) const noexcept
    {
        static_assert(Cnt <= 2, "move count should be in [0,2]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 2) return hi;
        else return _mm256_shuffle_pd(Data, hi.Data, 0b0101);
    }
#endif

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F64x4 VECCALL Compare(const F64x4 other) const noexcept
    {
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm256_cmp_pd(Data, other.Data, cmpImm);
    }

    // logic operations
    forceinline F64x4 VECCALL And   (const F64x4& other) const noexcept { return _mm256_and_pd   (Data, other.Data); }
    forceinline F64x4 VECCALL Or    (const F64x4& other) const noexcept { return _mm256_or_pd    (Data, other.Data); }
    forceinline F64x4 VECCALL Xor   (const F64x4& other) const noexcept { return _mm256_xor_pd   (Data, other.Data); }
    forceinline F64x4 VECCALL AndNot(const F64x4& other) const noexcept { return _mm256_andnot_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Not() const noexcept
    {
        return _mm256_xor_pd(Data, _mm256_castsi256_pd(_mm256_set1_epi64x(-1)));
    }

    // arithmetic operations
    forceinline F64x4 VECCALL Add(const F64x4& other) const noexcept { return _mm256_add_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Sub(const F64x4& other) const noexcept { return _mm256_sub_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Mul(const F64x4& other) const noexcept { return _mm256_mul_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Div(const F64x4& other) const noexcept { return _mm256_div_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Neg() const noexcept { return Xor(_mm256_castsi256_pd(_mm256_set1_epi64x(INT64_MIN))); }
    forceinline F64x4 VECCALL Abs() const noexcept { return And(_mm256_castsi256_pd(_mm256_set1_epi64x(INT64_MAX))); }
    forceinline F64x4 VECCALL Max(const F64x4& other) const noexcept { return _mm256_max_pd(Data, other.Data); }
    forceinline F64x4 VECCALL Min(const F64x4& other) const noexcept { return _mm256_min_pd(Data, other.Data); }
    //F64x4 Rcp() const { return _mm256_rcp_pd(Data); }
    forceinline F64x4 VECCALL Sqrt() const { return _mm256_sqrt_pd(Data); }
    //F64x4 Rsqrt() const { return _mm256_rsqrt_pd(Data); }

    forceinline F64x4 VECCALL MulAdd(const F64x4& muler, const F64x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_pd(Data, muler, adder);
#else
        return _mm256_add_pd(_mm256_mul_pd(Data, muler), adder);
#endif
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulAddLane(const F64x4& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulAdd(muler.BroadcastLane<Idx>(), adder);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulAdd(const F64x2& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        const auto full = muler.Broadcast<Idx>();
        return MulAdd(F64x4(full, full), adder);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulScalarAdd(const F64x4& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x4 VECCALL MulSub(const F64x4& muler, const F64x4& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_pd(Data, muler, suber);
#else
        return _mm256_sub_pd(_mm256_mul_pd(Data, muler), suber);
#endif
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulSubLane(const F64x4& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulSub(muler.BroadcastLane<Idx>(), suber);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulSub(const F64x2& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        const auto full = muler.Broadcast<Idx>();
        return MulSub(F64x4(full, full), suber);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL MulScalarSub(const F64x4& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F64x4 VECCALL NMulAdd(const F64x4& muler, const F64x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_pd(Data, muler, adder);
#else
        return _mm256_sub_pd(adder, _mm256_mul_pd(Data, muler));
#endif
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulAddLane(const F64x4& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulAdd(muler.BroadcastLane<Idx>(), adder);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulAdd(const F64x2& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        const auto full = muler.Broadcast<Idx>();
        return NMulAdd(F64x4(full, full), adder);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulScalarAdd(const F64x4& muler, const F64x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F64x4 VECCALL NMulSub(const F64x4& muler, const F64x4& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_pd(Data, muler, suber);
#else
        return _mm256_xor_pd(_mm256_add_pd(_mm256_mul_pd(Data, muler), suber), _mm256_set1_pd(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulSubLane(const F64x4& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return NMulSub(muler.BroadcastLane<Idx>(), suber);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulSub(const F64x2& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        const auto full = muler.Broadcast<Idx>();
        return NMulSub(F64x4(full, full), suber);
    }
    template<size_t Idx>
    forceinline F64x4 VECCALL NMulScalarSub(const F64x4& muler, const F64x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    forceinline F64x4 VECCALL operator*(const F64x4& other) const noexcept { return Mul(other); }
    forceinline F64x4 VECCALL operator/(const F64x4& other) const noexcept { return Div(other); }
    forceinline F64x4& VECCALL operator*=(const F64x4& other) noexcept { Data = Mul(other); return *this; }
    forceinline F64x4& VECCALL operator/=(const F64x4& other) noexcept { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F64x2, T>(), typename... Args>
    forceinline typename CastTyper<F64x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F64x4 VECCALL Round() const
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm256_round_pd(Data, imm8);
    }

    forceinline F64x2 VECCALL GetLoLane() const noexcept { return _mm256_castpd256_pd128(Data); /*_mm256_extractf128_pd(Data, 0);*/ }
    forceinline F64x2 VECCALL GetHiLane() const noexcept { return _mm256_extractf128_pd(Data, 1); }
};


struct alignas(__m256) F32x8 : public detail::AVX256Shared<F32x8, float>
{
    using EleType = float;
    using VecType = __m256;
    static constexpr size_t Count = 8;
    static constexpr VecDataInfo VDInfo = { VecDataInfo::DataTypes::Float,32,8,0 };
    union
    {
        __m256 Data;
        float Val[8];
    };
    forceinline constexpr F32x8() : Data() { }
    forceinline explicit F32x8(const float* ptr) noexcept : Data(_mm256_loadu_ps(ptr)) { }
    forceinline constexpr F32x8(const __m256 val) noexcept : Data(val) { }
    forceinline F32x8(const float val) noexcept : Data(_mm256_set1_ps(val)) { }
    forceinline F32x8(const Pack<F32x4, 2>& pack) noexcept : Data(_mm256_set_m128(pack[1].Data, pack[0].Data)) {}
    forceinline F32x8(const float lo0, const float lo1, const float lo2, const float lo3, const float lo4, const float lo5, const float lo6, const float hi7) noexcept
        : Data(_mm256_setr_ps(lo0, lo1, lo2, lo3, lo4, lo5, lo6, hi7)) { }
    forceinline constexpr operator const __m256& () const noexcept { return Data; }
    forceinline void VECCALL Load(const float* ptr) noexcept { Data = _mm256_loadu_ps(ptr); }
    forceinline void VECCALL Save(float* ptr) const noexcept { _mm256_storeu_ps(ptr, Data); }

    forceinline static F32x8 AllZero() noexcept { return _mm256_setzero_ps(); }
    forceinline static F32x8 LoadLoLane(const F32x4& lane) noexcept { return _mm256_zextps128_ps256(lane); }
    forceinline static F32x8 LoadLo(const float val) noexcept { return LoadLoLane(F32x4::LoadLo(val)); }
    forceinline static F32x8 LoadLo(const float* ptr) noexcept { return LoadLoLane(F32x4::LoadLo(ptr)); }
    forceinline static F32x8 BroadcastLane(const F32x4& lane) noexcept { return _mm256_broadcast_ps(&lane.Data); }
    forceinline static F32x8 Combine(const F32x4& lo, const F32x4& hi) noexcept { return Pack<F32x4, 2>{lo, hi}; }

    // shuffle operations
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline F32x8 VECCALL ShuffleLane() const noexcept // no cross lane
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        return _mm256_permute_ps(Data, (Hi3 << 6) + (Lo2 << 4) + (Lo1 << 2) + Lo0);
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline F32x8 VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        if constexpr (Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Lo3 < 4 &&
            Lo4 - Lo0 == 4 && Lo5 - Lo1 == 4 && Lo6 - Lo2 == 4 && Hi7 - Lo3 == 4) // no cross lane and same shuffle for two lane
        {
            return ShuffleLane<Lo0, Lo1, Lo2, Lo3>();
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
                const auto tmpa = PermuteLane<lane0 ? 1 : 0, lane4 ? 1 : 0>();
                const auto tmpb = PermuteLane<lane2 ? 1 : 0, lane6 ? 1 : 0>();
                return _mm256_shuffle_ps(tmpa, tmpb, (off3 << 6) + (off2 << 4) + (off1 << 2) + off0);
            }
            else
            {
                const auto tmpa = PermuteLane<lane0 ? 1 : 0, lane5 ? 1 : 0>();
                const auto tmpb = PermuteLane<lane2 ? 1 : 0, lane7 ? 1 : 0>();
                const auto f0257 = _mm256_shuffle_ps(tmpa, tmpb, (off7 << 6) + (off2 << 4) + (off5 << 2) + off0);
                const auto tmpc = PermuteLane<lane1 ? 1 : 0, lane4 ? 1 : 0>();
                const auto tmpd = PermuteLane<lane3 ? 1 : 0, lane6 ? 1 : 0>();
                const auto f1346 = _mm256_shuffle_ps(tmpc, tmpd, (off6 << 6) + (off3 << 4) + (off4 << 2) + off1);
                return _mm256_blend_ps(f0257, f1346, 0b01011010);
            }
#endif
        }
    }
    forceinline F32x8 VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const noexcept
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
            const auto hilo = PermuteLane<1, 0>(); // 01234567->45670123
            const auto lohiShuffle = _mm256_permutevar_ps(Data, offMask);
            const auto hiloShuffle = _mm256_permutevar_ps(hilo, offMask);
            return _mm256_blendv_ps(lohiShuffle, hiloShuffle, _mm256_castsi256_ps(laneMask));
        }
#endif
    }
    template<uint8_t Idx>
    forceinline F32x8 VECCALL BroadcastLane() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]");
        return ShuffleLane<Idx, Idx, Idx, Idx>();
    }
    template<uint8_t Idx>
    forceinline F32x8 VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 8, "shuffle index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
        return Shuffle<Idx, Idx, Idx, Idx, Idx, Idx, Idx, Idx>();
#else
        const auto lanes = BroadcastLane<Idx % 4>();
        constexpr uint8_t lohi = Idx >= 4 ? 1 : 0;
        return lanes.template PermuteLane<lohi, lohi>();
#endif
    }
    forceinline F32x8 VECCALL ZipLoLane(const F32x8& other) const noexcept { return _mm256_unpacklo_ps(this->Data, other.Data); }
    forceinline F32x8 VECCALL ZipHiLane(const F32x8& other) const noexcept { return _mm256_unpackhi_ps(this->Data, other.Data); }
    forceinline F32x8 VECCALL ZipOdd(const F32x8& other) const noexcept
    {
        return SelectWith<0xaa>(_mm256_castsi256_ps(_mm256_slli_epi64(_mm256_castps_si256(other.Data), 32)));
    }
    forceinline F32x8 VECCALL ZipEven(const F32x8& other) const noexcept
    {
        return other.template SelectWith<0x55>(_mm256_castsi256_ps(_mm256_srli_epi64(_mm256_castps_si256(Data), 32)));
    }
    template<MaskType Msk>
    forceinline F32x8 VECCALL SelectWith(const F32x8& other, F32x8 mask) const noexcept
    {
        return _mm256_blendv_ps(this->Data, other.Data, mask);
    }
    template<uint8_t Mask>
    forceinline F32x8 VECCALL SelectWith(const F32x8& other) const noexcept
    {
        return _mm256_blend_ps(this->Data, other.Data, Mask);
    }
#if COMMON_SIMD_LV >= 200
    template<uint8_t Cnt>
    forceinline F32x8 VECCALL MoveLaneToHi() const noexcept
    {
        static_assert(Cnt <= 4, "move count should be in [0,4]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 4) return AllZero();
        else return _mm256_castsi256_ps(_mm256_slli_si256(_mm256_castps_si256(Data), Cnt * 4));
    }
    template<uint8_t Cnt>
    forceinline F32x8 VECCALL MoveLaneToLo() const noexcept
    {
        static_assert(Cnt <= 4, "move count should be in [0,4]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 4) return AllZero();
        else return _mm256_castsi256_ps(_mm256_srli_si256(_mm256_castps_si256(Data), Cnt * 4));
    }
    forceinline F32x8 VECCALL MoveLaneHiToLo() const noexcept { return MoveLaneToLo<2>(); }
    template<uint8_t Cnt>
    forceinline F32x8 VECCALL MoveLaneToLoWith(const F32x8& hi) const noexcept
    {
        static_assert(Cnt <= 4, "move count should be in [0,4]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == 4) return hi;
        else if constexpr (Cnt == 2) return _mm256_shuffle_ps(Data, hi.Data, 0b01001110);
        else return _mm256_castsi256_ps(_mm256_alignr_epi8(_mm256_castps_si256(hi.Data), _mm256_castps_si256(Data), Cnt * 4));
    }
#endif

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F32x8 VECCALL Compare(const F32x8 other) const noexcept
    {
        constexpr auto cmpImm = detail::CmpTypeImm(Cmp);
        return _mm256_cmp_ps(Data, other.Data, cmpImm);
    }

    // logic operations
    forceinline F32x8 VECCALL And   (const F32x8& other) const noexcept { return _mm256_and_ps   (Data, other.Data); }
    forceinline F32x8 VECCALL Or    (const F32x8& other) const noexcept { return _mm256_or_ps    (Data, other.Data); }
    forceinline F32x8 VECCALL Xor   (const F32x8& other) const noexcept { return _mm256_xor_ps   (Data, other.Data); }
    forceinline F32x8 VECCALL AndNot(const F32x8& other) const noexcept { return _mm256_andnot_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Not() const
    {
        return _mm256_xor_ps(Data, _mm256_castsi256_ps(_mm256_set1_epi32(-1)));
    }

    // arithmetic operations
    forceinline F32x8 VECCALL Add(const F32x8& other) const noexcept { return _mm256_add_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Sub(const F32x8& other) const noexcept { return _mm256_sub_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Mul(const F32x8& other) const noexcept { return _mm256_mul_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Div(const F32x8& other) const noexcept { return _mm256_div_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Neg() const noexcept { return Xor(_mm256_castsi256_ps(_mm256_set1_epi32(INT32_MIN))); }
    forceinline F32x8 VECCALL Abs() const noexcept { return And(_mm256_castsi256_ps(_mm256_set1_epi32(INT32_MAX))); }
    forceinline F32x8 VECCALL Max(const F32x8& other) const noexcept { return _mm256_max_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Min(const F32x8& other) const noexcept { return _mm256_min_ps(Data, other.Data); }
    forceinline F32x8 VECCALL Rcp() const noexcept { return _mm256_rcp_ps(Data); }
    forceinline F32x8 VECCALL Sqrt() const noexcept { return _mm256_sqrt_ps(Data); }
    forceinline F32x8 VECCALL Rsqrt() const noexcept { return _mm256_rsqrt_ps(Data); }

    forceinline F32x8 VECCALL MulAdd(const F32x8& muler, const F32x8& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmadd_ps(Data, muler, adder);
#else
        return _mm256_add_ps(_mm256_mul_ps(Data, muler), adder);
#endif
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulAddLane(const F32x8& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulAdd(muler.BroadcastLane<Idx>(), adder);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulAdd(const F32x4& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        const auto full = muler.Broadcast<Idx>();
        return MulAdd(F32x8(full, full), adder);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulScalarAdd(const F32x8& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 8, "select index should be in [0,7]");
        return MulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x8 VECCALL MulSub(const F32x8& muler, const F32x8& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fmsub_ps(Data, muler, suber);
#else
        return _mm256_sub_ps(_mm256_mul_ps(Data, muler), suber);
#endif
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulSubLane(const F32x8& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSub(muler.BroadcastLane<Idx>(), suber);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulSub(const F32x4& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        const auto full = muler.Broadcast<Idx>();
        return MulSub(F32x8(full, full), suber);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL MulScalarSub(const F32x8& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 8, "select index should be in [0,7]");
        return MulSub(muler.Broadcast<Idx>(), suber);
    }
    forceinline F32x8 VECCALL NMulAdd(const F32x8& muler, const F32x8& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmadd_ps(Data, muler, adder);
#else
        return _mm256_sub_ps(adder, _mm256_mul_ps(Data, muler));
#endif
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulAddLane(const F32x8& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulAdd(muler.BroadcastLane<Idx>(), adder);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulAdd(const F32x4& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        const auto full = muler.Broadcast<Idx>();
        return NMulAdd(F32x8(full, full), adder);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulScalarAdd(const F32x8& muler, const F32x8& adder) const noexcept
    {
        static_assert(Idx < 8, "select index should be in [0,7]");
        return NMulAdd(muler.Broadcast<Idx>(), adder);
    }
    forceinline F32x8 VECCALL NMulSub(const F32x8& muler, const F32x8& suber) const noexcept
    {
#if COMMON_SIMD_LV >= 150
        return _mm256_fnmsub_ps(Data, muler, suber);
#else
        return _mm256_xor_ps(_mm256_add_ps(_mm256_mul_ps(Data, muler), suber), _mm256_set1_ps(-0.));
#endif
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulSubLane(const F32x8& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulSub(muler.BroadcastLane<Idx>(), suber);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulSub(const F32x4& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        const auto full = muler.Broadcast<Idx>();
        return NMulSub(F32x8(full, full), suber);
    }
    template<size_t Idx>
    forceinline F32x8 VECCALL NMulScalarSub(const F32x8& muler, const F32x8& suber) const noexcept
    {
        static_assert(Idx < 8, "select index should be in [0,7]");
        return NMulSub(muler.Broadcast<Idx>(), suber);
    }

    template<DotPos Mul, DotPos Res>
    forceinline F32x8 VECCALL Dot2(const F32x8& other) const noexcept
    { 
        return _mm256_dp_ps(Data, other.Data, static_cast<uint8_t>(Mul) << 4 | static_cast<uint8_t>(Res));
    }
    forceinline F32x8 VECCALL operator*(const F32x8& other) const noexcept { return Mul(other); }
    forceinline F32x8 VECCALL operator/(const F32x8& other) const noexcept { return Div(other); }
    forceinline F32x8& VECCALL operator*=(const F32x8& other) noexcept { Data = Mul(other); return *this; }
    forceinline F32x8& VECCALL operator/=(const F32x8& other) noexcept { Data = Div(other); return *this; }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F32x8, T>(), typename... Args>
    forceinline typename CastTyper<F32x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F32x8 VECCALL Round() const noexcept
    {
        constexpr auto imm8 = detail::RoundModeImm(Mode) | _MM_FROUND_NO_EXC;
        return _mm256_round_ps(Data, imm8);
    }

    forceinline F32x4 VECCALL GetLoLane() const noexcept { return _mm256_castps256_ps128(Data); /*_mm256_extractf128_ps(Data, 0);*/ }
    forceinline F32x4 VECCALL GetHiLane() const noexcept { return _mm256_extractf128_ps(Data, 1); }
};


struct alignas(32) I64x4 : public detail::Common64x4<I64x4, I64x2, int64_t>
{
    using Common64x4<I64x4, I64x2, int64_t>::Common64x4;

#if COMMON_SIMD_LV >= 200
    template<CompareType Cmp, MaskType Msk>
    forceinline I64x4 VECCALL Compare(const I64x4 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm256_cmpeq_epi64(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm256_cmpgt_epi64(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I64x4 VECCALL Neg() const noexcept { return _mm256_sub_epi64(_mm256_setzero_si256(), Data); }
    forceinline I64x4 VECCALL SatAdd(const I64x4& other) const noexcept
    {
        const auto added = Add(other);
# if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,1,0,0,0,0,1,0 -> 0x42
        const auto overflow = _mm256_ternarylogic_epi64(this->Data, other.Data, added, 0x42);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm256_mask_add_epi64(added, _mm256_movepi64_mask(overflow), ShiftRightLogic<63>(), I64x4(INT64_MAX));
# else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.AndNot(Xor(added)); // !diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<63>().Add(INT64_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
# endif
    }
    forceinline I64x4 VECCALL SatSub(const I64x4& other) const noexcept
    {
        const auto added = Sub(other);
# if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,0,0,1,1,0,0,0 -> 0x18
        const auto overflow = _mm256_ternarylogic_epi64(this->Data, other.Data, added, 0x18);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm256_mask_add_epi64(added, _mm256_movepi64_mask(overflow), ShiftRightLogic<63>(), I64x4(INT64_MAX));
# else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.And(Xor(added)); // diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<63>().Add(INT64_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
# endif
    }
    forceinline I64x4 VECCALL Max(const I64x4& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_max_epi64(Data, other.Data);
# else
        return _mm256_blendv_epi8(other.Data, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline I64x4 VECCALL Min(const I64x4& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_min_epi64(Data, other.Data);
# else
        return _mm256_blendv_epi8(Data, other.Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline I64x4 VECCALL Abs() const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_abs_epi64(Data);
# else
        const auto neg = Neg().As<F64x4>(), self = As<F64x4>();
        return _mm256_castpd_si256(_mm256_blendv_pd(self, neg, self));
# endif
    }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I64x4, T>(), typename... Args>
    forceinline typename CastTyper<I64x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};

struct alignas(32) U64x4 : public detail::Common64x4<U64x4, U64x2, uint64_t>
{
    using Common64x4<U64x4, U64x2, uint64_t>::Common64x4;

#if COMMON_SIMD_LV >= 200
    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U64x4 VECCALL Compare(const U64x4 other) const noexcept
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I64x4>().Compare<Cmp, Msk>(other.As<I64x4>()).template As<U64x4>();
        }
        else
        {
            const U64x4 sigMask(static_cast<uint64_t>(0x8000000000000000));
            return Xor(sigMask).As<I64x4>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I64x4>()).template As<U64x4>();
        }
    }

    // arithmetic operations
    forceinline U64x4 VECCALL SatAdd(const U64x4& other) const noexcept
    {
        return Min(other.Not()).Add(other);
    }
    forceinline U64x4 VECCALL SatSub(const U64x4& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return Max(other).Sub(other);
# else
        return Sub(other).And(Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline U64x4 VECCALL Max(const U64x4& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_max_epu64(Data, other.Data);
# else
        return _mm256_blendv_epi8(other.Data, Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline U64x4 VECCALL Min(const U64x4& other) const noexcept
    {
# if COMMON_SIMD_LV >= 320
        return _mm256_min_epu64(Data, other.Data);
# else
        return _mm256_blendv_epi8(Data, other.Data, Compare<CompareType::GreaterThan, MaskType::FullEle>(other));
# endif
    }
    forceinline U64x4 VECCALL Abs() const noexcept { return Data; }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U64x4, T>(), typename... Args>
    forceinline typename CastTyper<U64x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};


struct alignas(32) I32x8 : public detail::Common32x8<I32x8, I32x4, int32_t>
{
    using Common32x8<I32x8, I32x4, int32_t>::Common32x8;

#if COMMON_SIMD_LV >= 200
    template<CompareType Cmp, MaskType Msk>
    forceinline I32x8 VECCALL Compare(const I32x8 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm256_cmpeq_epi32(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm256_cmpgt_epi32(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I32x8 VECCALL Neg() const noexcept { return _mm256_sub_epi32(_mm256_setzero_si256(), Data); }
    forceinline I32x8 VECCALL SatAdd(const I32x8& other) const noexcept
    {
        const auto added = Add(other);
# if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,1,0,0,0,0,1,0 -> 0x42
        const auto overflow = _mm256_ternarylogic_epi32(this->Data, other.Data, added, 0x42);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm256_mask_add_epi32(added, _mm256_movepi32_mask(overflow), ShiftRightLogic<31>(), I32x8(INT32_MAX));
# else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.AndNot(Xor(added)); // !diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<31>().Add(INT32_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
# endif
    }
    forceinline I32x8 VECCALL SatSub(const I32x8& other) const noexcept
    {
        const auto added = Sub(other);
# if COMMON_SIMD_LV >= 320
        // this|other|added -> 000,001,010,011,100,101,110,111 -> this == other && this != added -> 0,0,0,1,1,0,0,0 -> 0x18
        const auto overflow = _mm256_ternarylogic_epi32(this->Data, other.Data, added, 0x18);
        // 0 -> 0x7fffffff(MAX), 1 -> 0x80000000(MIN) => MAX + signbit
        return _mm256_mask_add_epi32(added, _mm256_movepi32_mask(overflow), ShiftRightLogic<31>(), I32x8(INT32_MAX));
# else
        const auto diffFlag = Xor(other);
        const auto overflow = diffFlag.And(Xor(added)); // diffFlag && (this != added)
        // 0 -> 0x7f...f(MAX), 1 -> 0x80...0(MIN) => MAX + signbit
        const auto satVal = ShiftRightLogic<31>().Add(INT32_MAX);
        return added.SelectWith<MaskType::SigBit>(satVal, overflow);
# endif
    }
    forceinline I32x8 VECCALL Abs() const noexcept { return _mm256_abs_epi32(Data); }
    forceinline I32x8 VECCALL Max(const I32x8& other) const noexcept { return _mm256_max_epi32(Data, other.Data); }
    forceinline I32x8 VECCALL Min(const I32x8& other) const noexcept { return _mm256_min_epi32(Data, other.Data); }
    forceinline Pack<I64x4, 2> VECCALL MulX(const I32x8& other) const noexcept
    {
        // 02,46 | 13,57
        const auto even = _mm256_mul_epi32(Data, other.Data), odd = _mm256_mul_epi32(_mm256_srli_epi64(Data, 32), _mm256_srli_epi64(other.Data, 32));
        // 01,45 | 23,67
        const auto evenLane = _mm256_unpacklo_epi64(even, odd), oddLane = _mm256_unpackhi_epi64(even, odd);
        // 01,23 | 45,67
        return { _mm256_permute2x128_si256(evenLane, oddLane, 0x20), _mm256_permute2x128_si256(evenLane, oddLane, 0x31) };
    }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I32x8, T>(), typename... Args>
    forceinline typename CastTyper<I32x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<I64x4, 2> VECCALL I32x8::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtepi32_epi64(_mm256_castsi256_si128(Data)), _mm256_cvtepi32_epi64(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U64x4, 2> VECCALL I32x8::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
#endif
template<> forceinline F32x8 VECCALL I32x8::Cast<F32x8>() const noexcept
{
    return _mm256_cvtepi32_ps(Data);
}
template<> forceinline Pack<F64x4, 2> VECCALL I32x8::Cast<F64x4>() const noexcept
{
    return { _mm256_cvtepi32_pd(_mm256_extractf128_si256(Data, 0)), _mm256_cvtepi32_pd(_mm256_extractf128_si256(Data, 1)) };
}


struct alignas(__m256i) U32x8 : public detail::Common32x8<U32x8, U32x4, uint32_t>
{
    using Common32x8<U32x8, U32x4, uint32_t>::Common32x8;

#if COMMON_SIMD_LV >= 200
    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U32x8 VECCALL Compare(const U32x8 other) const noexcept
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I32x8>().Compare<Cmp, Msk>(other.As<I32x8>()).template As<U32x8>();
        }
        else
        {
            const U32x8 sigMask(static_cast<uint32_t>(0x80000000));
            return Xor(sigMask).As<I32x8>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I32x8>()).template As<U32x8>();
        }
    }

    // arithmetic operations
    forceinline U32x8 VECCALL SatAdd(const U32x8& other) const noexcept { return Min(other.Not()).Add(other); }
    forceinline U32x8 VECCALL SatSub(const U32x8& other) const noexcept { return Max(other).Sub(other); }
    forceinline U32x8 VECCALL Max(const U32x8& other) const noexcept { return _mm256_max_epu32(Data, other.Data); }
    forceinline U32x8 VECCALL Min(const U32x8& other) const noexcept { return _mm256_min_epu32(Data, other.Data); }
    forceinline U32x8 VECCALL Abs() const noexcept { return Data; }
    forceinline Pack<U64x4, 2> VECCALL MulX(const U32x8& other) const noexcept
    {
        // 02,46 | 13,57
        const auto even = _mm256_mul_epu32(Data, other.Data), odd = _mm256_mul_epu32(_mm256_srli_epi64(Data, 32), _mm256_srli_epi64(other.Data, 32));
        // 01,45 | 23,67
        const auto evenLane = _mm256_unpacklo_epi64(even, odd), oddLane = _mm256_unpackhi_epi64(even, odd);
        // 01,23 | 45,67
        return { _mm256_permute2x128_si256(evenLane, oddLane, 0x20), _mm256_permute2x128_si256(evenLane, oddLane, 0x31) };
    }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U32x8, T>(), typename... Args>
    forceinline typename CastTyper<U32x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I64x4, 2> VECCALL U32x8::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    return { _mm256_cvtepu32_epi64(_mm256_castsi256_si128(Data)), _mm256_cvtepu32_epi64(_mm256_extracti128_si256(Data, 1)) };
#else
    const auto data = _mm256_castsi256_ps(Data), zero = _mm256_castsi256_ps(_mm256_setzero_si256());
    const auto dat0145 = _mm256_unpacklo_ps(data, zero), dat2367 = _mm256_unpackhi_ps(data, zero);
    return
    {
        _mm256_castps_si256(_mm256_permute2f128_ps(dat0145, dat2367, 0x20)),
        _mm256_castps_si256(_mm256_permute2f128_ps(dat0145, dat2367, 0x31))
    };
#endif
}
template<> forceinline Pack<U64x4, 2> VECCALL U32x8::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline F32x8 VECCALL U32x8::Cast<F32x8, CastMode::RangeUndef>() const noexcept
{
# if COMMON_SIMD_LV >= 320
    return _mm512_castps512_ps256(_mm512_cvtepu32_ps(_mm512_castsi256_si512(Data)));
# else
    // use 2 convert to make sure error within 0.5 ULP, see https://stackoverflow.com/a/40766669
    const auto mul16 = _mm256_set1_ps(65536.f);
    const auto lo16 = And(0xffff);
    const auto hi16 = ShiftRightLogic<16>();
    const auto base = hi16.As<I32x8>().Cast<F32x8>();
    const auto addlo = lo16.As<I32x8>().Cast<F32x8>();
    return base.MulAdd(mul16, addlo);
# endif
}
template<> forceinline Pack<F64x4, 2> VECCALL U32x8::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
# if COMMON_SIMD_LV >= 320
    return { _mm256_cvtepu32_pd(_mm256_extractf128_si256(Data, 0)), _mm256_cvtepu32_pd(_mm256_extractf128_si256(Data, 1)) };
# else
    constexpr double Adder32 = 4294967296.f; // UINT32_MAX+1
    // if [sig], will be treated as negative, need to add Adder32
#   if COMMON_SIMD_LV >= 200
    const auto sig0123 = _mm256_castsi256_pd(Shuffle<0, 0, 1, 1, 2, 2, 3, 3>()), sig4567 = _mm256_castsi256_pd(Shuffle<4, 4, 5, 5, 6, 6, 7, 7>());
#   else
    const auto f32x8 = As<F32x8>();
    const auto val0145 = _mm256_permute_ps(f32x8, 0b01010000), val2367 = _mm256_permute_ps(f32x8, 0b11111010);
    const auto sig0145 = _mm256_castps_pd(val0145), sig2367 = _mm256_castps_pd(val2367);
    const auto sig0123 = _mm256_permute2f128_pd(sig0145, sig2367, 0x20), sig4567 = _mm256_permute2f128_pd(sig0145, sig2367, 0x31);
#   endif
    const auto adder1 = _mm256_blendv_pd(_mm256_setzero_pd(), _mm256_set1_pd(Adder32), sig0123);
    const auto adder2 = _mm256_blendv_pd(_mm256_setzero_pd(), _mm256_set1_pd(Adder32), sig4567);
    const auto f64 = As<I32x8>().Cast<F64x4>();
    return { f64[0].Add(adder1), f64[1].Add(adder2) };
# endif
}
#endif


struct alignas(32) I16x16 : public detail::Common16x16<I16x16, I16x8, int16_t>
{
    using Common16x16<I16x16, I16x8, int16_t>::Common16x16;

#if COMMON_SIMD_LV >= 200
    template<CompareType Cmp, MaskType Msk>
    forceinline I16x16 VECCALL Compare(const I16x16 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm256_cmpeq_epi16(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm256_cmpgt_epi16(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I16x16 VECCALL SatAdd(const I16x16& other) const noexcept { return _mm256_adds_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL SatSub(const I16x16& other) const noexcept { return _mm256_subs_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL MulHi(const I16x16& other) const noexcept { return _mm256_mulhi_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL Max(const I16x16& other) const noexcept { return _mm256_max_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL Min(const I16x16& other) const noexcept { return _mm256_min_epi16(Data, other.Data); }
    forceinline I16x16 VECCALL Neg() const noexcept { return _mm256_sub_epi16(_mm256_setzero_si256(), Data); }
    forceinline I16x16 VECCALL Abs() const noexcept { return _mm256_abs_epi16(Data); }
    forceinline Pack<I32x8, 2> VECCALL MulX(const I16x16& other) const noexcept
    {
        const auto lo = MulLo(other), hi = MulHi(other);
        const auto evenLane = _mm256_unpacklo_epi16(lo, hi), oddLane = _mm256_unpackhi_epi16(lo, hi);
        return { _mm256_permute2x128_si256(evenLane, oddLane, 0x20), _mm256_permute2x128_si256(evenLane, oddLane, 0x31) };
    }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I16x16, T>(), typename... Args>
    forceinline typename CastTyper<I16x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<I32x8, 2> VECCALL I16x16::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtepi16_epi32(_mm256_castsi256_si128(Data)), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U32x8, 2> VECCALL I16x16::Cast<U32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().As<U32x8>();
}
template<> forceinline Pack<I64x4, 4> VECCALL I16x16::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    const auto val0 = _mm256_cvtepi16_epi64(_mm256_castsi256_si128  (                  Data       ));
    const auto val1 = _mm256_cvtepi16_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 8)   ));
    const auto val2 = _mm256_cvtepi16_epi64(_mm256_extracti128_si256(                  Data, 1    ));
    const auto val3 = _mm256_cvtepi16_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data, 8), 1));
    return { val0, val1, val2, val3 };
}
template<> forceinline Pack<U64x4, 4> VECCALL I16x16::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
template<> forceinline Pack<F32x8, 2> VECCALL I16x16::Cast<F32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F32x8>();
}
template<> forceinline Pack<F64x4, 4> VECCALL I16x16::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F64x4>();
}
#endif


struct alignas(32) U16x16 : public detail::Common16x16<U16x16, U16x8, uint16_t>
{
    using Common16x16<U16x16, U16x8, uint16_t>::Common16x16;

#if COMMON_SIMD_LV >= 200
    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U16x16 VECCALL Compare(const U16x16 other) const noexcept
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I16x16>().Compare<Cmp, Msk>(other.As<I16x16>()).template As<U16x16>();
        }
        else
        {
            const U16x16 sigMask(static_cast<uint16_t>(0x8000));
            return Xor(sigMask).As<I16x16>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I16x16>()).template As<U16x16>();
        }
    }

    // arithmetic operations
    forceinline U16x16 VECCALL SatAdd(const U16x16& other) const noexcept { return _mm256_adds_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL SatSub(const U16x16& other) const noexcept { return _mm256_subs_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL MulHi(const U16x16& other) const noexcept { return _mm256_mulhi_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL Max(const U16x16& other) const noexcept { return _mm256_max_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL Min(const U16x16& other) const noexcept { return _mm256_min_epu16(Data, other.Data); }
    forceinline U16x16 VECCALL Abs() const noexcept { return Data; }
    forceinline Pack<U32x8, 2> VECCALL MulX(const U16x16& other) const noexcept
    {
        const auto lo = MulLo(other), hi = MulHi(other);
        const auto evenLane = _mm256_unpacklo_epi16(lo, hi), oddLane = _mm256_unpackhi_epi16(lo, hi);
        return { _mm256_permute2x128_si256(evenLane, oddLane, 0x20), _mm256_permute2x128_si256(evenLane, oddLane, 0x31) };
    }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U16x16, T>(), typename... Args>
    forceinline typename CastTyper<U16x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<I32x8, 2> VECCALL U16x16::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtepu16_epi32(_mm256_castsi256_si128(Data)), _mm256_cvtepu16_epi32(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U32x8, 2> VECCALL U16x16::Cast<U32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().As<U32x8>();
}
template<> forceinline Pack<I64x4, 4> VECCALL U16x16::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    const auto val0 = _mm256_cvtepu16_epi64(_mm256_castsi256_si128  (                  Data       ));
    const auto val1 = _mm256_cvtepu16_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 8)   ));
    const auto val2 = _mm256_cvtepu16_epi64(_mm256_extracti128_si256(                  Data, 1    ));
    const auto val3 = _mm256_cvtepu16_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data, 8), 1));
    return { val0, val1, val2, val3 };
}
template<> forceinline Pack<U64x4, 4> VECCALL U16x16::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
template<> forceinline Pack<F32x8, 2> VECCALL U16x16::Cast<F32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F32x8>();
}
template<> forceinline Pack<F64x4, 4> VECCALL U16x16::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F64x4>();
}
#endif


struct alignas(32) I8x32 : public detail::Common8x32<I8x32, I8x16, int8_t>
{
    using Common8x32<I8x32, I8x16, int8_t>::Common8x32;

#if COMMON_SIMD_LV >= 200
    template<CompareType Cmp, MaskType Msk>
    forceinline I8x32 VECCALL Compare(const I8x32 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return other.Compare<CompareType::GreaterThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return other.Compare<CompareType::GreaterEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else if constexpr (Cmp == CompareType::GreaterEqual) return Compare<CompareType::GreaterThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else
        {
                 if constexpr (Cmp == CompareType::Equal)        return _mm256_cmpeq_epi8(Data, other.Data);
            else if constexpr (Cmp == CompareType::GreaterThan)  return _mm256_cmpgt_epi8(Data, other.Data);
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
    }

    // arithmetic operations
    forceinline I8x32 VECCALL Neg() const noexcept { return _mm256_sub_epi8(_mm256_setzero_si256(), Data); }
    forceinline I8x32 VECCALL Abs() const noexcept { return _mm256_abs_epi8(Data); }
    forceinline I8x32 VECCALL SatAdd(const I8x32& other) const noexcept { return _mm256_adds_epi8(Data, other.Data); }
    forceinline I8x32 VECCALL SatSub(const I8x32& other) const noexcept { return _mm256_subs_epi8(Data, other.Data); }
    forceinline I8x32 VECCALL Max(const I8x32& other) const noexcept { return _mm256_max_epi8(Data, other.Data); }
    forceinline I8x32 VECCALL Min(const I8x32& other) const noexcept { return _mm256_min_epi8(Data, other.Data); }
    forceinline Pack<I16x16, 2> VECCALL MulX(const I8x32& other) const noexcept;
    forceinline I8x32 VECCALL MulHi(const I8x32& other) const noexcept
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightLogic<8>(), hi = full[1].ShiftRightLogic<8>();
        return _mm256_permute4x64_epi64(_mm256_packus_epi16(lo, hi), 0b11011000);
    }
    forceinline I8x32 VECCALL MulLo(const I8x32& other) const noexcept
    { 
        const auto full = MulX(other);
        const I16x16 mask(0x00ff);
        const auto lo = full[0].And(mask), hi = full[1].And(mask);
        return _mm256_permute4x64_epi64(_mm256_packus_epi16(lo, hi), 0b11011000);
    }
    forceinline I8x32 VECCALL operator*(const I8x32& other) const noexcept { return MulLo(other); }
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I8x32, T>(), typename... Args>
    forceinline typename CastTyper<I8x32, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<I16x16, 2> VECCALL I8x32::Cast<I16x16, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtepi8_epi16(_mm256_castsi256_si128(Data)), _mm256_cvtepi8_epi16(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U16x16, 2> VECCALL I8x32::Cast<U16x16, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x16>().As<U16x16>();
}
template<> forceinline Pack<I32x8, 4> VECCALL I8x32::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    const auto val0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128  (                  Data       ));
    const auto val1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 8)   ));
    const auto val2 = _mm256_cvtepi8_epi32(_mm256_extracti128_si256(                  Data, 1    ));
    const auto val3 = _mm256_cvtepi8_epi32(_mm256_extracti128_si256(_mm256_srli_si256(Data, 8), 1));
    return { val0, val1, val2, val3 };
}
template<> forceinline Pack<U32x8, 4> VECCALL I8x32::Cast<U32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().As<U32x8>();
}
template<> forceinline Pack<I64x4, 8> VECCALL I8x32::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    const auto val0 = _mm256_cvtepi8_epi64(_mm256_castsi256_si128  (                  Data     ));
    const auto val1 = _mm256_cvtepi8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data,  4)));
    const auto val2 = _mm256_cvtepi8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data,  8)));
    const auto val3 = _mm256_cvtepi8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 12)));
    const auto val4 = _mm256_cvtepi8_epi64(_mm256_extracti128_si256(                  Data     , 1));
    const auto val5 = _mm256_cvtepi8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data,  4), 1));
    const auto val6 = _mm256_cvtepi8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data,  8), 1));
    const auto val7 = _mm256_cvtepi8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data, 12), 1));
    return { val0, val1, val2, val3, val4, val5, val6, val7 };
}
template<> forceinline Pack<U64x4, 8> VECCALL I8x32::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
template<> forceinline Pack<F32x8, 4> VECCALL I8x32::Cast<F32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F32x8>();
}
template<> forceinline Pack<F64x4, 8> VECCALL I8x32::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F64x4>();
}
forceinline Pack<I16x16, 2> VECCALL I8x32::MulX(const I8x32& other) const noexcept
{
    const auto self16 = Cast<I16x16>(), other16 = other.Cast<I16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif


struct alignas(32) U8x32 : public detail::Common8x32<U8x32, U8x16, uint8_t>
{
    using Common8x32<U8x32, U8x16, uint8_t>::Common8x32;

#if COMMON_SIMD_LV >= 200
    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U8x32 VECCALL Compare(const U8x32 other) const noexcept
    {
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I8x32>().Compare<Cmp, Msk>(other.As<I8x32>()).template As<U8x32>();
        }
        else
        {
            const U8x32 sigMask(static_cast<uint8_t>(0x80));
            return Xor(sigMask).As<I8x32>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I8x32>()).template As<U8x32>();
        }
    }

    // arithmetic operations
    forceinline U8x32 VECCALL SatAdd(const U8x32& other) const noexcept { return _mm256_adds_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL SatSub(const U8x32& other) const noexcept { return _mm256_subs_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL Max(const U8x32& other) const noexcept { return _mm256_max_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL Min(const U8x32& other) const noexcept { return _mm256_min_epu8(Data, other.Data); }
    forceinline U8x32 VECCALL Abs() const noexcept { return Data; }
    forceinline U8x32 VECCALL operator*(const U8x32& other) const noexcept { return MulLo(other); }
    forceinline U8x32 VECCALL MulHi(const U8x32& other) const noexcept
    {
        const auto full = MulX(other);
        const auto lo = full[0].ShiftRightLogic<8>(), hi = full[1].ShiftRightLogic<8>();
        return _mm256_permute4x64_epi64(_mm256_packus_epi16(lo, hi), 0b11011000);
    }
    forceinline U8x32 VECCALL MulLo(const U8x32& other) const noexcept
    {
        const U16x16 u16self = Data, u16other = other.Data;
        const auto even = u16self * u16other;
        const auto odd = u16self.ShiftRightLogic<8>() * u16other.ShiftRightLogic<8>();
        const U16x16 mask((uint16_t)0x00ff);
        return U8x32(odd.ShiftLeftLogic<8>() | (even & mask));
    }
    forceinline Pack<U16x16, 2> VECCALL MulX(const U8x32& other) const noexcept;
#endif
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U8x32, T>(), typename... Args>
    forceinline typename CastTyper<U8x32, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<I16x16, 2> VECCALL U8x32::Cast<I16x16, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtepu8_epi16(_mm256_castsi256_si128(Data)), _mm256_cvtepu8_epi16(_mm256_extracti128_si256(Data, 1)) };
}
template<> forceinline Pack<U16x16, 2> VECCALL U8x32::Cast<U16x16, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x16>().As<U16x16>();
}
template<> forceinline Pack<I32x8, 4> VECCALL U8x32::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    /*const auto zero = _mm256_setzero_si256();
    const auto dat0145 = _mm256_unpacklo_epi8(Data, zero), dat2367 = _mm256_unpackhi_epi8(Data, zero);
    const auto dat04 = _mm256_unpacklo_epi16(dat0145, zero), dat15 = _mm256_unpackhi_epi16(dat0145, zero);
    const auto dat26 = _mm256_unpacklo_epi16(dat2367, zero), dat37 = _mm256_unpackhi_epi16(dat2367, zero);
    return
    {
        _mm256_permute2x128_si256(dat04, dat15, 0x20), _mm256_permute2x128_si256(dat26, dat37, 0x20),
        _mm256_permute2x128_si256(dat04, dat15, 0x31), _mm256_permute2x128_si256(dat26, dat37, 0x31)
    };*/
    const auto val0 = _mm256_cvtepu8_epi32(_mm256_castsi256_si128  (                  Data       ));
    const auto val1 = _mm256_cvtepu8_epi32(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 8)   ));
    const auto val2 = _mm256_cvtepu8_epi32(_mm256_extracti128_si256(                  Data, 1    ));
    const auto val3 = _mm256_cvtepu8_epi32(_mm256_extracti128_si256(_mm256_srli_si256(Data, 8), 1));
    return { val0, val1, val2, val3 };
}
template<> forceinline Pack<U32x8, 4> VECCALL U8x32::Cast<U32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().As<U32x8>();
}
template<> forceinline Pack<I64x4, 8> VECCALL U8x32::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    const auto val0 = _mm256_cvtepu8_epi64(_mm256_castsi256_si128  (                  Data     ));
    const auto val1 = _mm256_cvtepu8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data,  4)));
    const auto val2 = _mm256_cvtepu8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data,  8)));
    const auto val3 = _mm256_cvtepu8_epi64(_mm256_castsi256_si128  (_mm256_srli_si256(Data, 12)));
    const auto val4 = _mm256_cvtepu8_epi64(_mm256_extracti128_si256(                  Data     , 1));
    const auto val5 = _mm256_cvtepu8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data,  4), 1));
    const auto val6 = _mm256_cvtepu8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data,  8), 1));
    const auto val7 = _mm256_cvtepu8_epi64(_mm256_extracti128_si256(_mm256_srli_si256(Data, 12), 1));
    return { val0, val1, val2, val3, val4, val5, val6, val7 };
}
template<> forceinline Pack<U64x4, 8> VECCALL U8x32::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x4>().As<U64x4>();
}
template<> forceinline Pack<F32x8, 4> VECCALL U8x32::Cast<F32x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F32x8>();
}
template<> forceinline Pack<F64x4, 8> VECCALL U8x32::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x8>().Cast<F64x4>();
}
forceinline Pack<U16x16, 2> VECCALL U8x32::MulX(const U8x32& other) const noexcept
{
    const auto self16 = Cast<U16x16>(), other16 = other.Cast<U16x16>();
    return { self16[0].MulLo(other16[0]), self16[1].MulLo(other16[1]) };
}
#endif


#if COMMON_SIMD_LV >= 200
template<typename T, typename L, typename E>
forceinline T VECCALL detail::Common8x32<T, L, E>::ShiftRightArith(const uint8_t bits) const noexcept
{
    if constexpr (std::is_unsigned_v<E>)
        return ShiftRightLogic(bits);
    else
    {
        const auto bit16 = static_cast<const T&>(*this).template As<I16x16>();
        const I16x16 keepMask(static_cast<int16_t>(0xff00));
        const auto shiftHi = bit16                             .ShiftRightArith(bits).And(keepMask).template As<T>();
        const auto shiftLo = bit16.template ShiftLeftLogic<8>().ShiftRightArith(bits).And(keepMask);
        return shiftHi.Or(shiftLo.template ShiftRightLogic<8>().template As<T>());
    }
}
template<typename T, typename L, typename E>
template<uint8_t N>
forceinline T VECCALL detail::Common8x32<T, L, E>::ShiftRightArith() const noexcept
{
    if constexpr (std::is_unsigned_v<E>)
        return ShiftRightLogic<N>();
    else
    {
        const auto bit16 = static_cast<const T&>(*this).template As<I16x16>();
        const I16x16 keepMask(static_cast<int16_t>(0xff00));
        const auto shiftHi = bit16                             .template ShiftRightArith<N>().And(keepMask).template As<T>();
        const auto shiftLo = bit16.template ShiftLeftLogic<8>().template ShiftRightArith<N>().And(keepMask);
        return shiftHi.Or(shiftLo.template ShiftRightLogic<8>().template As<T>());
    }
}
#endif


template<> forceinline U64x4 VECCALL I64x4::Cast<U64x4, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I64x4 VECCALL U64x4::Cast<I64x4, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U32x8 VECCALL I32x8::Cast<U32x8, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I32x8 VECCALL U32x8::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U16x16 VECCALL I16x16::Cast<U16x16, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I16x16 VECCALL U16x16::Cast<I16x16, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline U8x32 VECCALL I8x32::Cast<U8x32, CastMode::RangeUndef>() const noexcept
{
    return Data;
}
template<> forceinline I8x32 VECCALL U8x32::Cast<I8x32, CastMode::RangeUndef>() const noexcept
{
    return Data;
}


#if COMMON_SIMD_LV >= 200
template<> forceinline U32x4 VECCALL U64x4::Cast<U32x4, CastMode::RangeTrunc>() const noexcept
{
    const auto mask = _mm256_setr_epi32(0, 2, 4, 6, 0, 2, 4, 6);
    const auto dat0101 = _mm256_permutevar8x32_epi32(Data, mask);//ab,ab
    return _mm256_castsi256_si128(dat0101);
}
template<> forceinline U32x8 VECCALL U64x4::Cast<U32x8, CastMode::RangeTrunc>(const U64x4& arg1) const noexcept
{
    const auto mask = _mm256_setr_epi32(0, 2, 4, 6, 0, 2, 4, 6);
    const auto dat0101 = _mm256_permutevar8x32_epi32(Data, mask);//ab,ab
    const auto dat2323 = _mm256_permutevar8x32_epi32(arg1, mask);//cd,cd
    return _mm256_blend_epi32(dat0101, dat2323, 0b11110000);//ab,cd
}
template<> forceinline U16x8 VECCALL U32x8::Cast<U16x8, CastMode::RangeTrunc>() const noexcept
{
    const auto mask = _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 4, 5, 8, 9, 12, 13);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a0,0b
    const auto dat0101 = _mm256_permute4x64_epi64(dat01, 0b11001100);//ab,ab
    return _mm256_castsi256_si128(dat0101);
}
template<> forceinline U16x16 VECCALL U32x8::Cast<U16x16, CastMode::RangeTrunc>(const U32x8& arg1) const noexcept
{
    const auto mask = _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 4, 5, 8, 9, 12, 13);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a0,0b
    const auto dat23 = _mm256_shuffle_epi8(arg1, mask);//c0,0d
    const auto dat0101 = _mm256_permute4x64_epi64(dat01, 0b11001100);//ab,ab
    const auto dat2323 = _mm256_permute4x64_epi64(dat23, 0b11001100);//cd,cd
    return _mm256_blend_epi32(dat0101, dat2323, 0b11110000);//ab,cd
}
template<> forceinline U8x16 VECCALL U16x16::Cast<U8x16, CastMode::RangeTrunc>() const noexcept
{
    const auto mask = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a0,0b
    const auto dat0101 = _mm256_permute4x64_epi64(dat01, 0b11001100);//ab,ab
    return _mm256_castsi256_si128(dat0101);
}
template<> forceinline U8x32 VECCALL U16x16::Cast<U8x32, CastMode::RangeTrunc>(const U16x16& arg1) const noexcept
{
    const auto mask = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a0,0b
    const auto dat23 = _mm256_shuffle_epi8(arg1, mask);//c0,0d
    const auto dat0101 = _mm256_permute4x64_epi64(dat01, 0b11001100);//ab,ab
    const auto dat2323 = _mm256_permute4x64_epi64(dat23, 0b11001100);//cd,cd
    return _mm256_blend_epi32(dat0101, dat2323, 0b11110000);//ab,cd
}
template<> forceinline U16x16 VECCALL U64x4::Cast<U16x16, CastMode::RangeTrunc>(const U64x4& arg1, const U64x4& arg2, const U64x4& arg3) const noexcept
{
    const auto mask  = _mm256_setr_epi8(0, 1, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a000,0b00
    const auto dat23 = _mm256_shuffle_epi8(arg1, mask);//c000,0d00
    const auto dat45 = _mm256_shuffle_epi8(arg2, mask);//e000,0f00
    const auto dat67 = _mm256_shuffle_epi8(arg3, mask);//g000,0h00
    const auto dat0213 = _mm256_unpacklo_epi64(dat01, dat23);//a0c0,0b0d
    const auto dat4657 = _mm256_unpacklo_epi64(dat45, dat67);//e0g0,0f0h
    const auto dat0246 = _mm256_permute2x128_si256(dat0213, dat4657, 0x20);//a0c0,e0g0
    const auto dat1357 = _mm256_permute2x128_si256(dat0213, dat4657, 0x31);//0b0d,0f0h
    return _mm256_or_si256(dat0246, dat1357);//abcd,efgh
}
template<> forceinline U8x32 VECCALL U32x8::Cast<U8x32, CastMode::RangeTrunc>(const U32x8& arg1, const U32x8& arg2, const U32x8& arg3) const noexcept
{
    // cvt is even slower due to 2cycle per inst
    //const auto dat0 = _mm256_cvtepi32_epi8(Data);
    //const auto dat1 = _mm256_cvtepi32_epi8(arg1);
    //const auto dat2 = _mm256_cvtepi32_epi8(arg2);
    //const auto dat3 = _mm256_cvtepi32_epi8(arg3);
    //return _mm256_setr_m128i(_mm_unpacklo_epi64(dat0, dat1), _mm_unpacklo_epi64(dat2, dat3));
    
    // need vbmi and not faster due to 10cycle per permute
    //const auto mask = _mm256_setr_epi8(0, 4, 8, 12, 16, 20, 24, 28, -1, -1, -1, -1, -1, -1, -1, -1, 32, 36, 40, 44, 48, 52, 56, 60, -1, -1, -1, -1, -1, -1, -1, -1);
    //const auto lo = _mm256_permutex2var_epi8(Data, mask, arg2);//ab00,ef00
    //const auto hi = _mm256_permutex2var_epi8(arg1, mask, arg3);//cd00,gh00
    //return _mm256_unpacklo_epi64(lo, hi);
    
    //unpack seems to be slighly faster on tgl&hsw, same on zen1&tgl
    //const auto mask  = _mm256_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1);
    //const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a000,0b00
    //const auto dat23 = _mm256_shuffle_epi8(arg1, mask);//c000,0d00
    //const auto dat45 = _mm256_shuffle_epi8(arg2, mask);//e000,0f00
    //const auto dat67 = _mm256_shuffle_epi8(arg3, mask);//g000,0h00
    //const auto dat0213 = _mm256_unpacklo_epi64(dat01, dat23);//a0c0,0b0d
    //const auto dat4657 = _mm256_unpacklo_epi64(dat45, dat67);//e0g0,0f0h

    const auto mask1 = _mm256_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto mask2 = _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask1);//a000,0b00
    const auto dat23 = _mm256_shuffle_epi8(arg1, mask2);//00c0,000d
    const auto dat45 = _mm256_shuffle_epi8(arg2, mask1);//e000,0f00
    const auto dat67 = _mm256_shuffle_epi8(arg3, mask2);//00g0,000h
    const auto dat0213 = _mm256_or_si256(dat01, dat23);//a0c0,0b0d
    const auto dat4657 = _mm256_or_si256(dat45, dat67);//e0g0,0f0h

    const auto dat0246 = _mm256_permute2x128_si256(dat0213, dat4657, 0x20);//a0c0,e0g0
    const auto dat1357 = _mm256_permute2x128_si256(dat0213, dat4657, 0x31);//0b0d,0f0h
    return _mm256_or_si256(dat0246, dat1357);//abcd,efgh
}
template<> forceinline U8x32 VECCALL U64x4::Cast<U8x32, CastMode::RangeTrunc>(const U64x4& arg1, const U64x4& arg2, const U64x4& arg3,
    const U64x4& arg4, const U64x4& arg5, const U64x4& arg6, const U64x4& arg7) const noexcept
{
    const auto mask = _mm256_setr_epi64x(0xffffffffffff0800, 0xffffffffffffffff, 0xffffffff0800ffff, 0xffffffffffffffff);
    //const auto mask = _mm256_setr_epi8(0, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const auto dat01 = _mm256_shuffle_epi8(Data, mask);//a0000000,0b000000
    const auto dat23 = _mm256_shuffle_epi8(arg1, mask);//c0000000,0d000000
    const auto dat45 = _mm256_shuffle_epi8(arg2, mask);//e0000000,0f000000
    const auto dat67 = _mm256_shuffle_epi8(arg3, mask);//g0000000,0h000000
    const auto dat89 = _mm256_shuffle_epi8(arg4, mask);//i0000000,0j000000
    const auto datab = _mm256_shuffle_epi8(arg5, mask);//k0000000,0l000000
    const auto datcd = _mm256_shuffle_epi8(arg6, mask);//m0000000,0n000000
    const auto datef = _mm256_shuffle_epi8(arg7, mask);//o0000000,0p000000
    //const auto dat0213 = _mm256_unpacklo_epi32(dat01, dat23);//a0c00000,0b0d0000
    //const auto dat4657 = _mm256_unpacklo_epi32(dat45, dat67);//e0g00000,0f0h0000
    //const auto dat8a9b = _mm256_unpacklo_epi32(dat89, datab);//i0k00000,0j0l0000
    //const auto datcedf = _mm256_unpacklo_epi32(datcd, datef);//m0o00000,0n0p0000
    
    // shift&or can run on different port than unpack (on intel), leverage port loads
    const auto dat0213 = _mm256_or_si256(dat01, _mm256_slli_epi64(dat23, 32));//a0c00000,0b0d0000
    const auto dat4657 = _mm256_or_si256(dat45, _mm256_slli_epi64(dat67, 32));//e0g00000,0f0h0000
    const auto dat8a9b = _mm256_or_si256(dat89, _mm256_slli_epi64(datab, 32));//i0k00000,0j0l0000
    const auto datcedf = _mm256_or_si256(datcd, _mm256_slli_epi64(datef, 32));//m0o00000,0n0p0000
    
    const auto dat02461357 = _mm256_unpacklo_epi64(dat0213, dat4657);//a0c0e0g0,0b0d0f0h
    const auto dat8ace9bdf = _mm256_unpacklo_epi64(dat8a9b, datcedf);//i0k0m0o0,0j0l0n0p
    const auto dat02468ace = _mm256_permute2x128_si256(dat02461357, dat8ace9bdf, 0x20);//a0c0e0g0,i0k0m0o0
    const auto dat13579bdf = _mm256_permute2x128_si256(dat02461357, dat8ace9bdf, 0x31);//0b0d0f0h,0j0l0n0p
    return _mm256_or_si256(dat02468ace, dat13579bdf);//abcdefgh,ijklmnop
}
template<> forceinline I32x4 VECCALL I64x4::Cast<I32x4, CastMode::RangeTrunc>() const noexcept
{
    return As<U64x4>().Cast<U32x4>().As<I32x4>();
}
template<> forceinline I32x8 VECCALL I64x4::Cast<I32x8, CastMode::RangeTrunc>(const I64x4& arg1) const noexcept
{
    return As<U64x4>().Cast<U32x8>(arg1.As<U64x4>()).As<I32x8>();
}
template<> forceinline I16x8 VECCALL I32x8::Cast<I16x8, CastMode::RangeTrunc>() const noexcept
{
    return As<U32x8>().Cast<U16x8>().As<I16x8>();
}
template<> forceinline I16x16 VECCALL I32x8::Cast<I16x16, CastMode::RangeTrunc>(const I32x8& arg1) const noexcept
{
    return As<U32x8>().Cast<U16x16>(arg1.As<U32x8>()).As<I16x16>();
}
template<> forceinline I8x16 VECCALL I16x16::Cast<I8x16, CastMode::RangeTrunc>() const noexcept
{
    return As<U16x16>().Cast<U8x16>().As<I8x16>();
}
template<> forceinline I8x32 VECCALL I16x16::Cast<I8x32, CastMode::RangeTrunc>(const I16x16& arg1) const noexcept
{
    return As<U16x16>().Cast<U8x32>(arg1.As<U16x16>()).As<I8x32>();
}
template<> forceinline I16x16 VECCALL I64x4::Cast<I16x16, CastMode::RangeTrunc>(const I64x4& arg1, const I64x4& arg2, const I64x4& arg3) const noexcept
{
    return As<U64x4>().Cast<U16x16>(arg1.As<U64x4>(), arg2.As<U64x4>(), arg3.As<U64x4>()).As<I16x16>();
}
template<> forceinline I8x32 VECCALL I32x8::Cast<I8x32, CastMode::RangeTrunc>(const I32x8& arg1, const I32x8& arg2, const I32x8& arg3) const noexcept
{
    return As<U32x8>().Cast<U8x32>(arg1.As<U32x8>(), arg2.As<U32x8>(), arg3.As<U32x8>()).As<I8x32>();
}
template<> forceinline I8x32 VECCALL I64x4::Cast<I8x32, CastMode::RangeTrunc>(const I64x4& arg1, const I64x4& arg2, const I64x4& arg3,
    const I64x4& arg4, const I64x4& arg5, const I64x4& arg6, const I64x4& arg7) const noexcept
{
    return As<U64x4>().Cast<U8x32>(arg1.As<U64x4>(), arg2.As<U64x4>(), arg3.As<U64x4>(),
        arg4.As<U64x4>(), arg5.As<U64x4>(), arg6.As<U64x4>(), arg7.As<U64x4>()).As<I8x32>();
}
#endif


template<> forceinline I32x8 VECCALL F32x8::Cast<I32x8, CastMode::RangeUndef>() const noexcept
{
    return _mm256_cvttps_epi32(Data);
}
#if COMMON_SIMD_LV >= 200
template<> forceinline I16x16 VECCALL F32x8::Cast<I16x16, CastMode::RangeUndef>(const F32x8& arg1) const noexcept
{
    return Cast<I32x8>().Cast<I16x16>(arg1.Cast<I32x8>());
}
template<> forceinline U16x16 VECCALL F32x8::Cast<U16x16, CastMode::RangeUndef>(const F32x8& arg1) const noexcept
{
    return Cast<I16x16>(arg1).As<U16x16>();
}
template<> forceinline I8x32 VECCALL F32x8::Cast<I8x32, CastMode::RangeUndef>(const F32x8& arg1, const F32x8& arg2, const F32x8& arg3) const noexcept
{
    return Cast<I32x8>().Cast<I8x32>(arg1.Cast<I32x8>(), arg2.Cast<I32x8>(), arg3.Cast<I32x8>());
}
template<> forceinline U8x32 VECCALL F32x8::Cast<U8x32, CastMode::RangeUndef>(const F32x8& arg1, const F32x8& arg2, const F32x8& arg3) const noexcept
{
    return Cast<I8x32>(arg1, arg2, arg3).As<U8x32>();
}
#endif
template<> forceinline Pack<F64x4, 2> VECCALL F32x8::Cast<F64x4, CastMode::RangeUndef>() const noexcept
{
    return { _mm256_cvtps_pd(_mm256_extractf128_ps(Data, 0)), _mm256_cvtps_pd(_mm256_extractf128_ps(Data, 1)) };
}
template<> forceinline F32x8 VECCALL F64x4::Cast<F32x8, CastMode::RangeUndef>(const F64x4& arg1) const noexcept
{
    return _mm256_set_m128(_mm256_cvtpd_ps(arg1.Data), _mm256_cvtpd_ps(Data));
}


template<> forceinline I32x8 VECCALL F32x8::Cast<I32x8, CastMode::RangeSaturate>() const noexcept
{
    const F32x8 minVal = static_cast<float>(INT32_MIN), maxVal = static_cast<float>(INT32_MAX);
    const auto val = Cast<I32x8, CastMode::RangeUndef>();
    // INT32 loses precision, need maunally bit-select
    const auto isLe = Compare<CompareType::LessEqual,    MaskType::FullEle>(minVal);
    const auto isGe = Compare<CompareType::GreaterEqual, MaskType::FullEle>(maxVal);
    /*const auto isLe = _mm256_cmp_ps(Data, minVal, _CMP_LE_OQ);
    const auto isGe = _mm256_cmp_ps(Data, maxVal, _CMP_GE_OQ);*/
    const auto satMin = _mm256_blendv_ps(val.As<F32x8>(), I32x8(INT32_MIN).As<F32x8>(), isLe);
    return _mm256_castps_si256(_mm256_blendv_ps(satMin, I32x8(INT32_MAX).As<F32x8>(), isGe));
}
#if COMMON_SIMD_LV >= 200
template<> forceinline I16x16 VECCALL F32x8::Cast<I16x16, CastMode::RangeSaturate>(const F32x8& arg1) const noexcept
{
    const F32x8 minVal = static_cast<float>(INT16_MIN), maxVal = static_cast<float>(INT16_MAX);
    return Min(maxVal).Max(minVal).Cast<I16x16, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline U16x16 VECCALL F32x8::Cast<U16x16, CastMode::RangeSaturate>(const F32x8& arg1) const noexcept
{
    const F32x8 minVal = 0, maxVal = static_cast<float>(UINT16_MAX);
    return Min(maxVal).Max(minVal).Cast<U16x16, CastMode::RangeUndef>(arg1.Min(maxVal).Max(minVal));
}
template<> forceinline I8x32 VECCALL F32x8::Cast<I8x32, CastMode::RangeSaturate>(const F32x8& arg1, const F32x8& arg2, const F32x8& arg3) const noexcept
{
    const F32x8 minVal = static_cast<float>(INT8_MIN), maxVal = static_cast<float>(INT8_MAX);
    return Min(maxVal).Max(minVal).Cast<I8x32, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline U8x32 VECCALL F32x8::Cast<U8x32, CastMode::RangeSaturate>(const F32x8& arg1, const F32x8& arg2, const F32x8& arg3) const noexcept
{
    const F32x8 minVal = 0, maxVal = static_cast<float>(UINT8_MAX);
    return Min(maxVal).Max(minVal).Cast<U8x32, CastMode::RangeUndef>(
        arg1.Min(maxVal).Max(minVal), arg2.Min(maxVal).Max(minVal), arg3.Min(maxVal).Max(minVal));
}
template<> forceinline I16x16 VECCALL I32x8::Cast<I16x16, CastMode::RangeSaturate>(const I32x8& arg1) const noexcept
{
    return _mm256_permute4x64_epi64(_mm256_packs_epi32(Data, arg1), 0b11011000);
}
template<> forceinline U16x16 VECCALL I32x8::Cast<U16x16, CastMode::RangeSaturate>(const I32x8& arg1) const noexcept
{
    return _mm256_permute4x64_epi64(_mm256_packus_epi32(Data, arg1), 0b11011000);
}
template<> forceinline U16x16 VECCALL U32x8::Cast<U16x16, CastMode::RangeSaturate>(const U32x8& arg1) const noexcept
{
    const auto data_ = Min(UINT16_MAX).As<I32x8>(), arg1_ = arg1.Min(UINT16_MAX).As<I32x8>();
    return data_.Cast<U16x16, CastMode::RangeSaturate>(arg1_);
}
template<> forceinline I8x32 VECCALL I16x16::Cast<I8x32, CastMode::RangeSaturate>(const I16x16& arg1) const noexcept
{
    return _mm256_permute4x64_epi64(_mm256_packs_epi16(Data, arg1), 0b11011000);
}
template<> forceinline U8x32 VECCALL I16x16::Cast<U8x32, CastMode::RangeSaturate>(const I16x16& arg1) const noexcept
{
    return _mm256_permute4x64_epi64(_mm256_packus_epi16(Data, arg1), 0b11011000);
}
template<> forceinline U8x32 VECCALL U16x16::Cast<U8x32, CastMode::RangeSaturate>(const U16x16& arg1) const noexcept
{
    const auto data_ = Min(UINT8_MAX).As<I16x16>(), arg1_ = arg1.Min(UINT8_MAX).As<I16x16>();
    return data_.Cast<U8x32, CastMode::RangeSaturate>(arg1_);
}
#endif


}
