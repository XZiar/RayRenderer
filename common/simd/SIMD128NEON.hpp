#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"
#include <cfenv>

#if COMMON_SIMD_LV < 10
#   error require at least NEON
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
struct I32x4;
struct U32x4;
struct F32x4;
struct I64x2;
struct F64x2;


namespace detail
{
#if COMMON_COMPILER_MSVC // msvc only typedef __n128
template<> forceinline __n128 AsType(__n128 from) noexcept { return from; }
#else
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType(float64x2_t from) noexcept { return from; }
template<> forceinline float32x4_t AsType(float64x2_t from) noexcept { return vreinterpretq_f32_f64(from); }
template<> forceinline   int64x2_t AsType(float64x2_t from) noexcept { return vreinterpretq_s64_f64(from); }
template<> forceinline  uint64x2_t AsType(float64x2_t from) noexcept { return vreinterpretq_u64_f64(from); }
template<> forceinline   int32x4_t AsType(float64x2_t from) noexcept { return vreinterpretq_s32_f64(from); }
template<> forceinline  uint32x4_t AsType(float64x2_t from) noexcept { return vreinterpretq_u32_f64(from); }
template<> forceinline   int16x8_t AsType(float64x2_t from) noexcept { return vreinterpretq_s16_f64(from); }
template<> forceinline  uint16x8_t AsType(float64x2_t from) noexcept { return vreinterpretq_u16_f64(from); }
template<> forceinline   int8x16_t AsType(float64x2_t from) noexcept { return vreinterpretq_s8_f64 (from); }
template<> forceinline  uint8x16_t AsType(float64x2_t from) noexcept { return vreinterpretq_u8_f64 (from); }
template<> forceinline float64x2_t AsType(float32x4_t from) noexcept { return vreinterpretq_f64_f32(from); }
# endif
template<> forceinline float32x4_t AsType(float32x4_t from) noexcept { return from; } 
template<> forceinline   int64x2_t AsType(float32x4_t from) noexcept { return vreinterpretq_s64_f32(from); }
template<> forceinline  uint64x2_t AsType(float32x4_t from) noexcept { return vreinterpretq_u64_f32(from); }
template<> forceinline   int32x4_t AsType(float32x4_t from) noexcept { return vreinterpretq_s32_f32(from); }
template<> forceinline  uint32x4_t AsType(float32x4_t from) noexcept { return vreinterpretq_u32_f32(from); }
template<> forceinline   int16x8_t AsType(float32x4_t from) noexcept { return vreinterpretq_s16_f32(from); }
template<> forceinline  uint16x8_t AsType(float32x4_t from) noexcept { return vreinterpretq_u16_f32(from); }
template<> forceinline   int8x16_t AsType(float32x4_t from) noexcept { return vreinterpretq_s8_f32 (from); }
template<> forceinline  uint8x16_t AsType(float32x4_t from) noexcept { return vreinterpretq_u8_f32 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType(  int64x2_t from) noexcept { return vreinterpretq_f64_s64(from); }
# endif
template<> forceinline float32x4_t AsType(  int64x2_t from) noexcept { return vreinterpretq_f32_s64(from); }
template<> forceinline   int64x2_t AsType(  int64x2_t from) noexcept { return from; } 
template<> forceinline  uint64x2_t AsType(  int64x2_t from) noexcept { return vreinterpretq_u64_s64(from); }
template<> forceinline   int32x4_t AsType(  int64x2_t from) noexcept { return vreinterpretq_s32_s64(from); }
template<> forceinline  uint32x4_t AsType(  int64x2_t from) noexcept { return vreinterpretq_u32_s64(from); }
template<> forceinline   int16x8_t AsType(  int64x2_t from) noexcept { return vreinterpretq_s16_s64(from); }
template<> forceinline  uint16x8_t AsType(  int64x2_t from) noexcept { return vreinterpretq_u16_s64(from); }
template<> forceinline   int8x16_t AsType(  int64x2_t from) noexcept { return vreinterpretq_s8_s64 (from); }
template<> forceinline  uint8x16_t AsType(  int64x2_t from) noexcept { return vreinterpretq_u8_s64 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType( uint64x2_t from) noexcept { return vreinterpretq_f64_u64(from); }
# endif
template<> forceinline float32x4_t AsType( uint64x2_t from) noexcept { return vreinterpretq_f32_u64(from); }
template<> forceinline   int64x2_t AsType( uint64x2_t from) noexcept { return vreinterpretq_s64_u64(from); }
template<> forceinline  uint64x2_t AsType( uint64x2_t from) noexcept { return from; } 
template<> forceinline   int32x4_t AsType( uint64x2_t from) noexcept { return vreinterpretq_s32_u64(from); }
template<> forceinline  uint32x4_t AsType( uint64x2_t from) noexcept { return vreinterpretq_u32_u64(from); }
template<> forceinline   int16x8_t AsType( uint64x2_t from) noexcept { return vreinterpretq_s16_u64(from); }
template<> forceinline  uint16x8_t AsType( uint64x2_t from) noexcept { return vreinterpretq_u16_u64(from); }
template<> forceinline   int8x16_t AsType( uint64x2_t from) noexcept { return vreinterpretq_s8_u64 (from); }
template<> forceinline  uint8x16_t AsType( uint64x2_t from) noexcept { return vreinterpretq_u8_u64 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType(  int32x4_t from) noexcept { return vreinterpretq_f64_s32(from); }
# endif
template<> forceinline float32x4_t AsType(  int32x4_t from) noexcept { return vreinterpretq_f32_s32(from); }
template<> forceinline   int64x2_t AsType(  int32x4_t from) noexcept { return vreinterpretq_s64_s32(from); }
template<> forceinline  uint64x2_t AsType(  int32x4_t from) noexcept { return vreinterpretq_u64_s32(from); }
template<> forceinline   int32x4_t AsType(  int32x4_t from) noexcept { return from; } 
template<> forceinline  uint32x4_t AsType(  int32x4_t from) noexcept { return vreinterpretq_u32_s32(from); }
template<> forceinline   int16x8_t AsType(  int32x4_t from) noexcept { return vreinterpretq_s16_s32(from); }
template<> forceinline  uint16x8_t AsType(  int32x4_t from) noexcept { return vreinterpretq_u16_s32(from); }
template<> forceinline   int8x16_t AsType(  int32x4_t from) noexcept { return vreinterpretq_s8_s32 (from); }
template<> forceinline  uint8x16_t AsType(  int32x4_t from) noexcept { return vreinterpretq_u8_s32 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType( uint32x4_t from) noexcept { return vreinterpretq_f64_u32(from); }
# endif
template<> forceinline float32x4_t AsType( uint32x4_t from) noexcept { return vreinterpretq_f32_u32(from); }
template<> forceinline   int64x2_t AsType( uint32x4_t from) noexcept { return vreinterpretq_s64_u32(from); }
template<> forceinline  uint64x2_t AsType( uint32x4_t from) noexcept { return vreinterpretq_u64_u32(from); }
template<> forceinline   int32x4_t AsType( uint32x4_t from) noexcept { return vreinterpretq_s32_u32(from); }
template<> forceinline  uint32x4_t AsType( uint32x4_t from) noexcept { return from; } 
template<> forceinline   int16x8_t AsType( uint32x4_t from) noexcept { return vreinterpretq_s16_u32(from); }
template<> forceinline  uint16x8_t AsType( uint32x4_t from) noexcept { return vreinterpretq_u16_u32(from); }
template<> forceinline   int8x16_t AsType( uint32x4_t from) noexcept { return vreinterpretq_s8_u32 (from); }
template<> forceinline  uint8x16_t AsType( uint32x4_t from) noexcept { return vreinterpretq_u8_u32 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType(  int16x8_t from) noexcept { return vreinterpretq_f64_s16(from); }
# endif
template<> forceinline float32x4_t AsType(  int16x8_t from) noexcept { return vreinterpretq_f32_s16(from); }
template<> forceinline   int64x2_t AsType(  int16x8_t from) noexcept { return vreinterpretq_s64_s16(from); }
template<> forceinline  uint64x2_t AsType(  int16x8_t from) noexcept { return vreinterpretq_u64_s16(from); }
template<> forceinline   int32x4_t AsType(  int16x8_t from) noexcept { return vreinterpretq_s32_s16(from); }
template<> forceinline  uint32x4_t AsType(  int16x8_t from) noexcept { return vreinterpretq_u32_s16(from); }
template<> forceinline   int16x8_t AsType(  int16x8_t from) noexcept { return from; } 
template<> forceinline  uint16x8_t AsType(  int16x8_t from) noexcept { return vreinterpretq_u16_s16(from); }
template<> forceinline   int8x16_t AsType(  int16x8_t from) noexcept { return vreinterpretq_s8_s16 (from); }
template<> forceinline  uint8x16_t AsType(  int16x8_t from) noexcept { return vreinterpretq_u8_s16 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType( uint16x8_t from) noexcept { return vreinterpretq_f64_u16(from); }
# endif
template<> forceinline float32x4_t AsType( uint16x8_t from) noexcept { return vreinterpretq_f32_u16(from); }
template<> forceinline   int64x2_t AsType( uint16x8_t from) noexcept { return vreinterpretq_s64_u16(from); }
template<> forceinline  uint64x2_t AsType( uint16x8_t from) noexcept { return vreinterpretq_u64_u16(from); }
template<> forceinline   int32x4_t AsType( uint16x8_t from) noexcept { return vreinterpretq_s32_u16(from); }
template<> forceinline  uint32x4_t AsType( uint16x8_t from) noexcept { return vreinterpretq_u32_u16(from); }
template<> forceinline   int16x8_t AsType( uint16x8_t from) noexcept { return vreinterpretq_s16_u16(from); }
template<> forceinline  uint16x8_t AsType( uint16x8_t from) noexcept { return from; } 
template<> forceinline   int8x16_t AsType( uint16x8_t from) noexcept { return vreinterpretq_s8_u16 (from); }
template<> forceinline  uint8x16_t AsType( uint16x8_t from) noexcept { return vreinterpretq_u8_u16 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType(  int8x16_t from) noexcept { return vreinterpretq_f64_s8(from); }
# endif
template<> forceinline float32x4_t AsType(  int8x16_t from) noexcept { return vreinterpretq_f32_s8(from); }
template<> forceinline   int64x2_t AsType(  int8x16_t from) noexcept { return vreinterpretq_s64_s8(from); }
template<> forceinline  uint64x2_t AsType(  int8x16_t from) noexcept { return vreinterpretq_u64_s8(from); }
template<> forceinline   int32x4_t AsType(  int8x16_t from) noexcept { return vreinterpretq_s32_s8(from); }
template<> forceinline  uint32x4_t AsType(  int8x16_t from) noexcept { return vreinterpretq_u32_s8(from); }
template<> forceinline   int16x8_t AsType(  int8x16_t from) noexcept { return vreinterpretq_s16_s8(from); }
template<> forceinline  uint16x8_t AsType(  int8x16_t from) noexcept { return vreinterpretq_u16_s8(from); }
template<> forceinline   int8x16_t AsType(  int8x16_t from) noexcept { return from; } 
template<> forceinline  uint8x16_t AsType(  int8x16_t from) noexcept { return vreinterpretq_u8_s8 (from); }
# if COMMON_SIMD_LV >= 200
template<> forceinline float64x2_t AsType( uint8x16_t from) noexcept { return vreinterpretq_f64_u8(from); }
# endif
template<> forceinline float32x4_t AsType( uint8x16_t from) noexcept { return vreinterpretq_f32_u8(from); }
template<> forceinline   int64x2_t AsType( uint8x16_t from) noexcept { return vreinterpretq_s64_u8(from); }
template<> forceinline  uint64x2_t AsType( uint8x16_t from) noexcept { return vreinterpretq_u64_u8(from); }
template<> forceinline   int32x4_t AsType( uint8x16_t from) noexcept { return vreinterpretq_s32_u8(from); }
template<> forceinline  uint32x4_t AsType( uint8x16_t from) noexcept { return vreinterpretq_u32_u8(from); }
template<> forceinline   int16x8_t AsType( uint8x16_t from) noexcept { return vreinterpretq_s16_u8(from); }
template<> forceinline  uint16x8_t AsType( uint8x16_t from) noexcept { return vreinterpretq_u16_u8(from); }
template<> forceinline   int8x16_t AsType( uint8x16_t from) noexcept { return vreinterpretq_s8_u8 (from); }
template<> forceinline  uint8x16_t AsType( uint8x16_t from) noexcept { return from; } 
#endif

template<typename T, typename SIMDType, typename E, size_t N>
struct Neon128Common : public CommonOperators<T>
{
    using EleType = E;
    using VecType = SIMDType;
    static constexpr size_t Count = N;
    static constexpr VecDataInfo VDInfo =
    {
        std::is_floating_point_v<E> ? VecDataInfo::DataTypes::Float : 
            (std::is_unsigned_v<E> ? VecDataInfo::DataTypes::Unsigned : VecDataInfo::DataTypes::Signed),
        static_cast<uint8_t>(128 / N), N, 0
    };
    union
    {
        SIMDType Data;
        E Val[N];
    };
    forceinline constexpr Neon128Common() noexcept : Data() { }
    forceinline constexpr Neon128Common(const SIMDType val) noexcept : Data(val) { }
    forceinline constexpr operator const SIMDType& () const noexcept { return Data; }

    // logic operations
    forceinline T VECCALL And(const T& other) const noexcept
    {
        return AsType<SIMDType>(vandq_u32(AsType<uint32x4_t>(Data), AsType<uint32x4_t>(other.Data)));
    }
    forceinline T VECCALL Or(const T& other) const noexcept
    {
        return AsType<SIMDType>(vorrq_u32(AsType<uint32x4_t>(Data), AsType<uint32x4_t>(other.Data)));
    }
    forceinline T VECCALL Xor(const T& other) const noexcept
    {
        return AsType<SIMDType>(veorq_u32(AsType<uint32x4_t>(Data), AsType<uint32x4_t>(other.Data)));
    }
    forceinline T VECCALL AndNot(const T& other) const noexcept
    {
        // swap since NOT performed on src.b
        return AsType<SIMDType>(vbicq_u32(AsType<uint32x4_t>(other.Data), AsType<uint32x4_t>(Data)));
    }
    forceinline T VECCALL Not() const noexcept
    {
        return AsType<SIMDType>(vmvnq_u32(AsType<uint32x4_t>(Data)));
    }

    // shuffle operations
    forceinline Pack<T, 2> VECCALL Zip(const T& other) const noexcept
    {
        return { static_cast<const T*>(this)->ZipLo(other), static_cast<const T*>(this)->ZipHi(other) };
    }
    template<size_t Cnt>
    forceinline T VECCALL MoveToHi() const noexcept
    {
        static_assert(Cnt <= N, "shift should be in [0, N]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N) return AllZero();
        else return AsType<SIMDType>(vextq_u8(vdupq_n_u8(0), AsType<uint8x16_t>(Data), 16 - Cnt * sizeof(E)));
    }
    template<size_t Cnt>
    forceinline T VECCALL MoveToLo() const noexcept
    {
        static_assert(Cnt <= N, "shift should be in [0, N]");
        if constexpr (Cnt == 0) return Data;
        else if constexpr (Cnt == N) return AllZero();
        else return AsType<SIMDType>(vextq_u8(AsType<uint8x16_t>(Data), vdupq_n_u8(0), Cnt * sizeof(E)));
    }
    forceinline T VECCALL MoveHiToLo() const noexcept { return MoveToLo<N / 2>(); }
    forceinline bool VECCALL IsAllZero() const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return vmaxvq_u32(AsType<uint32x4_t>(Data)) == 0;
#else
        const auto tmp = vqmovn_u64(AsType<uint64x2_t>(Data));
        return vget_lane_u32(vpmax_u32(tmp, tmp), 0) == 0;
#endif
    }

    forceinline static T AllZero() noexcept
    {
        if constexpr (std::is_same_v<EleType, float>)
            return vdupq_n_f32(0);
        else if constexpr (std::is_same_v<EleType, double>)
            return vdupq_n_f64(0);
        return AsType<SIMDType>(vdupq_n_u8(0));
    }
};


template<typename T, typename SIMDType>
struct Shuffle64Common
{
    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
        const int64_t lo = vgetq_lane_s64(data, 0), hi = vgetq_lane_s64(data, 1);
        const uint32_t idx = lo < 0 ? 0u : (hi < 0 ? 1u : 2u);
        uint32_t ret = lo < 0 ? 1u : 0u;
        if (hi < 0) ret |= 2u;
        return { idx, ret };
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
        const int64_t lo = vgetq_lane_s64(data, 0), hi = vgetq_lane_s64(data, 1);
        uint32_t ret = lo < 0 ? 1u : 0u;
        if (hi < 0) ret |= 2u;
        return ret;
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 2, "shuffle index should be in [0,1]");
        using V = typename T::VecType;
        const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<V>(vdupq_laneq_u64(data, Idx));
#else
        return AsType<V>(vdupq_n_u64(vgetq_lane_u64(data, Idx)));
#endif
    }
    template<uint8_t Lo, uint8_t Hi>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo < 2 && Hi < 2, "shuffle index should be in [0,1]");
        using V = typename T::VecType;
        const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
        if constexpr (Lo == Hi)
            return Broadcast<Lo>();
        else if constexpr (Lo == 0 && Hi == 1)
            return AsType<V>(data);
        else
            return AsType<V>(vextq_u64(data, data, 1));
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
    forceinline T VECCALL ZipLo(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip1q_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(other.Data)));
#else
        const auto a = vget_low_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data)), b = vget_low_u64(AsType<uint64x2_t>(other.Data));
        return AsType<SIMDType>(vcombine_u64(a, b));
#endif
    }
    forceinline T VECCALL ZipHi(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip2q_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(other.Data)));
#else
        const auto a = vget_high_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data)), b = vget_high_u64(AsType<uint64x2_t>(other.Data));
        return AsType<SIMDType>(vcombine_u64(a, b));
#endif
    }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn1q_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(other.Data)));
#else
        return ZipLo(other);
#endif
    }
    forceinline T VECCALL ZipEven(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn2q_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(other.Data)));
#else
        return ZipHi(other);
#endif
    }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, const T mask) const noexcept
    {
        uint64x2_t msk;
        if constexpr (Msk != MaskType::FullEle)
            msk = AsType<uint64x2_t>(vshrq_n_s64(AsType<int64x2_t>(mask.Data), 64)); // make sure all bits are covered
        else
            msk = AsType<uint64x2_t>(mask.Data);
        return AsType<SIMDType>(vbslq_u64(msk, AsType<uint64x2_t>(other.Data), AsType<uint64x2_t>(static_cast<const T*>(this)->Data)));
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        static_assert(Mask <= 0b11, "Only allow 2 bits");
        if constexpr (Mask == 0b00)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0b11)
            return other;
        else
        {
            constexpr auto Mask8 = ::common::simd::detail::ConvertMaskTo8<64, 2>(Mask);
            static constexpr uint64_t mask[2] = { ::common::simd::detail::FullMask64[Mask8 & 0xff], ::common::simd::detail::FullMask64[(Mask8 >> 8) & 0xff] };
            return SelectWith<MaskType::FullEle>(other, AsType<SIMDType>(vld1q_u64(mask)));
        }
    }
    template<uint8_t Count>
    forceinline T VECCALL MoveToLoWith(const T& hi) const noexcept
    {
        static_assert(Count <= 2, "move count should be in [0,2]");
        if constexpr (Count == 0)
            return *static_cast<const T*>(this);
        else if constexpr (Count == 2)
            return hi;
        else
            return AsType<SIMDType>(vextq_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(hi.Data), Count));
    }
};


template<typename T, typename SIMDType, typename E>
struct alignas(16) Common64x2 : public Neon128Common<T, SIMDType, E, 2>, public Shuffle64Common<T, SIMDType>
{
    using Neon128Common<T, SIMDType, E, 2>::Neon128Common;
    forceinline T VECCALL SwapEndian() const noexcept
    {
        return AsType<SIMDType>(vrev64q_u8(AsType<uint8x16_t>(this->Data)));
    }

    // arithmetic operations
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint64x2_t>(this->Data);
        return AsType<SIMDType>(vshlq_u64(data, vdupq_n_s64(bits)));
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint64x2_t>(this->Data);
        return AsType<SIMDType>(vshlq_u64(data, vdupq_n_s64(-bits)));
    }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return vshlq_u64(this->Data, vdupq_n_s64(-bits));
        else
            return vshlq_s64(this->Data, vdupq_n_s64(-bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
        const auto data = AsType<uint64x2_t>(this->Data);
        return AsType<SIMDType>(vshlq_u64(data, AsType<int64x2_t>(bits.Data)));
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
        const auto data = AsType<uint64x2_t>(this->Data);
        const auto bits_ = vnegq_s64(AsType<int64x2_t>(bits.Data));
        return AsType<SIMDType>(vshlq_u64(data, bits_));
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 64)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint64x2_t>(this->Data);
            return AsType<SIMDType>(vshlq_n_u64(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 64)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint64x2_t>(this->Data);
            return AsType<SIMDType>(vshrq_n_u64(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        static_assert(N <= 64, "NEON limits right shift within [0,64]");
        if constexpr (N == 0)
            return this->Data;
        else
        {
            if constexpr (std::is_unsigned_v<E>)
                return vshrq_n_u64(this->Data, N);
            else
                return vshrq_n_s64(this->Data, N);
        }
    }

    forceinline T VECCALL operator*(const T& other) const noexcept { return static_cast<const T*>(this)->MulLo(other); }

    forceinline static T LoadLo(const E val) noexcept 
    { 
        return AsType<SIMDType>(vcombine_u64(vld1_u64(reinterpret_cast<const uint64_t*>(&val)), vdup_n_u64(0)));
    }
    forceinline static T LoadLo(const E* ptr) noexcept
    {
        return AsType<SIMDType>(vcombine_u64(vld1_u64(reinterpret_cast<const uint64_t*>(ptr)), vdup_n_u64(0)));
    }
};


template<typename T, typename SIMDType>
struct Shuffle32Common
{
    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        if constexpr (Msk == MaskType::FullEle)
        {
            const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
            // 32 10
            const auto combined2 = vshrn_n_u64(data, 16);
            const auto sign16bits = vget_lane_u64(vreinterpret_u64_u32(combined2), 0);
            const auto idx = sign16bits ? CountTralingZero(sign16bits) >> 4 : 4u;
            if constexpr (NeedSignBits)
            {
                uint32_t signbits = static_cast<uint32_t>(sign16bits & 0x1u);
                if (sign16bits & 0x10000u) signbits |= 0x2u;
                if (sign16bits & 0x100000000u) signbits |= 0x4u;
                if (sign16bits & 0x1000000000000u) signbits |= 0x8u;
                return { idx, signbits };
            }
            else
                return { idx, sign16bits ? 1u : 0u };
        }
        else
        {
            if constexpr (NeedSignBits)
            {
                const auto signbits = ExtractSignBit();
                const auto ctzbits = signbits | 0x10u;
                return { CountTralingZero(ctzbits), signbits };
            }
            else
            {
                const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
                const uint64_t lo = vgetq_lane_u64(data, 0), hi = vgetq_lane_u64(data, 1);
                const auto bit0 = lo & 0x80000000u, bit1 = lo & 0x8000000000000000u, bit2 = hi & 0x80000000u, bit3 = hi & 0x8000000000000000u;
                uint32_t idx = bit0 ? 0u : (bit1 ? 1u : (bit2 ? 2u : (bit3 ? 3u : 4u)));
                const auto bit01 = lo & 0x8000000080000000u, bit23 = hi & 0x8000000080000000u;
                uint32_t ret = (bit01 | bit23) ? 1u : 0u;
                return { idx, ret };
            }
        }
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        const auto data = AsType<uint64x2_t>(static_cast<const T*>(this)->Data);
        const uint64_t lo = vgetq_lane_u64(data, 0), hi = vgetq_lane_u64(data, 1);
        const auto bit0 = lo & 0x80000000u, bit1 = lo & 0x8000000000000000u, bit2 = hi & 0x80000000u, bit3 = hi & 0x8000000000000000u;
        uint32_t ret = bit0 ? 1u : 0u;
        if (bit1) ret |= 2u;
        if (bit2) ret |= 4u;
        if (bit3) ret |= 8u;
        return ret;
        // USHR,USRA seems high latency, above can be less latency(more inst) when not piepeliened
        // // xxx .......3 xxx .......2 | xxx .......1 xxx .......0
        // const auto leastBits = AsType<uint64x2_t>(vshrq_n_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), 31));
        // // xxx .......3 xxx ......32 | xxx .......1 xxx ......10
        // const auto leastBits2 = AsType<uint8x16_t>(vsraq_n_u64(leastBits, leastBits, 31));
        // const uint32_t lo = vgetq_lane_u8(leastBits2, 0), hi = vgetq_lane_u8(leastBits2, 8);
        // return (hi << 2) + lo;
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 4, "shuffle index should be in [0,3]"); 
        using V = typename T::VecType;
        const auto data = AsType<uint32x4_t>(static_cast<const T*>(this)->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<V>(vdupq_laneq_u32(data, Idx));
#else
        return AsType<V>(vdupq_n_u32(vgetq_lane_u32(data, Idx)));
#endif
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Hi3>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        using V = typename T::VecType;
        const auto data = AsType<uint32x4_t>(static_cast<const T*>(this)->Data);
        if constexpr (Lo0 == 0 && Lo1 == 1 && Lo2 == 2 && Hi3 == 3)
            return AsType<V>(data);
        else if constexpr (Lo0 == Lo1 && Lo1 == Lo2 && Lo2 == Hi3)
            return Broadcast<Lo0>();
        else if constexpr (Lo1 == (Lo0 + 1) % 4 && Lo2 == (Lo1 + 1) % 4 && Hi3 == (Lo2 + 1) % 4)
            return AsType<V>(vextq_u32(data, data, Lo0));
        else if constexpr (Lo0 == Lo1 && Lo2 == Hi3) // xxyy
        {
            static_assert(Lo0 != Lo2);
            if constexpr (Lo0 == 0 && Lo2 == 1) // 0011
                return ZipLo(*static_cast<const T*>(this));
            else if constexpr (Lo0 == 2 && Lo2 == 3) // 2233
                return ZipHi(*static_cast<const T*>(this));
            else
#if COMMON_SIMD_LV >= 200
                return AsType<V>(vcombine_u32(vdup_laneq_u32(data, Lo0), vdup_laneq_u32(data, Lo2)));
#else
                return AsType<V>(vcombine_u32(
                    vdup_n_u32(vgetq_lane_u32(data, Lo0)), vdup_n_u32(vgetq_lane_u32(data, Lo2))
                    ));
#endif
        }
        else if constexpr (Lo0 == Lo2 && Lo1 == Hi3) // xyxy
        {
            static_assert(Lo0 != Lo1);
#if COMMON_SIMD_LV >= 200
            if constexpr (Lo0 == 0 && Lo1 == 2) // 0202
                return AsType<V>(vuzp1q_u32(data, data));
            else if constexpr (Lo0 == 1 && Lo1 == 3) // 1313
                return AsType<V>(vuzp2q_u32(data, data));
            else
                return AsType<V>(vzip1q_u32(vdupq_laneq_u32(data, Lo0), vdupq_laneq_u32(data, Lo1)));
#else
            if constexpr ((Lo0 == 0 && Lo1 == 2) || (Lo0 == 1 && Lo1 == 3)) // 0202 OR 1313
                return AsType<V>(vuzpq_u32(data, data).val[Lo0]);
            else
            {
                const auto a = vdup_n_u32(vgetq_lane_u32(data, Lo0)), b = vdup_n_u32(vgetq_lane_u32(data, Lo1));
                const auto zip = vzip_u32(a, b);
                return AsType<V>(vcombine_u32(zip.val[0], zip.val[1]));
            }
#endif
        }
        else
        {
            alignas(16) static constexpr uint32_t indexes[] =
            {
                (Lo0 * 4u) * 0x01010101u + 0x03020100u,
                (Lo1 * 4u) * 0x01010101u + 0x03020100u,
                (Lo2 * 4u) * 0x01010101u + 0x03020100u,
                (Hi3 * 4u) * 0x01010101u + 0x03020100u,
            };
#if COMMON_SIMD_LV >= 200
            const auto tbl = vreinterpretq_u8_u32(vld1q_u32(indexes));
            return AsType<V>(vqtbl1q_u8(vreinterpretq_u8_u32(data), tbl));
#else
            const uint8x8x2_t src = { vreinterpret_u8_u32(vget_low_u32(data)), vreinterpret_u8_u32(vget_high_u32(data)) };
            const auto tbl_ = vld1_u32_x2(indexes);
            const auto tbl1 = vreinterpret_u8_u32(tbl_.val[0]), tbl2 = vreinterpret_u8_u32(tbl_.val[1]);
            const auto lo = vtbl2_u8(src, tbl1), hi = vtbl2_u8(src, tbl2);
            return AsType<V>(vcombine_u32(vreinterpret_u32_u8(lo), vreinterpret_u32_u8(hi)));
#endif
        }
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Hi3) const noexcept
    {
        //static_assert(Lo0 < 4 && Lo1 < 4 && Lo2 < 4 && Hi3 < 4, "shuffle index should be in [0,3]");
        using V = typename T::VecType;
        alignas(16) const uint32_t indexes[] = { Lo0, Lo1, Lo2, Hi3, };
        const auto adder = vdupq_n_u32(0x03020100u);
        const auto tbl = vreinterpretq_u8_u32(vmlaq_n_u32(adder, vld1q_u32(indexes), 0x04040404u));
        const auto data = AsType<uint8x16_t>(static_cast<const T*>(this)->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<V>(vqtbl1q_u8(data, tbl));
#else
        const uint8x8x2_t src = { vget_low_u8(data), vget_high_u8(data) };
        const auto lo = vtbl2_u8(src, vget_low_u8(tbl)), hi = vtbl2_u8(src, vget_high_u8(tbl));
        return AsType<V>(vcombine_u32(vreinterpret_u32_u8(lo), vreinterpret_u32_u8(hi)));
#endif
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip1q_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(other.Data)));
#else
        const auto a = vget_low_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data)), b = vget_low_u32(AsType<uint32x4_t>(other.Data));
        const auto zip = vzip_u32(a, b);
        return AsType<SIMDType>(vcombine_u32(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipHi(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip2q_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(other.Data)));
#else
        const auto a = vget_high_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data)), b = vget_high_u32(AsType<uint32x4_t>(other.Data));
        const auto zip = vzip_u32(a, b);
        return AsType<SIMDType>(vcombine_u32(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn1q_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(other.Data)));
#else
        return AsType<SIMDType>(vsliq_n_u64(AsType<uint64x2_t>(static_cast<const T*>(this)->Data), AsType<uint64x2_t>(other.Data), 32));
#endif
    }
    forceinline T VECCALL ZipEven(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn2q_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(other.Data)));
#else
        return AsType<SIMDType>(vsriq_n_u64(AsType<uint64x2_t>(other.Data), AsType<uint64x2_t>(static_cast<const T*>(this)->Data), 32));
#endif
    }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, const T mask) const noexcept
    {
        uint32x4_t msk;
        if constexpr (Msk != MaskType::FullEle)
            msk = AsType<uint32x4_t>(vshrq_n_s32(AsType<int32x4_t>(mask.Data), 32)); // make sure all bits are covered
        else
            msk = AsType<uint32x4_t>(mask.Data);
        return AsType<SIMDType>(vbslq_u32(msk, AsType<uint32x4_t>(other.Data), AsType<uint32x4_t>(static_cast<const T*>(this)->Data)));
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        static_assert(Mask <= 0b1111, "Only allow 4 bits");
        if constexpr (Mask == 0b0000)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0b1111)
            return other;
        else
        {
            constexpr auto Mask8 = ::common::simd::detail::ConvertMaskTo8<32, 4>(Mask);
            static constexpr uint64_t mask[2] = { ::common::simd::detail::FullMask64[Mask8 & 0xff], ::common::simd::detail::FullMask64[(Mask8 >> 8) & 0xff] };
            return SelectWith<MaskType::FullEle>(other, AsType<SIMDType>(vld1q_u64(mask)));
        }
    }
    template<uint8_t Count>
    forceinline T VECCALL MoveToLoWith(const T& hi) const noexcept
    {
        static_assert(Count <= 4, "move count should be in [0,4]");
        if constexpr (Count == 0)
            return *static_cast<const T*>(this);
        else if constexpr (Count == 4)
            return hi;
        else
            return AsType<SIMDType>(vextq_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(hi.Data), Count));
    }
};


template<typename T, typename SIMDType, typename E>
struct alignas(16) Common32x4 : public Neon128Common<T, SIMDType, E, 4>, public Shuffle32Common<T, SIMDType>
{
    using Neon128Common<T, SIMDType, E, 4>::Neon128Common;
    forceinline T VECCALL SwapEndian() const noexcept
    {
        return AsType<SIMDType>(vrev32q_u8(AsType<uint8x16_t>(this->Data)));
    }

    // arithmetic operations
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint32x4_t>(this->Data);
        return AsType<SIMDType>(vshlq_u32(data, vdupq_n_s32(bits)));
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint32x4_t>(this->Data);
        return AsType<SIMDType>(vshlq_u32(data, vdupq_n_s32(-bits)));
    }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return vshlq_u32(this->Data, vdupq_n_s32(-bits));
        else
            return vshlq_s32(this->Data, vdupq_n_s32(-bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
        const auto data = AsType<uint32x4_t>(this->Data);
        return AsType<SIMDType>(vshlq_u32(data, AsType<int32x4_t>(bits.Data)));
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
        const auto data = AsType<uint32x4_t>(this->Data);
        const auto bits_ = vnegq_s32(AsType<int32x4_t>(bits.Data));
        return AsType<SIMDType>(vshlq_u32(data, bits_));
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 32)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint32x4_t>(this->Data);
            return AsType<SIMDType>(vshlq_n_u32(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 32)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint32x4_t>(this->Data);
            return AsType<SIMDType>(vshrq_n_u32(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        static_assert(N <= 32, "NEON limits right shift within [0,32]");
        if constexpr (N == 0)
            return this->Data;
        else
        {
            if constexpr (std::is_unsigned_v<E>)
                return vshrq_n_u32(this->Data, N);
            else
                return vshrq_n_s32(this->Data, N);
        }
    }

    forceinline T VECCALL operator*(const T& other) const noexcept { return static_cast<const T*>(this)->MulLo(other); }

    forceinline static T LoadLo(const E val) noexcept
    {
        return AsType<SIMDType>(vsetq_lane_u32(static_cast<uint32_t>(val), vdupq_n_u32(0), 0));
    }
    forceinline static T LoadLo(const E* ptr) noexcept
    {
        return LoadLo(*ptr);
    }
};


template<typename T, typename SIMDType, typename E>
struct alignas(16) Common16x8 : public Neon128Common<T, SIMDType, E, 8>
{
    using Neon128Common<T, SIMDType, E, 8>::Neon128Common;
    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        if constexpr (Msk == MaskType::FullEle)
        {
            const auto data = AsType<uint32x4_t>(this->Data);
            // 76 54 32 10
            const auto combined2 = vshrn_n_u32(data, 8);
            const auto sign8bits = vget_lane_u64(vreinterpret_u64_u16(combined2), 0);
            const auto idx = sign8bits ? CountTralingZero(sign8bits) >> 3 : 8u;
            if constexpr (NeedSignBits)
            {
                // .......1 .......0
                const auto least8bits = sign8bits & 0x0101010101010101u;
                // ......43 ......32 ......21 ......10
                const auto least8bits2 = least8bits | (least8bits >> 7);
                // .......7 ......76 .....765 ....7654 ....6543 ....5432 ....4321 ....3210
                const auto least8bits4 = least8bits2 | (least8bits2 >> 14);
                // .......7 ......76 .....765 ....7654 ...76543 ..765432 .7654321 76543210
                const auto least8bits8 = least8bits4 | (least8bits4 >> 28);
                uint32_t signbits = static_cast<uint8_t>(least8bits8);
                return { idx, signbits };
            }
            else
                return { idx, sign8bits ? 1u : 0u };
        }
        else
        {
            const auto signbits = ExtractSignBit();
            const auto ctzbits = signbits | 0x100u;
            return { CountTralingZero(ctzbits), signbits };
        }
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        // x .......3 x .......2 | x .......1 x .......0
        const auto leastBits = AsType<uint32x4_t>(vshrq_n_u16(AsType<uint16x8_t>(this->Data), 15));
        // x .......7 x ......76 x .......5 x ......54 | x .......3 x ......32 x .......1 x ......10
        const auto leastBits2 = AsType<uint64x2_t>(vsraq_n_u32(leastBits, leastBits, 15));
        // x .......7 x ......76 x .....7.5 x ....7654 | x .......3 x ......32 x .....3.1 x ....3210
        const auto leastBits4 = AsType<uint8x16_t>(vsraq_n_u64(leastBits2, leastBits2, 30));
        const uint32_t lo = vgetq_lane_u8(leastBits4, 0), hi = vgetq_lane_u8(leastBits4, 8);
        return (hi << 4) + lo;
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 8, "shuffle index should be in [0,7]");
        using V = typename T::VecType;
        const auto data = AsType<uint16x8_t>(this->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<V>(vdupq_laneq_u16(data, Idx));
#else
        return AsType<V>(vdupq_n_u16(vgetq_lane_u16(data, Idx)));
#endif
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Hi7>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 8 && Lo1 < 8 && Lo2 < 8 && Lo3 < 8 && Lo4 < 8 && Lo5 < 8 && Lo6 < 8 && Hi7 < 8, "shuffle index should be in [0,7]");
        using V = typename T::VecType;
        const auto data = AsType<uint16x8_t>(this->Data);
        if constexpr (Lo0 == 0 && Lo1 == 1 && Lo2 == 2 && Lo3 == 3 && Lo4 == 4 && Lo5 == 5 && Lo6 == 6 && Hi7 == 7)
            return AsType<V>(data);
        else if constexpr (Lo0 == Lo1 && Lo1 == Lo2 && Lo2 == Lo3 && Lo3 == Lo4 && Lo5 == Lo6 && Lo6 == Hi7)
            return Broadcast<Lo0>();
        else if constexpr (Lo1 == (Lo0 + 1) % 8 && Lo2 == (Lo1 + 1) % 8 && Lo3 == (Lo2 + 1) % 8 && 
            Lo4 == (Lo3 + 1) % 8 && Lo5 == (Lo4 + 1) % 8 && Lo6 == (Lo5 + 1) % 8 && Hi7 == (Lo6 + 1) % 8)
            return AsType<V>(vextq_u16(data, data, Lo0));
        else if constexpr (Lo0 == 0 && Lo1 == 0 && Lo2 == 1 && Lo3 == 1 && Lo4 == 2 && Lo5 == 2 && Lo6 == 3 && Hi7 == 3) // 00112233
            return ZipLo(*static_cast<const T*>(this));
        else if constexpr (Lo0 == 4 && Lo1 == 4 && Lo2 == 5 && Lo3 == 5 && Lo4 == 6 && Lo5 == 6 && Lo6 == 7 && Hi7 == 7) // 44556677
            return ZipHi(*static_cast<const T*>(this));
        else if constexpr (Lo0 == 0 && Lo1 == 2 && Lo2 == 4 && Lo3 == 6 && Lo4 == 0 && Lo5 == 2 && Lo6 == 4 && Hi7 == 6) // 02460246
#if COMMON_SIMD_LV >= 200
            return AsType<V>(vuzp1q_u16(data, data));
#else
            return AsType<V>(vuzpq_u16(data, data).val[0]);
#endif
        else if constexpr (Lo0 == 1 && Lo1 == 3 && Lo2 == 5 && Lo3 == 7 && Lo4 == 1 && Lo5 == 3 && Lo6 == 5 && Hi7 == 7) // 13571357
#if COMMON_SIMD_LV >= 200
            return AsType<V>(vuzp2q_u16(data, data));
#else
            return AsType<V>(vuzpq_u16(data, data).val[1]);
#endif
        else if constexpr (Lo0 == Lo2 && Lo2 == Lo4 && Lo4 == Lo6 && Lo1 == Lo3 && Lo3 == Lo5 && Lo5 == Hi7) // xyxyxyxy
        {
#if COMMON_SIMD_LV >= 200
            return AsType<V>(vzip1q_u16(vdupq_laneq_u16(data, Lo0), vdupq_laneq_u16(data, Lo1)));
#else
            const auto a = vdup_n_u16(vgetq_lane_u16(data, Lo0)), b = vdup_n_u16(vgetq_lane_u16(data, Lo1));
            const auto zip = vzip_u16(a, b);
            return AsType<V>(vcombine_u16(zip.val[0], zip.val[1]));
#endif
        }
        else
        {
            alignas(16) static const uint16_t indexes[] =
            {
                (Lo0 * 2) * 0x0101 + 0x0100,
                (Lo1 * 2) * 0x0101 + 0x0100,
                (Lo2 * 2) * 0x0101 + 0x0100,
                (Lo3 * 2) * 0x0101 + 0x0100,
                (Lo4 * 2) * 0x0101 + 0x0100,
                (Lo5 * 2) * 0x0101 + 0x0100,
                (Lo6 * 2) * 0x0101 + 0x0100,
                (Hi7 * 2) * 0x0101 + 0x0100,
            };
#if COMMON_SIMD_LV >= 200
            const auto tbl = vreinterpretq_u8_u16(vld1q_u16(indexes));
            return AsType<V>(vqtbl1q_u8(vreinterpretq_u8_u16(data), tbl));
#else
            const uint8x8x2_t src = { vreinterpret_u8_u16(vget_low_u16(data)), vreinterpret_u8_u16(vget_high_u16(data)) };
            const auto tbl_ = vld1_u16_x2(indexes);
            const auto tbl1 = vreinterpret_u8_u16(tbl_.val[0]), tbl2 = vreinterpret_u8_u16(tbl_.val[1]);
            const auto lo = vtbl2_u8(src, tbl1), hi = vtbl2_u8(src, tbl2);
            return AsType<V>(vcombine_u32(vreinterpret_u16_u8(lo), vreinterpret_u16_u8(hi)));
#endif
        }
    }
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Hi7) const noexcept
    {
        alignas(16) const uint8_t indexes[] = { Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7 };
        const auto base = vmovl_u8(vld1_u8(indexes));
        const auto adder = vdupq_n_u16(0x0100);
        const auto tbl = vreinterpretq_u8_u16(vmlaq_n_u16(adder, base, 0x0202));
        const auto data = AsType<uint8x16_t>(this->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vqtbl1q_u8(data, tbl));
#else
        const uint8x8x2_t src = { vget_low_u8(data), vget_high_u8(data) };
        const auto lo = vtbl2_u8(src, vget_low_u8(tbl)), hi = vtbl2_u8(src, vget_high_u8(tbl));
        return AsType<SIMDType>(vcombine_u16(vreinterpret_u16_u8(lo), vreinterpret_u16_u8(hi)));
#endif
    }
    forceinline T VECCALL SwapEndian() const noexcept
    {
        return AsType<SIMDType>(vrev16q_u8(AsType<uint8x16_t>(this->Data)));
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip1q_u16(AsType<uint16x8_t>(this->Data), AsType<uint16x8_t>(other.Data)));
#else
        const auto a = vget_low_u16(AsType<uint16x8_t>(this->Data)), b = vget_low_u16(AsType<uint16x8_t>(other.Data));
        const auto zip = vzip_u16(a, b);
        return AsType<SIMDType>(vcombine_u16(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipHi(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip2q_u16(AsType<uint16x8_t>(this->Data), AsType<uint16x8_t>(other.Data)));
#else
        const auto a = vget_high_u16(AsType<uint16x8_t>(this->Data)), b = vget_high_u16(AsType<uint16x8_t>(other.Data));
        const auto zip = vzip_u16(a, b);
        return AsType<SIMDType>(vcombine_u16(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn1q_u16(AsType<uint16x8_t>(static_cast<const T*>(this)->Data), AsType<uint16x8_t>(other.Data)));
#else
        return AsType<SIMDType>(vsliq_n_u32(AsType<uint32x4_t>(static_cast<const T*>(this)->Data), AsType<uint32x4_t>(other.Data), 16));
#endif
    }
    forceinline T VECCALL ZipEven(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn2q_u16(AsType<uint16x8_t>(static_cast<const T*>(this)->Data), AsType<uint16x8_t>(other.Data)));
#else
        return AsType<SIMDType>(vsriq_n_u32(AsType<uint32x4_t>(other.Data), AsType<uint32x4_t>(static_cast<const T*>(this)->Data), 16));
#endif
    }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, const T mask) const noexcept
    {
        uint16x8_t msk;
        if constexpr (Msk != MaskType::FullEle)
            msk = AsType<uint16x8_t>(vshrq_n_s16(AsType<int16x8_t>(mask.Data), 16)); // make sure all bits are covered
        else
            msk = AsType<uint16x8_t>(mask.Data);
        return AsType<SIMDType>(vbslq_u16(msk, AsType<uint16x8_t>(other.Data), AsType<uint16x8_t>(this->Data)));
    }
    template<uint8_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        if constexpr (Mask == 0b00000000)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0b11111111)
            return other;
        else
        {
            constexpr auto Mask8 = ::common::simd::detail::ConvertMaskTo8<16, 8>(Mask);
            static constexpr uint64_t mask[2] = { ::common::simd::detail::FullMask64[Mask8 & 0xff], ::common::simd::detail::FullMask64[(Mask8 >> 8) & 0xff] };
            return SelectWith<MaskType::FullEle>(other, AsType<SIMDType>(vld1q_u64(mask)));
        }
    }
    template<uint8_t Count>
    forceinline T VECCALL MoveToLoWith(const T& hi) const noexcept
    {
        static_assert(Count <= 8, "move count should be in [0,8]");
        if constexpr (Count == 0)
            return this->Data;
        else if constexpr (Count == 8)
            return hi;
        else
            return AsType<SIMDType>(vextq_u16(AsType<uint16x8_t>(this->Data), AsType<uint16x8_t>(hi.Data), Count));
    }

    // arithmetic operations
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint16x8_t>(this->Data);
        return AsType<SIMDType>(vshlq_u16(data, vdupq_n_s16(bits)));
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint16x8_t>(this->Data);
        return AsType<SIMDType>(vshlq_u16(data, vdupq_n_s16(-bits)));
    }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return vshlq_u16(this->Data, vdupq_n_s16(-bits));
        else
            return vshlq_s16(this->Data, vdupq_n_s16(-bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    forceinline T VECCALL ShiftLeftLogic (const T bits) const noexcept
    {
        const auto data = AsType<uint16x8_t>(this->Data);
        return AsType<SIMDType>(vshlq_u16(data, AsType<int16x8_t>(bits.Data)));
    }
    forceinline T VECCALL ShiftRightLogic(const T bits) const noexcept
    {
        const auto data = AsType<uint16x8_t>(this->Data);
        const auto bits_ = vnegq_s16(AsType<int16x8_t>(bits.Data));
        return AsType<SIMDType>(vshlq_u16(data, bits_));
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 16)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint16x8_t>(this->Data);
            return AsType<SIMDType>(vshlq_n_u16(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 16)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint16x8_t>(this->Data);
            return AsType<SIMDType>(vshrq_n_u16(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        static_assert(N <= 16, "NEON limits right shift within [0,16]");
        if constexpr (N == 0)
            return this->Data;
        else
        {
            if constexpr (std::is_unsigned_v<E>)
                return vshrq_n_u16(this->Data, N);
            else
                return vshrq_n_s16(this->Data, N);
        }
    }

    forceinline T VECCALL operator*(const T& other) const noexcept { return static_cast<const T*>(this)->MulLo(other); }

    forceinline static T LoadLo(const E val) noexcept
    {
        return AsType<SIMDType>(vsetq_lane_u16(static_cast<uint16_t>(val), vdupq_n_u16(0), 0));
    }
    forceinline static T LoadLo(const E* ptr) noexcept
    {
        return LoadLo(*ptr);
    }
};


template<typename T, typename SIMDType, typename E>
struct alignas(16) Common8x16 : public Neon128Common<T, SIMDType, E, 16>
{
    using Neon128Common<T, SIMDType, E, 16>::Neon128Common;
    // shuffle operations
    template<MaskType Msk, bool NeedSignBits = false>
    forceinline std::pair<uint32_t, uint32_t> VECCALL GetMaskFirstIndex() const noexcept
    {
        if constexpr (Msk == MaskType::FullEle)
        {
            const auto data = AsType<uint16x8_t>(this->Data);
            // fe dc ba 98 76 54 32 10
            const auto combined2 = vshrn_n_u16(data, 4);
            const auto sign4bits = vget_lane_u64(vreinterpret_u64_u8(combined2), 0);
            const auto idx = sign4bits ? CountTralingZero(sign4bits) >> 2 : 16u;
            if constexpr (NeedSignBits)
            {
                // ...3...2 ...1...0
                const auto least4bits = sign4bits & 0x1111111111111111u;
                // ..87..76 ..65..54 ..43..32 ..21..10
                const auto least4bits2 = least4bits | (least4bits >> 3);
                // ...f..fe .fedfedc edcbdcba cba9ba98 a9879876 87657654 65435432 43213210
                const auto least4bits4 = least4bits2 | (least4bits2 >> 6);
                // XX Xfedc XX Xba98 XX X7654 XX X3210
                const auto clean4bits4 = least4bits4 & 0x000f000f000f000fu;
                // XX Xfedc XX fedcba98 XX ba987654 XX 76543210
                const auto clean4bits8 = clean4bits4 | (clean4bits4 >> 12);
                // XX Xfedc XX fedcba98 Xfedc ba987654 fedcba98 76543210
                const auto clean4bits16 = clean4bits8 | (clean4bits8 >> 24);
                uint32_t signbits = static_cast<uint16_t>(clean4bits16);
                return { idx, signbits };
            }
            else
                return { idx, sign4bits ? 1u : 0u };
        }
        else
        {
            const auto signbits = ExtractSignBit();
            const auto ctzbits = signbits | 0x10000u;
            return { CountTralingZero(ctzbits), signbits };
        }
    }
    forceinline uint32_t VECCALL ExtractSignBit() const noexcept
    {
        // .......3 .......2 | .......1 .......0
        const auto leastBits  = AsType<uint16x8_t>(vshrq_n_u8(AsType<uint8x16_t>(this->Data), 7));
        // .......7 ......76 .......5 ......54 | .......3 ......32 .......1 ......10
        const auto leastBits2 = AsType<uint32x4_t>(vsraq_n_u16(leastBits,  leastBits,   7));
        // .......f ......fe .....f.d ....fedc .......b ......ba .....b.9 ....ba98 | .......7 ......76 .....7.5 ....7654 .......3 ......32 .....3.1 ....3210
        const auto leastBits4 = AsType<uint64x2_t>(vsraq_n_u32(leastBits2, leastBits2, 14));
        // .......f ......fe .....f.d ....fedc ...f...b ..fe..ba .f.d.b.9 fedcba98 | .......7 ......76 .....7.5 ....7654 ...7...3 ..76..32 .7.5.3.1 76543210
        const auto leastBits8 = AsType<uint8x16_t>(vsraq_n_u64(leastBits4, leastBits4, 28));
        const uint32_t lo = vgetq_lane_u8(leastBits8, 0), hi = vgetq_lane_u8(leastBits8, 8);
        return (hi << 8) + lo;
    }
    template<uint8_t Idx>
    forceinline T VECCALL Broadcast() const noexcept
    {
        static_assert(Idx < 16, "shuffle index should be in [0,15]");
        using V = typename T::VecType;
        const auto data = AsType<uint16x8_t>(this->Data);
#if COMMON_SIMD_LV >= 200
        return AsType<V>(vdupq_laneq_u8(data, Idx));
#else
        return AsType<V>(vdupq_n_u8(vgetq_lane_u8(data, Idx)));
#endif
    }
    template<uint8_t Lo0, uint8_t Lo1, uint8_t Lo2, uint8_t Lo3, uint8_t Lo4, uint8_t Lo5, uint8_t Lo6, uint8_t Lo7, uint8_t Lo8, uint8_t Lo9, uint8_t Lo10, uint8_t Lo11, uint8_t Lo12, uint8_t Lo13, uint8_t Lo14, uint8_t Hi15>
    forceinline T VECCALL Shuffle() const noexcept
    {
        static_assert(Lo0 < 16 && Lo1 < 16 && Lo2 < 16 && Lo3 < 16 && Lo4 < 16 && Lo5 < 16 && Lo6 < 16 && Lo7 < 16
            && Lo8 < 16 && Lo9 < 16 && Lo10 < 16 && Lo11 < 16 && Lo12 < 16 && Lo13 < 16 && Lo14 < 16 && Hi15 < 16, "shuffle index should be in [0,15]");
        alignas(16) constexpr uint8_t indexes[] = { Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Lo7, Lo8, Lo9, Lo10, Lo11, Lo12, Lo13, Lo14, Hi15 };
        const auto tbl = vld1q_u8(indexes);
        return Shuffle(tbl);
    }
    forceinline T VECCALL Shuffle(const U8x16& pos) const noexcept;
    forceinline T VECCALL Shuffle(const uint8_t Lo0, const uint8_t Lo1, const uint8_t Lo2, const uint8_t Lo3, const uint8_t Lo4, const uint8_t Lo5, const uint8_t Lo6, const uint8_t Lo7,
        const uint8_t Lo8, const uint8_t Lo9, const uint8_t Lo10, const uint8_t Lo11, const uint8_t Lo12, const uint8_t Lo13, const uint8_t Lo14, const uint8_t Hi15) const noexcept
    {
        alignas(16) const uint8_t indexes[] = { Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Lo7, Lo8, Lo9, Lo10, Lo11, Lo12, Lo13, Lo14, Hi15 };
        const auto tbl = vld1q_u8(indexes);
        return Shuffle(tbl);
    }
    forceinline T VECCALL ZipLo(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip1q_u8(AsType<uint8x16_t>(this->Data), AsType<uint8x16_t>(other.Data)));
#else
        const auto a = vget_low_u8(AsType<uint8x16_t>(this->Data)), b = vget_low_u8(AsType<uint8x16_t>(other.Data));
        const auto zip = vzip_u8(a, b);
        return AsType<SIMDType>(vcombine_u8(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipHi(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vzip2q_u8(AsType<uint8x16_t>(this->Data), AsType<uint8x16_t>(other.Data)));
#else
        const auto a = vget_high_u8(AsType<uint8x16_t>(this->Data)), b = vget_high_u8(AsType<uint8x16_t>(other.Data));
        const auto zip = vzip_u8(a, b);
        return AsType<SIMDType>(vcombine_u8(zip.val[0], zip.val[1]));
#endif
    }
    forceinline T VECCALL ZipOdd(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn1q_u8(AsType<uint8x16_t>(static_cast<const T*>(this)->Data), AsType<uint8x16_t>(other.Data)));
#else
        return AsType<SIMDType>(vsliq_n_u16(AsType<uint16x8_t>(static_cast<const T*>(this)->Data), AsType<uint16x8_t>(other.Data), 8));
#endif
    }
    forceinline T VECCALL ZipEven(const T& other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return AsType<SIMDType>(vtrn2q_u8(AsType<uint8x16_t>(static_cast<const T*>(this)->Data), AsType<uint8x16_t>(other.Data)));
#else
        return AsType<SIMDType>(vsriq_n_u16(AsType<uint16x8_t>(other.Data), AsType<uint16x8_t>(static_cast<const T*>(this)->Data), 8));
#endif
    }
    template<MaskType Msk>
    forceinline T VECCALL SelectWith(const T& other, const T mask) const noexcept
    {
        uint8x16_t msk;
        if constexpr (Msk != MaskType::FullEle)
            msk = AsType<uint8x16_t>(vshrq_n_s8(AsType<int8x16_t>(mask.Data), 8)); // make sure all bits are covered
        else
            msk = AsType<uint8x16_t>(mask.Data);
        return AsType<SIMDType>(vbslq_u8(msk, AsType<uint8x16_t>(other.Data), AsType<uint8x16_t>(this->Data)));
    }
    template<uint16_t Mask>
    forceinline T VECCALL SelectWith(const T& other) const noexcept
    {
        if constexpr (Mask == 0x0)
            return *static_cast<const T*>(this);
        else if constexpr (Mask == 0xffff)
            return other;
        else
        {
            static constexpr uint64_t mask[2] = { ::common::simd::detail::FullMask64[Mask & 0xff], ::common::simd::detail::FullMask64[(Mask >> 8) & 0xff] };
            return SelectWith<MaskType::FullEle>(other, AsType<SIMDType>(vld1q_u64(mask)));
        }
    }
    template<uint8_t Count>
    forceinline T VECCALL MoveToLoWith(const T& hi) const noexcept
    {
        static_assert(Count <= 16, "move count should be in [0,16]");
        if constexpr (Count == 0)
            return this->Data;
        else if constexpr (Count == 16)
            return hi;
        else
            return AsType<SIMDType>(vextq_u8(AsType<uint8x16_t>(this->Data), AsType<uint8x16_t>(hi.Data), Count));
    }

    // arithmetic operations
    forceinline T VECCALL ShiftLeftLogic (const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint8x16_t>(this->Data);
        return AsType<SIMDType>(vshlq_u8(data, vdupq_n_s8(bits)));
    }
    forceinline T VECCALL ShiftRightLogic(const uint8_t bits) const noexcept
    {
        const auto data = AsType<uint8x16_t>(this->Data);
        return AsType<SIMDType>(vshlq_u8(data, vdupq_n_s8(-bits)));
    }
    forceinline T VECCALL ShiftRightArith(const uint8_t bits) const noexcept
    { 
        if constexpr (std::is_unsigned_v<E>)
            return vshlq_u8(this->Data, vdupq_n_s8(-bits));
        else
            return vshlq_s8(this->Data, vdupq_n_s8(-bits));
    }
    forceinline T VECCALL operator<<(const uint8_t bits) const noexcept { return ShiftLeftLogic (bits); }
    forceinline T VECCALL operator>>(const uint8_t bits) const noexcept { return ShiftRightArith(bits); }
    template<uint8_t N>
    forceinline T VECCALL ShiftLeftLogic () const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 8)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint8x16_t>(this->Data);
            return AsType<SIMDType>(vshlq_n_u8(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightLogic() const noexcept
    {
        if constexpr (N == 0)
            return this->Data;
        else if constexpr (N >= 8)
            return T::AllZero();
        else
        {
            const auto data = AsType<uint8x16_t>(this->Data);
            return AsType<SIMDType>(vshrq_n_u8(data, N));
        }
    }
    template<uint8_t N>
    forceinline T VECCALL ShiftRightArith() const noexcept
    { 
        static_assert(N <= 8, "NEON limits right shift within [0,8]");
        if constexpr (N == 0)
            return this->Data;
        else
        {
            if constexpr (std::is_unsigned_v<E>)
                return vshrq_n_u8(this->Data, N);
            else
                return vshrq_n_s8(this->Data, N);
        }
    }

    forceinline T VECCALL operator*(const T& other) const noexcept { return static_cast<const T*>(this)->MulLo(other); }

    forceinline static T LoadLo(const E val) noexcept
    {
        return AsType<SIMDType>(vsetq_lane_u8(static_cast<uint16_t>(val), vdupq_n_u8(0), 0));
    }
    forceinline static T LoadLo(const E* ptr) noexcept
    {
        return LoadLo(*ptr);
    }
};


}


#if COMMON_SIMD_LV >= 200
struct alignas(16) F64x2 : public detail::Neon128Common<F64x2, float64x2_t, double, 2>, public detail::Shuffle64Common<F64x2, float64x2_t>
{
    using Neon128Common<F64x2, float64x2_t, double, 2>::Neon128Common;
    forceinline explicit F64x2(const double* ptr) noexcept : Neon128Common(vld1q_f64(ptr)) { }
    forceinline F64x2(const double val) noexcept : Neon128Common(vdupq_n_f64(val)) { }
    forceinline F64x2(const double lo, const double hi) noexcept : Neon128Common(vcombine_f64(vdup_n_f64(lo), vdup_n_f64(hi))) { }
    forceinline void VECCALL Load(const double *ptr) noexcept { Data = vld1q_f64(ptr); }
    forceinline void VECCALL Save(double *ptr) const noexcept { vst1q_f64(reinterpret_cast<float64_t*>(ptr), Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F64x2 VECCALL Compare(const F64x2 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<float64x2_t>(vcltq_f64(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<float64x2_t>(vcleq_f64(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<float64x2_t>(vceqq_f64(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<float64x2_t>(vcgeq_f64(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<float64x2_t>(vcgtq_f64(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not(); // TODO: Handle NaN
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline F64x2 VECCALL Add(const F64x2& other) const noexcept { return vaddq_f64(Data, other.Data); }
    forceinline F64x2 VECCALL Sub(const F64x2& other) const noexcept { return vsubq_f64(Data, other.Data); }
    forceinline F64x2 VECCALL Mul(const F64x2& other) const noexcept { return vmulq_f64(Data, other.Data); }
    forceinline F64x2 VECCALL Div(const F64x2& other) const noexcept { return vdivq_f64(Data, other.Data); }
    forceinline F64x2 VECCALL Neg() const noexcept { return vnegq_f64(Data); }
    forceinline F64x2 VECCALL Abs() const noexcept { return vabsq_f64(Data); }
    forceinline F64x2 VECCALL Max(const F64x2& other) const noexcept { return vmaxq_f64(Data, other.Data); }
    forceinline F64x2 VECCALL Min(const F64x2& other) const noexcept { return vminq_f64(Data, other.Data); }
    //F64x2 Rcp() const { return _mm_rcp_pd(Data); }
    forceinline F64x2 VECCALL Sqrt() const { return vsqrtq_f64(Data); }
    //F64x2 Rsqrt() const { return _mm_rsqrt_pd(Data); }
    forceinline F64x2 VECCALL MulAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        return vfmaq_f64(adder.Data, Data, muler.Data);
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL MulScalarAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return vfmaq_laneq_f64(adder.Data, Data, muler.Data, Idx);
    }
    forceinline F64x2 VECCALL MulSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        return MulAdd(muler, vnegq_f64(suber.Data));
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL MulScalarSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return MulScalarAdd<Idx>(muler, vnegq_f64(suber.Data));
    }
    forceinline F64x2 VECCALL NMulAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        return vfmsq_f64(adder.Data, Data, muler.Data);
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulScalarAdd(const F64x2& muler, const F64x2& adder) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return vfmsq_laneq_f64(adder.Data, Data, muler.Data, Idx);
    }
    forceinline F64x2 VECCALL NMulSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        return vnegq_f64(MulAdd(muler, suber));
    }
    template<size_t Idx>
    forceinline F64x2 VECCALL NMulScalarSub(const F64x2& muler, const F64x2& suber) const noexcept
    {
        static_assert(Idx < 2, "select index should be in [0,1]");
        return vnegq_f64(MulScalarAdd<Idx>(muler, suber));
    }
    forceinline F64x2 VECCALL operator*(const F64x2& other) const noexcept { return Mul(other); }
    forceinline F64x2 VECCALL operator/(const F64x2& other) const noexcept { return Div(other); }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F64x2, T>(), typename... Args>
    forceinline typename CastTyper<F64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F64x2 VECCALL Round() const noexcept
    {
        switch (Mode)
        {
        case RoundMode::ToEven:     return vrndnq_f64(Data);
        case RoundMode::ToZero:     return vrndq_f64 (Data);
        case RoundMode::ToPosInf:   return vrndpq_f64(Data);
        case RoundMode::ToNegInf:   return vrndmq_f64(Data);
        default:                    return vrndiq_f64(Data);
        }
    }
};
#endif


struct alignas(16) F32x4 : public detail::Neon128Common<F32x4, float32x4_t, float, 4>, public detail::Shuffle32Common<F32x4, float32x4_t>
{
    using Neon128Common<F32x4, float32x4_t, float, 4>::Neon128Common;
    forceinline explicit F32x4(const float* ptr) noexcept : Neon128Common(vld1q_f32(ptr)) { }
    forceinline F32x4(const float val) noexcept : Neon128Common(vdupq_n_f32(val)) { }
    forceinline F32x4(const float lo0, const float lo1, const float lo2, const float hi3) noexcept
    {
        alignas(16) float tmp[] = { lo0, lo1, lo2, hi3 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const float *ptr) noexcept { Data = vld1q_f32(ptr); }
    forceinline void VECCALL Save(float *ptr) const noexcept { vst1q_f32(reinterpret_cast<float32_t*>(ptr), Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline F32x4 VECCALL Compare(const F32x4 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<float32x4_t>(vcltq_f32(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<float32x4_t>(vcleq_f32(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<float32x4_t>(vceqq_f32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<float32x4_t>(vcgeq_f32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<float32x4_t>(vcgtq_f32(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not(); // TODO: Handle NaN
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline F32x4 VECCALL Add(const F32x4& other) const noexcept { return vaddq_f32(Data, other.Data); }
    forceinline F32x4 VECCALL Sub(const F32x4& other) const noexcept { return vsubq_f32(Data, other.Data); }
    forceinline F32x4 VECCALL Mul(const F32x4& other) const noexcept { return vmulq_f32(Data, other.Data); }
    forceinline F32x4 VECCALL Div(const F32x4& other) const noexcept 
    {
#if COMMON_SIMD_LV >= 200
        return vdivq_f32(Data, other.Data);
#else
        auto rcp = other.Rcp();
        // https://github.com/DLTcollab/sse2neon#compile-time-configurations
        // Add 2 round for accuracy
        rcp = vmulq_f32(rcp, vrecpsq_f32(rcp, other.Data));
        rcp = vmulq_f32(rcp, vrecpsq_f32(rcp, other.Data));
        return Mul(rcp);
#endif
    }
    forceinline F32x4 VECCALL Neg() const noexcept { return vnegq_f32(Data); }
    forceinline F32x4 VECCALL Abs() const noexcept { return vabsq_f32(Data); }
    forceinline F32x4 VECCALL Max(const F32x4& other) const noexcept { return vmaxq_f32(Data, other.Data); }
    forceinline F32x4 VECCALL Min(const F32x4& other) const noexcept { return vminq_f32(Data, other.Data); }
    forceinline F32x4 VECCALL Rcp() const noexcept
    {
        auto rcp = vrecpeq_f32(Data);
        rcp = vmulq_f32(rcp, vrecpsq_f32(rcp, Data));
        // https://github.com/DLTcollab/sse2neon#compile-time-configurations
        // optional round
        // rcp = vmulq_f32(rcp, vrecpsq_f32(rcp, Data));
        return rcp;
    }
    forceinline F32x4 VECCALL Sqrt() const noexcept
    { 
#if COMMON_SIMD_LV >= 200
        return vsqrtq_f32(Data);
#else
        return Rsqrt().Rcp();
#endif
    }
    forceinline F32x4 VECCALL Rsqrt() const noexcept
    {
        auto rsqrt = vrsqrteq_f32(Data);
        // https://github.com/DLTcollab/sse2neon#compile-time-configurations
        // optional round
        // rsqrt = vmulq_f32(rsqrt, vrsqrtsq_f32(vmulq_f32(rsqrt, Data), rsqrt));
        // rsqrt = vmulq_f32(rsqrt, vrsqrtsq_f32(vmulq_f32(rsqrt, Data), rsqrt));
        return rsqrt;
    }
    forceinline F32x4 VECCALL MulAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 40
        return vfmaq_f32(adder.Data, Data, muler.Data);
#else
        return vmlaq_f32(adder.Data, Data, muler.Data);
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL MulScalarAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
        return vfmaq_laneq_f32(adder.Data, Data, muler.Data, Idx);
#else
        if constexpr (Idx < 2)
            return vmlaq_lane_f32(adder.Data, Data, vget_low_f32(muler.Data), Idx);
        else
            return vmlaq_lane_f32(adder.Data, Data, vget_high_f32(muler.Data), Idx - 2);
#endif
    }
    forceinline F32x4 VECCALL MulSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        return MulAdd(muler, vnegq_f32(suber.Data));
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL MulScalarSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulScalarAdd<Idx>(muler, vnegq_f32(suber.Data));
    }
    forceinline F32x4 VECCALL NMulAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
#if COMMON_SIMD_LV >= 40
        return vfmsq_f32(adder.Data, Data, muler.Data);
#else
        return vmlsq_f32(adder.Data, Data, muler.Data);
#endif
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulScalarAdd(const F32x4& muler, const F32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
        return vfmsq_laneq_f32(adder.Data, Data, muler.Data, Idx);
#else
        if constexpr (Idx < 2)
            return vmlsq_lane_f32(adder.Data, Data, vget_low_f32(muler.Data), Idx);
        else
            return vmlsq_lane_f32(adder.Data, Data, vget_high_f32(muler.Data), Idx - 2);
#endif
    }
    forceinline F32x4 VECCALL NMulSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        return vnegq_f32(MulAdd(muler, suber));
    }
    template<size_t Idx>
    forceinline F32x4 VECCALL NMulScalarSub(const F32x4& muler, const F32x4& suber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return vnegq_f32(MulScalarAdd<Idx>(muler, suber));
    }
    template<DotPos Mul>
    forceinline float VECCALL Dot(const F32x4& other) const noexcept
    {
        constexpr auto MulExclude = Mul ^ DotPos::XYZW;
        if constexpr (MulExclude == DotPos::XYZW)
            return 0;
        else
        {
            constexpr bool MulEx0 = HAS_FIELD(MulExclude, DotPos::X), MulEx1 = HAS_FIELD(MulExclude, DotPos::Y),
                MulEx2 = HAS_FIELD(MulExclude, DotPos::Z), MulEx3 = HAS_FIELD(MulExclude, DotPos::W);
            auto prod = this->Mul(other).Data;
            if constexpr (MulEx0 + MulEx1 + MulEx2 + MulEx3 == 0) // all need
            { }
            else if constexpr (MulEx0 + MulEx1 + MulEx2 + MulEx3 == 1)
            {
                prod = vsetq_lane_f32(0, prod, MulEx0 ? 0 : (MulEx1 ? 1 : (MulEx2 ? 2 : 3)));
            }
            else if constexpr (MulEx0 == MulEx1 && MulEx2 == MulEx3)
            {
                static_assert(MulEx0 + MulEx1 + MulEx2 + MulEx3 == 2);
                prod = vreinterpretq_f32_u64(vsetq_lane_u64(0, vreinterpretq_u64_f32(prod), MulEx0 ? 0 : 1));
            }
            else
            {
                alignas(16) const int32_t mask[4] = { MulEx0 ? 0 : -1, MulEx1 ? 0 : -1, MulEx2 ? 0 : -1, MulEx3 ? 0 : -1 };
                prod = vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(prod), vld1q_s32(mask)));
            }
#if COMMON_SIMD_LV >= 200
            return vaddvq_f32(prod);
#else
            const auto part = vpadd_f32(vget_low_f32(prod), vget_high_f32(prod));
            return vget_lane_f32(vpadd_f32(part, part), 0);
#endif
        }
    }
    template<DotPos Mul, DotPos Res>
    forceinline F32x4 VECCALL Dot(const F32x4& other) const noexcept
    {
        constexpr auto ResExclude = Res ^ DotPos::XYZW;
        if constexpr (ResExclude == DotPos::XYZW)
        {
            return 0;
        }
        else
        {
            constexpr bool ResEx0 = HAS_FIELD(ResExclude, DotPos::X), ResEx1 = HAS_FIELD(ResExclude, DotPos::Y),
                ResEx2 = HAS_FIELD(ResExclude, DotPos::Z), ResEx3 = HAS_FIELD(ResExclude, DotPos::W);
            const float sum = Dot<Mul>(other);
            if constexpr (ResEx0 + ResEx1 + ResEx2 + ResEx3 == 0) // all need
            {
                return sum;
            }
            else if constexpr (ResEx0 + ResEx1 + ResEx2 + ResEx3 == 1)
            {
                return vsetq_lane_f32(0, vdupq_n_f32(sum), ResEx0 ? 0 : (ResEx1 ? 1 : (ResEx2 ? 2 : 3)));
            }
            else if constexpr (ResEx0 + ResEx1 + ResEx2 + ResEx3 == 3) // only need one
            {
                return vsetq_lane_f32(sum, vdupq_n_f32(0), ResEx0 ? (ResEx1 ? (ResEx2 ? 3 : 2) : 1) : 0);
            }
            else
            {
                alignas(16) const int32_t mask[4] = { ResEx0 ? 0 : -1, ResEx1 ? 0 : -1, ResEx2 ? 0 : -1, ResEx3 ? 0 : -1 };
                return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(vdupq_n_f32(sum)), vld1q_s32(mask)));
            }
        }
    }
    forceinline F32x4 VECCALL operator*(const F32x4& other) const noexcept { return Mul(other); }
    forceinline F32x4 VECCALL operator/(const F32x4& other) const noexcept { return Div(other); }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<F32x4, T>(), typename... Args>
    forceinline typename CastTyper<F32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
    template<RoundMode Mode = RoundMode::ToEven>
    forceinline F32x4 VECCALL Round() const noexcept
    {
#if COMMON_SIMD_LV >= 100
        switch (Mode)
        {
        case RoundMode::ToEven:     return vrndnq_f32(Data);
        case RoundMode::ToZero:     return vrndq_f32(Data);
        case RoundMode::ToPosInf:   return vrndpq_f32(Data);
        case RoundMode::ToNegInf:   return vrndmq_f32(Data);
        default:                    return vrndiq_f32(Data);
        }
#else
        switch (Mode)
        {
        case RoundMode::ToPosInf:   return { std::ceil(Val[0]),  std::ceil(Val[1]),  std::ceil(Val[2]),  std::ceil(Val[3])  };
        case RoundMode::ToNegInf:   return { std::floor(Val[0]), std::floor(Val[1]), std::floor(Val[2]), std::floor(Val[3]) };
        case RoundMode::ToZero:     return 
            { 
                Val[0] > 0 ? std::floor(Val[0]) : std::ceil(Val[0]), Val[1] > 0 ? std::floor(Val[1]) : std::ceil(Val[1]),
                Val[2] > 0 ? std::floor(Val[2]) : std::ceil(Val[2]), Val[3] > 0 ? std::floor(Val[3]) : std::ceil(Val[3])
            };
        case RoundMode::ToEven:
        default:
            std::fesetround(FE_TONEAREST);
            return { std::nearbyint(Val[0]), std::nearbyint(Val[1]), std::nearbyint(Val[2]), std::nearbyint(Val[3]) };
        }
#endif
    }
};


struct alignas(16) I64x2 : public detail::Common64x2<I64x2, int64x2_t, int64_t>
{
    using Common64x2<I64x2, int64x2_t, int64_t>::Common64x2;
    forceinline explicit I64x2(const int64_t* ptr) noexcept : Common64x2(vld1q_s64(ptr)) { }
    forceinline I64x2(const int64_t val) noexcept : Common64x2(vdupq_n_s64(val)) { }
    forceinline I64x2(const int64_t lo, const int64_t hi) noexcept : Common64x2(vcombine_s64(vdup_n_s64(lo), vdup_n_s64(hi))) { }
    forceinline void VECCALL Load(const int64_t* ptr) noexcept { Data = vld1q_s64(ptr); }
    forceinline void VECCALL Save(int64_t* ptr) const noexcept { vst1q_s64(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I64x2 VECCALL Compare(const I64x2 other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<int64x2_t>(vcltq_s64(Data, other.Data));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<int64x2_t>(vcleq_s64(Data, other.Data));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<int64x2_t>(vceqq_s64(Data, other.Data));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<int64x2_t>(vcgeq_s64(Data, other.Data));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<int64x2_t>(vcgtq_s64(Data, other.Data));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
#else
             if constexpr (Cmp == CompareType::LessEqual)    return Compare<CompareType::LessThan, Msk>(other).Or(Compare<CompareType::Equal, Msk>(other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return other.Compare<CompareType::LessEqual, Msk>(Data);
        else if constexpr (Cmp == CompareType::GreaterThan)  return other.Compare<CompareType::LessThan,  Msk>(Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else
        {
            if constexpr (Cmp == CompareType::Equal)
            {
                const auto u32eq = vceqq_u32(detail::AsType<uint32x4_t>(Data), detail::AsType<uint32x4_t>(other.Data));
                return vandq_u32(u32eq, vrev64q_u32(u32eq));
            }
            else if constexpr (Cmp == CompareType::LessThan)
            {
                const auto subbed = SatSub(other);
                if constexpr (Msk == MaskType::FullEle) return vshrq_n_s64(subbed, 64);
                else                                    return subbed;
            }        
            else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
        }
#endif
    }

    // arithmetic operations
    forceinline I64x2 VECCALL SatAdd(const I64x2& other) const noexcept { return vqaddq_s64(Data, other.Data); }
    forceinline I64x2 VECCALL SatSub(const I64x2& other) const noexcept { return vqsubq_s64(Data, other.Data); }
    forceinline I64x2 VECCALL Add(const I64x2& other) const noexcept { return vaddq_s64(Data, other.Data); }
    forceinline I64x2 VECCALL Sub(const I64x2& other) const noexcept { return vsubq_s64(Data, other.Data); }
    forceinline I64x2 VECCALL Neg() const noexcept
    {
#if COMMON_SIMD_LV >= 200
        return vnegq_s64(Data);
#else
        return AllZero().Sub(*this);
#endif
    }
    forceinline I64x2 VECCALL Abs() const noexcept
    { 
#if COMMON_SIMD_LV >= 200
        return vabsq_s64(Data);
#else
        return SelectWith<MaskType::SigBit>(Neg(), *this);
#endif
    }
    forceinline I64x2 VECCALL Max(const I64x2& other) const noexcept
    {
        const auto isGt = Compare<CompareType::GreaterThan, MaskType::FullEle>(other);
        return other.SelectWith<MaskType::FullEle>(Data, isGt);
        //return vbslq_s64(isGt, Data, other.Data);
    }
    forceinline I64x2 VECCALL Min(const I64x2& other) const noexcept
    {
        const auto isGt = Compare<CompareType::GreaterThan, MaskType::FullEle>(other);
        return SelectWith<MaskType::FullEle>(other, isGt);
        //return vbslq_s64(isGt, other.Data, Data);
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I64x2, T>(), typename... Args>
    forceinline typename CastTyper<I64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline F64x2 VECCALL I64x2::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return vcvtq_f64_s64(Data);
}
#endif


struct alignas(16) U64x2 : public detail::Common64x2<U64x2, uint64x2_t, uint64_t>
{
    using Common64x2<U64x2, uint64x2_t, uint64_t>::Common64x2;
    forceinline explicit U64x2(const uint64_t* ptr) noexcept : Common64x2(vld1q_u64(ptr)) { }
    forceinline U64x2(const uint64_t val) noexcept : Common64x2(vdupq_n_u64(val)) { }
    forceinline U64x2(const uint64_t lo, const int64_t hi) noexcept : Common64x2(vcombine_u64(vdup_n_u64(lo), vdup_n_u64(hi))) { }
    forceinline void VECCALL Load(const uint64_t* ptr) noexcept { Data = vld1q_u64(ptr); }
    forceinline void VECCALL Save(uint64_t* ptr) const noexcept { vst1q_u64(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U64x2 VECCALL Compare(const U64x2 other) const noexcept
    {
#if COMMON_SIMD_LV >= 200
             if constexpr (Cmp == CompareType::LessThan)     return vcltq_u64(Data, other.Data);
        else if constexpr (Cmp == CompareType::LessEqual)    return vcleq_u64(Data, other.Data);
        else if constexpr (Cmp == CompareType::Equal)        return vceqq_u64(Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterEqual) return vcgeq_u64(Data, other.Data);
        else if constexpr (Cmp == CompareType::GreaterThan)  return vcgtq_u64(Data, other.Data);
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
#else
        if constexpr (Cmp == CompareType::Equal || Cmp == CompareType::NotEqual)
        {
            return As<I64x2>().Compare<Cmp, Msk>(other.As<I64x2>()).template As<U64x2>();
        }
        else
        {
            const U64x2 sigMask(static_cast<uint64_t>(0x8000000000000000));
            return Xor(sigMask).As<I64x2>().Compare<Cmp, Msk>(other.Xor(sigMask).As<I64x2>()).template As<U64x2>();
        }
#endif
    }

    // arithmetic operations
    forceinline U64x2 VECCALL SatAdd(const U64x2& other) const noexcept { return vqaddq_u64(Data, other.Data); }
    forceinline U64x2 VECCALL SatSub(const U64x2& other) const noexcept { return vqsubq_u64(Data, other.Data); }
    forceinline U64x2 VECCALL Add(const U64x2& other) const noexcept { return vaddq_u64(Data, other.Data); }
    forceinline U64x2 VECCALL Sub(const U64x2& other) const noexcept { return vsubq_u64(Data, other.Data); }
    forceinline U64x2 VECCALL Abs() const noexcept { return Data; }
    forceinline U64x2 VECCALL Max(const U64x2& other) const noexcept
    {
        const auto isGt = Compare<CompareType::GreaterThan, MaskType::FullEle>(other);
        return other.SelectWith<MaskType::FullEle>(Data, isGt);
        //const auto isGt = vcgtq_u64(Data, other.Data);
        //return vbslq_f64(isGt, Data, other.Data);
    }
    forceinline U64x2 VECCALL Min(const U64x2& other) const noexcept
    {
        const auto isGt = Compare<CompareType::GreaterThan, MaskType::FullEle>(other);
        return SelectWith<MaskType::FullEle>(other, isGt);
        //const auto isGt = vcgtq_u64(Data, other.Data);
        //return vbslq_f64(isGt, other.Data, Data);
    }
    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U64x2, T>(), typename... Args>
    forceinline typename CastTyper<U64x2, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
#if COMMON_SIMD_LV >= 200
template<> forceinline F64x2 VECCALL U64x2::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return vcvtq_f64_u64(Data);
}
#endif


struct alignas(16) I32x4 : public detail::Common32x4<I32x4, int32x4_t, int32_t>
{
    using Common32x4<I32x4, int32x4_t, int32_t>::Common32x4;
    forceinline explicit I32x4(const int32_t* ptr) noexcept : Common32x4(vld1q_s32(ptr)) { }
    forceinline I32x4(const int32_t val) noexcept : Common32x4(vdupq_n_s32(val)) { }
    forceinline I32x4(const int32_t lo0, const int32_t lo1, const int32_t lo2, const int32_t hi3) noexcept : Common32x4()
    {
        alignas(16) int32_t tmp[] = { lo0, lo1, lo2, hi3 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const int32_t* ptr) noexcept { Data = vld1q_s32(ptr); }
    forceinline void VECCALL Save(int32_t* ptr) const noexcept { vst1q_s32(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I32x4 VECCALL Compare(const I32x4 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<int32x4_t>(vcltq_s32(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<int32x4_t>(vcleq_s32(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<int32x4_t>(vceqq_s32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<int32x4_t>(vcgeq_s32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<int32x4_t>(vcgtq_s32(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline I32x4 VECCALL SatAdd(const I32x4& other) const noexcept { return vqaddq_s32(Data, other.Data); }
    forceinline I32x4 VECCALL SatSub(const I32x4& other) const noexcept { return vqsubq_s32(Data, other.Data); }
    forceinline I32x4 VECCALL Add(const I32x4& other) const noexcept { return vaddq_s32(Data, other.Data); }
    forceinline I32x4 VECCALL Sub(const I32x4& other) const noexcept { return vsubq_s32(Data, other.Data); }
    template<bool Enforce = false>
    forceinline I32x4 VECCALL MulLo(const I32x4& other) const noexcept { return vmulq_s32(Data, other.Data); }
    //forceinline I32x4 VECCALL Div(const I32x4& other) const { return vdivq_s32(Data, other.Data); }
    forceinline I32x4 VECCALL Neg() const noexcept { return vnegq_s32(Data); }
    forceinline I32x4 VECCALL Abs() const noexcept { return vabsq_s32(Data); }
    forceinline I32x4 VECCALL Max(const I32x4& other) const noexcept { return vmaxq_s32(Data, other.Data); }
    forceinline I32x4 VECCALL Min(const I32x4& other) const noexcept { return vminq_s32(Data, other.Data); }
    forceinline Pack<I64x2, 2> VECCALL MulX(const I32x4& other) const noexcept
    {
        const auto lo = vmull_s32(vget_low_s32(Data), vget_low_s32(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_s32(Data, other.Data);
#else
        const auto hi = vmull_s32(vget_high_s32(Data), vget_high_s32(other.Data));
#endif
        return { lo, hi };
    }
    forceinline I32x4 VECCALL MulAddLo(const I32x4& muler, const I32x4& adder) const noexcept
    {
        return vmlaq_s32(adder.Data, Data, muler.Data);
    }
    forceinline I32x4 VECCALL MulScalarAddLo(const int32_t muler, const I32x4& adder) const noexcept
    {
        return vmlaq_n_s32(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline I32x4 VECCALL MulScalarAddLo(const I32x4& muler, const I32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        int32x4_t ret = adder.Data;
        __asm__("mla %0.4s, %1.4s, %2.s[%c3]\n\t" : "+w"(ret) : "w"(Data), "w"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlaq_laneq_s32(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 2)
            return vmlaq_lane_s32(adder.Data, Data, vget_low_s32(muler.Data), Idx);
        else
            return vmlaq_lane_s32(adder.Data, Data, vget_high_s32(muler.Data), Idx - 2);
#endif
    }
    forceinline I32x4 VECCALL MulSubLo(const I32x4& muler, const I32x4& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline I32x4 VECCALL MulScalarSubLo(const int32_t muler, const I32x4& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline I32x4 VECCALL MulScalarSubLo(const I32x4& muler, const I32x4& subber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline I32x4 VECCALL NMulAddLo(const I32x4& muler, const I32x4& adder) const noexcept
    {
        return vmlsq_s32(adder.Data, Data, muler.Data);
    }
    forceinline I32x4 VECCALL NMulScalarAddLo(const int32_t muler, const I32x4& adder) const noexcept
    {
        return vmlsq_n_s32(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline I32x4 VECCALL NMulScalarAddLo(const I32x4& muler, const I32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        int32x4_t ret = adder.Data;
        __asm__("mls %0.4s, %1.4s, %2.s[%c3]\n\t" : "+w"(ret) : "w"(Data), "w"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlsq_laneq_s32(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 2)
            return vmlsq_lane_s32(adder.Data, Data, vget_low_s32(muler.Data), Idx);
        else
            return vmlsq_lane_s32(adder.Data, Data, vget_high_s32(muler.Data), Idx - 2);
#endif
    }
    forceinline I32x4 VECCALL NMulSubLo(const I32x4& muler, const I32x4& subber) const noexcept
    {
        return MulAddLo(muler, subber).Neg();
    }
    forceinline I32x4 VECCALL NMulScalarSubLo(const int32_t muler, const I32x4& subber) const noexcept
    {
        return MulScalarAddLo(muler, subber).Neg();
    }
    template<size_t Idx>
    forceinline I32x4 VECCALL NMulScalarSubLo(const I32x4& muler, const I32x4& subber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return NMulSubLo(muler.Broadcast<Idx>(), subber);
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I32x4, T>(), typename... Args>
    forceinline typename CastTyper<I32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I64x2, 2> VECCALL I32x4::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return { vmovl_s32(vget_low_s32(Data)), vmovl_s32(vget_high_s32(Data)) };
}
template<> forceinline Pack<U64x2, 2> VECCALL I32x4::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline F32x4 VECCALL I32x4::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return vcvtq_f32_s32(Data);
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 2> VECCALL I32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().Cast<F64x2>();
}
#endif


struct alignas(16) U32x4 : public detail::Common32x4<U32x4, uint32x4_t, uint32_t>
{
    using Common32x4<U32x4, uint32x4_t, uint32_t>::Common32x4;
    forceinline explicit U32x4(const uint32_t* ptr) noexcept : Common32x4(vld1q_u32(ptr)) { }
    forceinline U32x4(const uint32_t val) noexcept : Common32x4(vdupq_n_u32(val)) { }
    forceinline U32x4(const uint32_t lo0, const uint32_t lo1, const uint32_t lo2, const uint32_t hi3) noexcept : Common32x4()
    {
        alignas(16) uint32_t tmp[] = { lo0, lo1, lo2, hi3 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const uint32_t* ptr) noexcept { Data = vld1q_u32(ptr); }
    forceinline void VECCALL Save(uint32_t* ptr) const noexcept { vst1q_u32(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U32x4 VECCALL Compare(const U32x4 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<uint32x4_t>(vcltq_u32(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<uint32x4_t>(vcleq_u32(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<uint32x4_t>(vceqq_u32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<uint32x4_t>(vcgeq_u32(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<uint32x4_t>(vcgtq_u32(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline U32x4 VECCALL SatAdd(const U32x4& other) const noexcept { return vqaddq_u32(Data, other.Data); }
    forceinline U32x4 VECCALL SatSub(const U32x4& other) const noexcept { return vqsubq_u32(Data, other.Data); }
    forceinline U32x4 VECCALL Add(const U32x4& other) const noexcept { return vaddq_u32(Data, other.Data); }
    forceinline U32x4 VECCALL Sub(const U32x4& other) const noexcept { return vsubq_u32(Data, other.Data); }
    template<bool Enforce = false>
    forceinline U32x4 VECCALL MulLo(const U32x4& other) const noexcept { return vmulq_u32(Data, other.Data); }
    //forceinline U32x4 VECCALL Div(const U32x4& other) const noexcept { return vdivq_s32(Data, other.Data); }
    forceinline U32x4 VECCALL Abs() const noexcept { return Data; }
    forceinline U32x4 VECCALL Max(const U32x4& other) const noexcept { return vmaxq_u32(Data, other.Data); }
    forceinline U32x4 VECCALL Min(const U32x4& other) const noexcept { return vminq_u32(Data, other.Data); }
    forceinline Pack<U64x2, 2> VECCALL MulX(const U32x4& other) const noexcept
    {
        const auto lo = vmull_u32(vget_low_u32(Data), vget_low_u32(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_u32(Data, other.Data);
#else
        const auto hi = vmull_u32(vget_high_u32(Data), vget_high_u32(other.Data));
#endif
        return { lo, hi };
    }
    forceinline U32x4 VECCALL MulAddLo(const U32x4& muler, const U32x4& adder) const noexcept
    {
        return vmlaq_u32(adder.Data, Data, muler.Data);
    }
    forceinline U32x4 VECCALL MulScalarAddLo(const uint32_t muler, const U32x4& adder) const noexcept
    {
        return vmlaq_n_u32(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline U32x4 VECCALL MulScalarAddLo(const U32x4& muler, const U32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        uint32x4_t ret = adder.Data;
        __asm__("mla %0.4s, %1.4s, %2.s[%c3]\n\t" : "+w"(ret) : "w"(Data), "w"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlaq_laneq_u32(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 2)
            return vmlaq_lane_u32(adder.Data, Data, vget_low_u32(muler.Data), Idx);
        else
            return vmlaq_lane_u32(adder.Data, Data, vget_high_u32(muler.Data), Idx - 2);
#endif
    }
    forceinline U32x4 VECCALL MulSubLo(const U32x4& muler, const U32x4& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline U32x4 VECCALL MulScalarSubLo(const uint32_t muler, const U32x4& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline U32x4 VECCALL MulScalarSubLo(const U32x4& muler, const U32x4& subber) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline U32x4 VECCALL NMulAddLo(const U32x4& muler, const U32x4& adder) const noexcept
    {
        return vmlsq_u32(adder.Data, Data, muler.Data);
    }
    forceinline U32x4 VECCALL NMulScalarAddLo(const uint32_t muler, const U32x4& adder) const noexcept
    {
        return vmlsq_n_u32(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline U32x4 VECCALL NMulScalarAddLo(const U32x4& muler, const U32x4& adder) const noexcept
    {
        static_assert(Idx < 4, "select index should be in [0,3]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        uint32x4_t ret = adder.Data;
        __asm__("mls %0.4s, %1.4s, %2.s[%c3]\n\t" : "+w"(ret) : "w"(Data), "w"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlsq_laneq_u32(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 2)
            return vmlsq_lane_u32(adder.Data, Data, vget_low_u32(muler.Data), Idx);
        else
            return vmlsq_lane_u32(adder.Data, Data, vget_high_u32(muler.Data), Idx - 2);
#endif
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U32x4, T>(), typename... Args>
    forceinline typename CastTyper<U32x4, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<U64x2, 2> VECCALL U32x4::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    const auto zero = vdupq_n_u32(0);
    return { vzip1q_u32(Data, zero), vzip2q_u32(Data, zero) };
#else
    return { vmovl_u32(vget_low_u32(Data)), vmovl_u32(vget_high_u32(Data)) };
#endif
}
template<> forceinline Pack<I64x2, 2> VECCALL U32x4::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().As<I64x2>();
}
template<> forceinline F32x4 VECCALL U32x4::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return vcvtq_f32_u32(Data);
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 2> VECCALL U32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().Cast<F64x2>();
}
#endif


struct alignas(16) I16x8 : public detail::Common16x8<I16x8, int16x8_t, int16_t>
{
    using Common16x8<I16x8, int16x8_t, int16_t>::Common16x8;
    forceinline explicit I16x8(const int16_t* ptr) noexcept : Common16x8(vld1q_s16(ptr)) { }
    forceinline I16x8(const int16_t val) noexcept : Common16x8(vdupq_n_s16(val)) { }
    forceinline I16x8(const int16_t lo0, const int16_t lo1, const int16_t lo2, const int16_t lo3, 
        const int16_t lo4, const int16_t lo5, const int16_t lo6, const int16_t hi7) noexcept : Common16x8()
    {
        alignas(16) int16_t tmp[] = { lo0, lo1, lo2, lo3, lo4, lo5, lo6, hi7 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const int16_t* ptr) noexcept { Data = vld1q_s16(ptr); }
    forceinline void VECCALL Save(int16_t* ptr) const noexcept { vst1q_s16(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I16x8 VECCALL Compare(const I16x8 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<int16x8_t>(vcltq_s16(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<int16x8_t>(vcleq_s16(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<int16x8_t>(vceqq_s16(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<int16x8_t>(vcgeq_s16(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<int16x8_t>(vcgtq_s16(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline I16x8 VECCALL SatAdd(const I16x8& other) const noexcept { return vqaddq_s16(Data, other.Data); }
    forceinline I16x8 VECCALL SatSub(const I16x8& other) const noexcept { return vqsubq_s16(Data, other.Data); }
    forceinline I16x8 VECCALL Add(const I16x8& other) const noexcept { return vaddq_s16(Data, other.Data); }
    forceinline I16x8 VECCALL Sub(const I16x8& other) const noexcept { return vsubq_s16(Data, other.Data); }
    template<bool Enforce = false>
    forceinline I16x8 VECCALL MulLo(const I16x8& other) const noexcept { return vmulq_s16(Data, other.Data); }
    //forceinline I16x8 VECCALL Div(const I16x8& other) const noexcept { return vdivq_s16(Data, other.Data); }
    forceinline I16x8 VECCALL Neg() const noexcept { return vnegq_s16(Data); }
    forceinline I16x8 VECCALL Abs() const noexcept { return vabsq_s16(Data); }
    forceinline I16x8 VECCALL Max(const I16x8& other) const noexcept { return vmaxq_s16(Data, other.Data); }
    forceinline I16x8 VECCALL Min(const I16x8& other) const noexcept { return vminq_s16(Data, other.Data); }
    forceinline Pack<I32x4, 2> VECCALL MulX(const I16x8& other) const noexcept
    {
        const auto lo = vmull_s16(vget_low_s16(Data), vget_low_s16(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_s16(Data, other.Data);
#else
        const auto hi = vmull_s16(vget_high_s16(Data), vget_high_s16(other.Data));
#endif
        return { lo, hi };
    }
    forceinline I16x8 VECCALL MulAddLo(const I16x8& muler, const I16x8& adder) const noexcept
    {
        return vmlaq_s16(adder.Data, Data, muler.Data);
    }
    forceinline I16x8 VECCALL MulScalarAddLo(const int16_t muler, const I16x8& adder) const noexcept
    {
        return vmlaq_n_s16(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline I16x8 VECCALL MulScalarAddLo(const I16x8& muler, const I16x8& adder) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        int16x8_t ret = adder.Data;
        __asm__("mla %0.8h, %1.8h, %2.h[%c3]\n\t" : "+w"(ret) : "w"(Data), "x"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlaq_laneq_s16(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 4)
            return vmlaq_lane_s16(adder.Data, Data, vget_low_s16(muler.Data), Idx);
        else
            return vmlaq_lane_s16(adder.Data, Data, vget_high_s16(muler.Data), Idx - 4);
#endif
    }
    forceinline I16x8 VECCALL MulSubLo(const I16x8& muler, const I16x8& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline I16x8 VECCALL MulScalarSubLo(const int16_t muler, const I16x8& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline I16x8 VECCALL MulScalarSubLo(const I16x8& muler, const I16x8& subber) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline I16x8 VECCALL NMulAddLo(const I16x8& muler, const I16x8& adder) const noexcept
    {
        return vmlsq_s16(adder.Data, Data, muler.Data);
    }
    forceinline I16x8 VECCALL NMulScalarAddLo(const int16_t muler, const I16x8& adder) const noexcept
    {
        return vmlsq_n_s16(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline I16x8 VECCALL NMulScalarAddLo(const I16x8& muler, const I16x8& adder) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        int16x8_t ret = adder.Data;
        __asm__("mls %0.8h, %1.8h, %2.h[%c3]\n\t" : "+w"(ret) : "w"(Data), "x"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlsq_laneq_s16(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 4)
            return vmlsq_lane_s16(adder.Data, Data, vget_low_s16(muler.Data), Idx);
        else
            return vmlsq_lane_s16(adder.Data, Data, vget_high_s16(muler.Data), Idx - 4);
#endif
    }
    forceinline I16x8 VECCALL NMulSubLo(const I16x8& muler, const I16x8& subber) const noexcept
    {
        return MulAddLo(muler, subber).Neg();
    }
    forceinline I16x8 VECCALL NMulScalarSubLo(const int16_t muler, const I16x8& subber) const noexcept
    {
        return MulScalarAddLo(muler, subber).Neg();
    }
    template<size_t Idx>
    forceinline I16x8 VECCALL NMulScalarSubLo(const I16x8& muler, const I16x8& subber) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
        return NMulSubLo(muler.Broadcast<Idx>(), subber);
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I16x8, T>(), typename... Args>
    forceinline typename CastTyper<I16x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<I32x4, 2> VECCALL I16x8::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return { vmovl_s16(vget_low_s16(Data)), vmovl_s16(vget_high_s16(Data)) };
}
template<> forceinline Pack<U32x4, 2> VECCALL I16x8::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 4> VECCALL I16x8::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<I64x2>();
}
template<> forceinline Pack<U64x2, 4> VECCALL I16x8::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL I16x8::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 4> VECCALL I16x8::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().Cast<F64x2>();
}
#endif


struct alignas(16) U16x8 : public detail::Common16x8<U16x8, uint16x8_t, uint16_t>
{
    using Common16x8<U16x8, uint16x8_t, uint16_t>::Common16x8;
    forceinline explicit U16x8(const uint16_t* ptr) noexcept : Common16x8(vld1q_u16(ptr)) { }
    forceinline U16x8(const uint16_t val) noexcept : Common16x8(vdupq_n_u16(val)) { }
    forceinline U16x8(const uint16_t lo0, const uint16_t lo1, const uint16_t lo2, const uint16_t lo3,
        const uint16_t lo4, const uint16_t lo5, const uint16_t lo6, const uint16_t hi7) noexcept : Common16x8()
    {
        alignas(16) uint16_t tmp[] = { lo0, lo1, lo2, lo3, lo4, lo5, lo6, hi7 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const uint16_t* ptr) noexcept { Data = vld1q_u16(ptr); }
    forceinline void VECCALL Save(uint16_t* ptr) const noexcept { vst1q_u16(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U16x8 VECCALL Compare(const U16x8 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<uint16x8_t>(vcltq_u16(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<uint16x8_t>(vcleq_u16(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<uint16x8_t>(vceqq_u16(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<uint16x8_t>(vcgeq_u16(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<uint16x8_t>(vcgtq_u16(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline U16x8 VECCALL SatAdd(const U16x8& other) const noexcept { return vqaddq_u16(Data, other.Data); }
    forceinline U16x8 VECCALL SatSub(const U16x8& other) const noexcept { return vqsubq_u16(Data, other.Data); }
    forceinline U16x8 VECCALL Add(const U16x8& other) const noexcept { return vaddq_u16(Data, other.Data); }
    forceinline U16x8 VECCALL Sub(const U16x8& other) const noexcept { return vsubq_u16(Data, other.Data); }
    template<bool Enforce = false>
    forceinline U16x8 VECCALL MulLo(const U16x8& other) const noexcept { return vmulq_u16(Data, other.Data); }
    //forceinline U16x8 VECCALL Div(const U16x8& other) const noexcept { return vdivq_u16(Data, other.Data); }
    forceinline U16x8 VECCALL Abs() const noexcept { return Data; }
    forceinline U16x8 VECCALL Max(const U16x8& other) const noexcept { return vmaxq_u16(Data, other.Data); }
    forceinline U16x8 VECCALL Min(const U16x8& other) const noexcept { return vminq_u16(Data, other.Data); }
    forceinline Pack<U32x4, 2> VECCALL MulX(const U16x8& other) const noexcept
    {
        const auto lo = vmull_u16(vget_low_u16(Data), vget_low_u16(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_u16(Data, other.Data);
#else
        const auto hi = vmull_u16(vget_high_u16(Data), vget_high_u16(other.Data));
#endif
        return { lo, hi };
    }
    forceinline U16x8 VECCALL MulAddLo(const U16x8& muler, const U16x8& adder) const noexcept
    {
        return vmlaq_u16(adder.Data, Data, muler.Data);
    }
    forceinline U16x8 VECCALL MulScalarAddLo(const uint16_t muler, const U16x8& adder) const noexcept
    {
        return vmlaq_n_u16(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline U16x8 VECCALL MulScalarAddLo(const U16x8& muler, const U16x8& adder) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        uint16x8_t ret = adder.Data;
        __asm__("mla %0.8h, %1.8h, %2.h[%c3]\n\t" : "+w"(ret) : "w"(Data), "x"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlaq_laneq_u16(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 4)
            return vmlaq_lane_u16(adder.Data, Data, vget_low_u16(muler.Data), Idx);
        else
            return vmlaq_lane_u16(adder.Data, Data, vget_high_u16(muler.Data), Idx - 4);
#endif
    }
    forceinline U16x8 VECCALL MulSubLo(const U16x8& muler, const U16x8& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline U16x8 VECCALL MulScalarSubLo(const uint16_t muler, const U16x8& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline U16x8 VECCALL MulScalarSubLo(const U16x8& muler, const U16x8& subber) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline U16x8 VECCALL NMulAddLo(const U16x8& muler, const U16x8& adder) const noexcept
    {
        return vmlsq_u16(adder.Data, Data, muler.Data);
    }
    forceinline U16x8 VECCALL NMulScalarAddLo(const uint16_t muler, const U16x8& adder) const noexcept
    {
        return vmlsq_n_u16(adder.Data, Data, muler);
    }
    template<size_t Idx>
    forceinline U16x8 VECCALL NMulScalarAddLo(const U16x8& muler, const U16x8& adder) const noexcept
    {
        static_assert(Idx < 7, "select index should be in [0,7]");
#if COMMON_SIMD_LV >= 200
# if COMMON_COMPILER_CLANG
        uint16x8_t ret = adder.Data;
        __asm__("mls %0.8h, %1.8h, %2.h[%c3]\n\t" : "+w"(ret) : "w"(Data), "x"(muler.Data), "n"(Idx));
        return ret;
# else
        return vmlsq_laneq_u16(adder.Data, Data, muler.Data, Idx);
# endif
#else
        if constexpr (Idx < 4)
            return vmlsq_lane_u16(adder.Data, Data, vget_low_u16(muler.Data), Idx);
        else
            return vmlsq_lane_u16(adder.Data, Data, vget_high_u16(muler.Data), Idx - 4);
#endif
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U16x8, T>(), typename... Args>
    forceinline typename CastTyper<U16x8, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<U32x4, 2> VECCALL U16x8::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    const auto zero = vdupq_n_u16(0);
    return { vzip1q_u16(Data, zero), vzip2q_u16(Data, zero) };
#else
    return { vmovl_u16(vget_low_u16(Data)), vmovl_u16(vget_high_u16(Data)) };
#endif
}
template<> forceinline Pack<I32x4, 2> VECCALL U16x8::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<U32x4>().As<I32x4>();
}
template<> forceinline Pack<U64x2, 4> VECCALL U16x8::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    constexpr uint64_t mask4x4[] =
    {
        0xffffffffffff0100, 0xffffffffffff0302,
        0xffffffffffff0504, 0xffffffffffff0706,
        0xffffffffffff0908, 0xffffffffffff0b0a,
        0xffffffffffff0d0c, 0xffffffffffff0f0e,
    };
    const auto masks = vld1q_u64_x4(mask4x4);
    return
    {
        vreinterpretq_u64_u8(vqtbl1q_u8(Data, vreinterpretq_u8_u64(masks.val[0]))),
        vreinterpretq_u64_u8(vqtbl1q_u8(Data, vreinterpretq_u8_u64(masks.val[1]))),
        vreinterpretq_u64_u8(vqtbl1q_u8(Data, vreinterpretq_u8_u64(masks.val[2]))),
        vreinterpretq_u64_u8(vqtbl1q_u8(Data, vreinterpretq_u8_u64(masks.val[3])))
    };
#else
    return Cast<U32x4>().Cast<U64x2>();
    /*const auto mid = Cast<U32x4>();
    const auto lo = mid[0].Cast<I64x2>(), hi = mid[0].Cast<I64x2>();
    return { lo[0], lo[1], hi[0], hi[1] };*/
#endif
}
template<> forceinline Pack<I64x2, 4> VECCALL U16x8::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().As<I64x2>();
}
template<> forceinline Pack<F32x4, 2> VECCALL U16x8::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 4> VECCALL U16x8::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().Cast<F64x2>();
}
#endif


struct alignas(16) I8x16 : public detail::Common8x16<I8x16, int8x16_t, int8_t>
{
    using Common8x16<I8x16, int8x16_t, int8_t>::Common8x16;
    forceinline explicit I8x16(const int8_t* ptr) noexcept : Common8x16(vld1q_s8(ptr)) { }
    forceinline I8x16(const int8_t val) noexcept : Common8x16(vdupq_n_s8(val)) { }
    forceinline I8x16(const int8_t lo0, const int8_t lo1, const int8_t lo2, const int8_t lo3,
        const int8_t lo4, const int8_t lo5, const int8_t lo6, const int8_t lo7,
        const int8_t lo8, const int8_t lo9, const int8_t lo10, const int8_t lo11,
        const int8_t lo12, const int8_t lo13, const int8_t lo14, const int8_t hi15) noexcept : Common8x16()
    {
        alignas(16) int8_t tmp[] = { lo0, lo1, lo2, lo3, lo4, lo5, lo6, lo7, lo8, lo9, lo10, lo11, lo12, lo13, lo14, hi15 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const int8_t* ptr) noexcept { Data = vld1q_s8(ptr); }
    forceinline void VECCALL Save(int8_t* ptr) const noexcept { vst1q_s8(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline I8x16 VECCALL Compare(const I8x16 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<int8x16_t>(vcltq_s8(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<int8x16_t>(vcleq_s8(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<int8x16_t>(vceqq_s8(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<int8x16_t>(vcgeq_s8(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<int8x16_t>(vcgtq_s8(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline I8x16 VECCALL SatAdd(const I8x16& other) const noexcept { return vqaddq_s8(Data, other.Data); }
    forceinline I8x16 VECCALL SatSub(const I8x16& other) const noexcept { return vqsubq_s8(Data, other.Data); }
    forceinline I8x16 VECCALL Add(const I8x16& other) const noexcept { return vaddq_s8(Data, other.Data); }
    forceinline I8x16 VECCALL Sub(const I8x16& other) const noexcept { return vsubq_s8(Data, other.Data); }
    template<bool Enforce = false>
    forceinline I8x16 VECCALL MulLo(const I8x16& other) const noexcept { return vmulq_s8(Data, other.Data); }
    //forceinline I8x16 VECCALL Div(const I8x16& other) const noexcept { return vdivq_s8(Data, other.Data); }
    forceinline I8x16 VECCALL Neg() const noexcept { return vnegq_s8(Data); }
    forceinline I8x16 VECCALL Abs() const noexcept { return vabsq_s8(Data); }
    forceinline I8x16 VECCALL Max(const I8x16& other) const noexcept { return vmaxq_s8(Data, other.Data); }
    forceinline I8x16 VECCALL Min(const I8x16& other) const noexcept { return vminq_s8(Data, other.Data); }
    forceinline Pack<I16x8, 2> VECCALL MulX(const I8x16& other) const noexcept
    {
        const auto lo = vmull_s8(vget_low_s8(Data), vget_low_s8(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_s8(Data, other.Data);
#else
        const auto hi = vmull_s8(vget_high_s8(Data), vget_high_s8(other.Data));
#endif
        return { lo, hi };
    }
    forceinline I8x16 VECCALL MulAddLo(const I8x16& muler, const I8x16& adder) const noexcept
    {
        return vmlaq_s8(adder.Data, Data, muler.Data);
    }
    forceinline I8x16 VECCALL MulScalarAddLo(const int8_t muler, const I8x16& adder) const noexcept
    {
        return MulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline I8x16 VECCALL MulScalarAddLo(const I8x16& muler, const I8x16& adder) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return MulAddLo(muler.Broadcast<Idx>(), adder);
    }
    forceinline I8x16 VECCALL MulSubLo(const I8x16& muler, const I8x16& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline I8x16 VECCALL MulScalarSubLo(const int8_t muler, const I8x16& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline I8x16 VECCALL MulScalarSubLo(const I8x16& muler, const I8x16& subber) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline I8x16 VECCALL NMulAddLo(const I8x16& muler, const I8x16& adder) const noexcept
    {
        return vmlsq_s8(adder.Data, Data, muler.Data);
    }
    forceinline I8x16 VECCALL NMulScalarAddLo(const int8_t muler, const I8x16& adder) const noexcept
    {
        return NMulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline I8x16 VECCALL NMulScalarAddLo(const I8x16& muler, const I8x16& adder) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return NMulAddLo(muler.Broadcast<Idx>(), adder);
    }
    forceinline I8x16 VECCALL NMulSubLo(const I8x16& muler, const I8x16& subber) const noexcept
    {
        return MulAddLo(muler, subber).Neg();
    }
    forceinline I8x16 VECCALL NMulScalarSubLo(const int8_t muler, const I8x16& subber) const noexcept
    {
        return MulScalarAddLo(muler, subber).Neg();
    }
    template<size_t Idx>
    forceinline I8x16 VECCALL NMulScalarSubLo(const I8x16& muler, const I8x16& subber) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return NMulSubLo(muler.Broadcast<Idx>(), subber);
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<I8x16, T>(), typename... Args>
    forceinline typename CastTyper<I8x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;

};
template<> forceinline Pack<I16x8, 2> VECCALL I8x16::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    return { vmovl_s8(vget_low_s8(Data)), vmovl_s8(vget_high_s8(Data)) };
}
template<> forceinline Pack<U16x8, 2> VECCALL I8x16::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x8>().As<U16x8>();
}
template<> forceinline Pack<I32x4, 4> VECCALL I8x16::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x8>().Cast<I32x4>();
}
template<> forceinline Pack<U32x4, 4> VECCALL I8x16::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().As<U32x4>();
}
template<> forceinline Pack<I64x2, 8> VECCALL I8x16::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I16x8>().Cast<I32x4>().Cast<I64x2>();
}
template<> forceinline Pack<U64x2, 8> VECCALL I8x16::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().As<U64x2>();
}
template<> forceinline Pack<F32x4, 4> VECCALL I8x16::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 8> VECCALL I8x16::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<I64x2>().Cast<F64x2>();
}
#endif


struct alignas(16) U8x16 : public detail::Common8x16<U8x16, uint8x16_t, uint8_t>
{
    using Common8x16<U8x16, uint8x16_t, uint8_t>::Common8x16;
    forceinline explicit U8x16(const uint8_t* ptr) noexcept : Common8x16(vld1q_u8(ptr)) { }
    forceinline U8x16(const uint8_t val) noexcept : Common8x16(vdupq_n_u8(val)) { }
    forceinline U8x16(const uint8_t lo0, const uint8_t lo1, const uint8_t lo2, const uint8_t lo3,
        const uint8_t lo4, const uint8_t lo5, const uint8_t lo6, const uint8_t lo7,
        const uint8_t lo8, const uint8_t lo9, const uint8_t lo10, const uint8_t lo11,
        const uint8_t lo12, const uint8_t lo13, const uint8_t lo14, const uint8_t hi15) noexcept : Common8x16()
    {
        alignas(16) uint8_t tmp[] = { lo0, lo1, lo2, lo3, lo4, lo5, lo6, lo7, lo8, lo9, lo10, lo11, lo12, lo13, lo14, hi15 };
        Load(tmp);
    }
    forceinline void VECCALL Load(const uint8_t* ptr) noexcept { Data = vld1q_u8(ptr); }
    forceinline void VECCALL Save(uint8_t* ptr) const noexcept { vst1q_u8(ptr, Data); }

    // compare operations
    template<CompareType Cmp, MaskType Msk>
    forceinline U8x16 VECCALL Compare(const U8x16 other) const noexcept
    {
             if constexpr (Cmp == CompareType::LessThan)     return detail::AsType<uint8x16_t>(vcltq_u8(Data, other));
        else if constexpr (Cmp == CompareType::LessEqual)    return detail::AsType<uint8x16_t>(vcleq_u8(Data, other));
        else if constexpr (Cmp == CompareType::Equal)        return detail::AsType<uint8x16_t>(vceqq_u8(Data, other));
        else if constexpr (Cmp == CompareType::GreaterEqual) return detail::AsType<uint8x16_t>(vcgeq_u8(Data, other));
        else if constexpr (Cmp == CompareType::GreaterThan)  return detail::AsType<uint8x16_t>(vcgtq_u8(Data, other));
        else if constexpr (Cmp == CompareType::NotEqual)     return Compare<CompareType::Equal, Msk>(other).Not();
        else static_assert(!::common::AlwaysTrue2<Cmp>, "unrecognized compare");
    }

    // arithmetic operations
    forceinline U8x16 VECCALL SatAdd(const U8x16& other) const noexcept { return vqaddq_u8(Data, other.Data); }
    forceinline U8x16 VECCALL SatSub(const U8x16& other) const noexcept { return vqsubq_u8(Data, other.Data); }
    forceinline U8x16 VECCALL Add(const U8x16& other) const noexcept { return vaddq_u8(Data, other.Data); }
    forceinline U8x16 VECCALL Sub(const U8x16& other) const noexcept { return vsubq_u8(Data, other.Data); }
    template<bool Enforce = false>
    forceinline U8x16 VECCALL MulLo(const U8x16& other) const noexcept { return vmulq_u8(Data, other.Data); }
    //forceinline U8x16 VECCALL Div(const U8x16& other) const noexcept { return vdivq_u8(Data, other.Data); }
    forceinline U8x16 VECCALL Abs() const noexcept { return Data; }
    forceinline U8x16 VECCALL Max(const U8x16& other) const noexcept { return vmaxq_u8(Data, other.Data); }
    forceinline U8x16 VECCALL Min(const U8x16& other) const noexcept { return vminq_u8(Data, other.Data); }
    forceinline Pack<U16x8, 2> VECCALL MulX(const U8x16& other) const noexcept
    {
        const auto lo = vmull_u8(vget_low_u8(Data), vget_low_u8(other.Data));
#if COMMON_SIMD_LV >= 200
        const auto hi = vmull_high_u8(Data, other.Data);
#else
        const auto hi = vmull_u8(vget_high_u8(Data), vget_high_u8(other.Data));
#endif
        return { lo, hi };
    }
    forceinline U8x16 VECCALL MulAddLo(const U8x16& muler, const U8x16& adder) const noexcept
    {
        return vmlaq_u8(adder.Data, Data, muler.Data);
    }
    forceinline U8x16 VECCALL MulScalarAddLo(const uint8_t muler, const U8x16& adder) const noexcept
    {
        return MulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline U8x16 VECCALL MulScalarAddLo(const U8x16& muler, const U8x16& adder) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return MulAddLo(muler.Broadcast<Idx>(), adder);
    }
    forceinline U8x16 VECCALL MulSubLo(const U8x16& muler, const U8x16& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    forceinline U8x16 VECCALL MulScalarSubLo(const uint8_t muler, const U8x16& subber) const noexcept
    {
        return MulLo(muler).Sub(subber);
    }
    template<size_t Idx>
    forceinline U8x16 VECCALL MulScalarSubLo(const U8x16& muler, const U8x16& subber) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return MulSubLo(muler.Broadcast<Idx>(), subber);
    }
    forceinline U8x16 VECCALL NMulAddLo(const U8x16& muler, const U8x16& adder) const noexcept
    {
        return vmlsq_u8(adder.Data, Data, muler.Data);
    }
    forceinline U8x16 VECCALL NMulScalarAddLo(const uint8_t muler, const U8x16& adder) const noexcept
    {
        return NMulAddLo(muler, adder);
    }
    template<size_t Idx>
    forceinline U8x16 VECCALL NMulScalarAddLo(const U8x16& muler, const U8x16& adder) const noexcept
    {
        static_assert(Idx < 15, "select index should be in [0,15]");
        return NMulAddLo(muler.Broadcast<Idx>(), adder);
    }

    template<typename T, CastMode Mode = ::common::simd::detail::CstMode<U8x16, T>(), typename... Args>
    forceinline typename CastTyper<U8x16, T>::Type VECCALL Cast(const Args&... args) const noexcept;
};
template<> forceinline Pack<U16x8, 2> VECCALL U8x16::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    const auto zero = vdupq_n_u8(0);
    return { vzip1q_u8(Data, zero), vzip2q_u8(Data, zero) };
#else
    return { vmovl_u8(vget_low_u8(Data)), vmovl_u8(vget_high_u8(Data)) };
#endif
}
template<> forceinline Pack<I16x8, 2> VECCALL U8x16::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    return Cast<U16x8>().As<I16x8>();
}
template<> forceinline Pack<U32x4, 4> VECCALL U8x16::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
#if COMMON_SIMD_LV >= 200
    constexpr uint32_t mask4x4[] =
    {
        0xffffff00, 0xffffff01, 0xffffff02, 0xffffff03,
        0xffffff04, 0xffffff05, 0xffffff06, 0xffffff07,
        0xffffff08, 0xffffff09, 0xffffff0a, 0xffffff0b,
        0xffffff0c, 0xffffff0d, 0xffffff0e, 0xffffff0f,
    };
    const auto masks = vld1q_u32_x4(mask4x4);
    return 
    { 
        vqtbl1q_u8(Data, vreinterpretq_u8_u32(masks.val[0])), 
        vqtbl1q_u8(Data, vreinterpretq_u8_u32(masks.val[1])), 
        vqtbl1q_u8(Data, vreinterpretq_u8_u32(masks.val[2])), 
        vqtbl1q_u8(Data, vreinterpretq_u8_u32(masks.val[3])) 
    };
#else
    return Cast<U16x8>().Cast<U32x4>();
#endif
}
template<> forceinline Pack<I32x4, 4> VECCALL U8x16::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<U32x4>().As<I32x4>();
}
template<> forceinline Pack<U64x2, 8> VECCALL U8x16::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U32x4>().Cast<U64x2>();
}
template<> forceinline Pack<I64x2, 8> VECCALL U8x16::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().As<I64x2>();
}
template<> forceinline Pack<F32x4, 4> VECCALL U8x16::Cast<F32x4, CastMode::RangeUndef>() const noexcept
{
    return Cast<I32x4>().Cast<F32x4>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 8> VECCALL U8x16::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return Cast<U64x2>().Cast<F64x2>();
}
#endif


template<> forceinline U64x2 VECCALL I64x2::Cast<U64x2, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_s64_u64(Data);
}
template<> forceinline I64x2 VECCALL U64x2::Cast<I64x2, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_u64_s64(Data);
}
template<> forceinline U32x4 VECCALL I32x4::Cast<U32x4, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_s32_u32(Data);
}
template<> forceinline I32x4 VECCALL U32x4::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_u32_s32(Data);
}
template<> forceinline U16x8 VECCALL I16x8::Cast<U16x8, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_s16_u16(Data);
}
template<> forceinline I16x8 VECCALL U16x8::Cast<I16x8, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_u16_s16(Data);
}
template<> forceinline U8x16 VECCALL I8x16::Cast<U8x16, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_s8_u8(Data);
}
template<> forceinline I8x16 VECCALL U8x16::Cast<I8x16, CastMode::RangeUndef>() const noexcept
{
    return vreinterpretq_u8_s8(Data);
}


template<> forceinline U32x4 VECCALL U64x2::Cast<U32x4, CastMode::RangeTrunc>(const U64x2& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vuzp1q_u32(vreinterpretq_u64_u32(Data), vreinterpretq_u64_u32(arg1));
#elif COMMON_SIMD_LV >= 100
    return vmovn_high_u64(vmovn_u64(Data), arg1);
#else
    return vcombine_u32(vmovn_u64(Data), vmovn_u64(arg1));
#endif
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeTrunc>(const U32x4& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vuzp1q_u16(vreinterpretq_u32_u16(Data), vreinterpretq_u32_u16(arg1));
#elif COMMON_SIMD_LV >= 100
    return vmovn_high_u32(vmovn_u32(Data), arg1);
#else
    return vcombine_u16(vmovn_u32(Data), vmovn_u32(arg1));
#endif
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeTrunc>(const U16x8& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vuzp1q_u8(vreinterpretq_u16_u8(Data), vreinterpretq_u16_u8(arg1));
#elif COMMON_SIMD_LV >= 100
    return vmovn_high_u16(vmovn_u16(Data), arg1);
#else

    return vcombine_u8(vmovn_u16(Data), vmovn_u16(arg1));
#endif
}
template<> forceinline U16x8 VECCALL U64x2::Cast<U16x8, CastMode::RangeTrunc>(const U64x2& arg1, const U64x2& arg2, const U64x2& arg3) const noexcept
{
    const auto lo16 = Cast<U32x4>(arg1), hi16 = arg2.Cast<U32x4>(arg3);
    return lo16.Cast<U16x8>(hi16);
}
template<> forceinline U8x16 VECCALL U32x4::Cast<U8x16, CastMode::RangeTrunc>(const U32x4& arg1, const U32x4& arg2, const U32x4& arg3) const noexcept
{
    const auto lo16 = Cast<U16x8>(arg1), hi16 = arg2.Cast<U16x8>(arg3);
    return lo16.Cast<U8x16>(hi16);
}
template<> forceinline U8x16 VECCALL U64x2::Cast<U8x16, CastMode::RangeTrunc>(const U64x2& arg1, const U64x2& arg2, const U64x2& arg3,
    const U64x2& arg4, const U64x2& arg5, const U64x2& arg6, const U64x2& arg7) const noexcept
{
    const auto lo8 = Cast<U16x8>(arg1, arg2, arg3), hi8 = arg4.Cast<U16x8>(arg5, arg6, arg7);
    return lo8.Cast<U8x16>(hi8);
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


template<typename T, typename SIMDType, typename E>
forceinline T VECCALL detail::Common8x16<T, SIMDType, E>::Shuffle(const U8x16& pos) const noexcept
{
    using V = typename T::VecType;
    const auto data = AsType<uint8x16_t>(static_cast<const T*>(this)->Data);
    return AsType<V>(vqtbl1q_u8(data, pos));
}


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeUndef>() const noexcept
{
    return vcvtq_s32_f32(Data);
}
template<> forceinline I16x8 VECCALL F32x4::Cast<I16x8, CastMode::RangeUndef>(const F32x4& arg1) const noexcept
{
    return Cast<I32x4>().Cast<I16x8>(arg1.Cast<I32x4>());
}
template<> forceinline U16x8 VECCALL F32x4::Cast<U16x8, CastMode::RangeUndef>(const F32x4& arg1) const noexcept
{
    return Cast<I16x8>(arg1).As<U16x8>();
}
template<> forceinline I8x16 VECCALL F32x4::Cast<I8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    return Cast<I32x4>().Cast<I8x16>(arg1.Cast<I32x4>(), arg2.Cast<I32x4>(), arg3.Cast<I32x4>());
}
template<> forceinline U8x16 VECCALL F32x4::Cast<U8x16, CastMode::RangeUndef>(const F32x4& arg1, const F32x4& arg2, const F32x4& arg3) const noexcept
{
    return Cast<I8x16>(arg1, arg2, arg3).As<U8x16>();
}
#if COMMON_SIMD_LV >= 200
template<> forceinline Pack<F64x2, 2> VECCALL F32x4::Cast<F64x2, CastMode::RangeUndef>() const noexcept
{
    return { vcvt_f64_f32(vget_low_f32(Data)), vcvt_high_f64_f32(Data) };
}
template<> forceinline F32x4 VECCALL F64x2::Cast<F32x4, CastMode::RangeUndef>(const F64x2& arg1) const noexcept
{
    return vcombine_f32(vcvt_f32_f64(Data), vcvt_f32_f64(arg1));
}
#endif


template<> forceinline I32x4 VECCALL F32x4::Cast<I32x4, CastMode::RangeSaturate>() const noexcept
{
    const F32x4 minVal = static_cast<float>(INT32_MIN), maxVal = static_cast<float>(INT32_MAX);
    const auto val = Cast<I32x4, CastMode::RangeUndef>();
    // INT32 loses precision, need maunally bit-select
    const auto isLe = Compare<CompareType::LessEqual,    MaskType::FullEle>(minVal).As<I32x4>();
    const auto isGe = Compare<CompareType::GreaterEqual, MaskType::FullEle>(maxVal).As<I32x4>();
    /*const auto isLe = vcleq_f32(Data, minVal);
    const auto isGe = vcgeq_f32(Data, maxVal);*/
    const auto satMin = vbslq_s32(isLe, I32x4(INT32_MIN), val);
    return vbslq_s32(isGe, I32x4(INT32_MAX), satMin);
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
template<> forceinline I32x4 VECCALL I64x2::Cast<I32x4, CastMode::RangeSaturate>(const I64x2& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_s64(vqmovn_s64(Data), arg1);
#else
    return vcombine_s32(vqmovn_s64(Data), vqmovn_s64(arg1));
#endif
}
template<> forceinline U32x4 VECCALL I64x2::Cast<U32x4, CastMode::RangeSaturate>(const I64x2& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovun_high_s64(vqmovun_s64(Data), arg1);
#else
    return vcombine_s32(vqmovun_s64(Data), vqmovun_s64(arg1));
#endif
}
template<> forceinline U32x4 VECCALL U64x2::Cast<U32x4, CastMode::RangeSaturate>(const U64x2& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_u64(vqmovn_u64(Data), arg1);
#else
    return vcombine_u32(vqmovn_u64(Data), vqmovn_u64(arg1));
#endif
}
template<> forceinline I16x8 VECCALL I32x4::Cast<I16x8, CastMode::RangeSaturate>(const I32x4& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_s32(vqmovn_s32(Data), arg1);
#else
    return vcombine_s16(vqmovn_s32(Data), vqmovn_s32(arg1));
#endif
}
template<> forceinline U16x8 VECCALL I32x4::Cast<U16x8, CastMode::RangeSaturate>(const I32x4& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovun_high_s32(vqmovun_s32(Data), arg1);
#else
    return vcombine_s16(vqmovun_s32(Data), vqmovun_s32(arg1));
#endif
}
template<> forceinline U16x8 VECCALL U32x4::Cast<U16x8, CastMode::RangeSaturate>(const U32x4& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_u32(vqmovn_u32(Data), arg1);
#else
    return vcombine_u16(vqmovn_u32(Data), vqmovn_u32(arg1));
#endif
}
template<> forceinline I8x16 VECCALL I16x8::Cast<I8x16, CastMode::RangeSaturate>(const I16x8& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_s16(vqmovn_s16(Data), arg1);
#else
    return vcombine_s8(vqmovn_s16(Data), vqmovn_s16(arg1));
#endif
}
template<> forceinline U8x16 VECCALL I16x8::Cast<U8x16, CastMode::RangeSaturate>(const I16x8& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovun_high_s16(vqmovun_s16(Data), arg1);
#else
    return vcombine_s8(vqmovun_s16(Data), vqmovun_s16(arg1));
#endif
}
template<> forceinline U8x16 VECCALL U16x8::Cast<U8x16, CastMode::RangeSaturate>(const U16x8& arg1) const noexcept
{
#if COMMON_SIMD_LV >= 200
    return vqmovn_high_u16(vqmovn_u16(Data), arg1);
#else
    return vcombine_u8(vqmovn_u16(Data), vqmovn_u16(arg1));
#endif
}


}
