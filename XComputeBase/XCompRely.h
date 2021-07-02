#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef XCOMPBAS_EXPORT
#   define XCOMPBASAPI _declspec(dllexport)
# else
#   define XCOMPBASAPI _declspec(dllimport)
# endif
#else
# define XCOMPBASAPI [[gnu::visibility("default")]]
#endif

#include "MiniLogger/MiniLogger.h"

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"
#include "common/simd/SIMD.hpp"

#include <cstdio>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <tuple>
#include <optional>
#include <variant>



namespace xcomp
{

struct VTypeInfo
{
    static constexpr uint8_t DimPackMap[33] = { 0,1,2,3,4,0,0,0,5,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7 };
    static constexpr uint8_t DimUnPackMap[16] = { 0,1,2,3,4,8,16,32,0,0,0,0,0,0,0,0 };
    static constexpr uint8_t PackDims(uint32_t dim0, uint32_t dim1) noexcept
    {
        const auto dim0_ = dim0 <= 32 ? DimPackMap[dim0] : 0u;
        const auto dim1_ = dim1 <= 32 ? DimPackMap[dim1] : 0u;
        return static_cast<uint8_t>(dim0_ | (dim1_ << 4));
    }
    using DataTypes = common::simd::VecDataInfo::DataTypes;
    enum class TypeFlags : uint8_t { Empty = 0, Unsupport = 0b00000001, MinBits = 0b00000010 };
    uint8_t Dims;
    uint8_t Bits;
    TypeFlags Flag;
    DataTypes Type;
    constexpr VTypeInfo() noexcept :
        Dims(0), Bits(0), Flag(VTypeInfo::TypeFlags::Empty), Type(DataTypes::Empty)
    { }
    constexpr VTypeInfo(DataTypes type, uint8_t dim0, uint8_t dim1, uint8_t bits, TypeFlags flag = VTypeInfo::TypeFlags::Empty) noexcept :
        Dims(PackDims(dim0, dim1)), Bits(bits), Flag(flag), Type(type)
    { }
    constexpr VTypeInfo(common::simd::VecDataInfo vinfo) noexcept :
        VTypeInfo(vinfo.Type, vinfo.Dim0, vinfo.Dim1, vinfo.Bit) 
    { }
    constexpr VTypeInfo(uint32_t idx) noexcept :
        Dims(static_cast<uint8_t>(idx)), Bits(static_cast<uint8_t>(idx >> 8)), Flag(static_cast<TypeFlags>(idx >> 16)), Type(DataTypes::Custom) 
    {
        Expects(idx <= 0xffffff);
    }
    forceinline constexpr explicit operator bool() const noexcept 
    { 
        return Type != DataTypes::Empty;
    }
    forceinline constexpr bool IsCustomType() const noexcept
    {
        return Type == DataTypes::Custom;
    }
    forceinline constexpr bool HasFlag(TypeFlags flag) const noexcept;
    forceinline constexpr bool IsSupportVec() const noexcept
    {
        return !HasFlag(TypeFlags::Unsupport);
    }
    forceinline constexpr operator uint32_t() const noexcept
    {
        return (static_cast<uint32_t>(Dims) << 0) | (static_cast<uint32_t>(Bits) << 8)
            | (static_cast<uint32_t>(Flag) << 16) | (static_cast<uint32_t>(Type) << 24);
    }
    forceinline constexpr uint32_t ToIndex() const noexcept
    {
        return (static_cast<uint32_t>(Dims) << 0) | (static_cast<uint32_t>(Bits) << 8) | (static_cast<uint32_t>(Flag) << 16);
    }
    forceinline constexpr common::simd::VecDataInfo ToVecDataInfo() const noexcept
    {
        return { Type, Bits, Dim0(), Dim1() };
    }
    forceinline constexpr uint8_t Dim0() const noexcept
    {
        return DimUnPackMap[Dims & 0xfu];
    }
    forceinline constexpr uint8_t Dim1() const noexcept
    {
        return DimUnPackMap[Dims >> 4];
    }
    forceinline constexpr void SetDims(uint8_t dim0, uint8_t dim1) noexcept
    {
        Dims = PackDims(dim0, dim1);
    }
};
MAKE_ENUM_BITFIELD(VTypeInfo::TypeFlags)
inline constexpr bool VTypeInfo::HasFlag(TypeFlags flag) const noexcept
{
    return !IsCustomType() && HAS_FIELD(Flag, flag);
}

enum class VTypeDimSupport : uint8_t
{
    Empty = 0x0, Support1 = 0x1, Support2 = 0x2, Support3 = 0x4, Support4 = 0x8, Support8 = 0x10, Support16 = 0x20, Support32 = 0x40,
    All = 0xff
};
MAKE_ENUM_BITFIELD(VTypeDimSupport)
struct VecDimSupport
{
    VTypeDimSupport Support;
    constexpr VecDimSupport(VTypeDimSupport support = VTypeDimSupport::Empty) noexcept : Support(support) {}
    [[nodiscard]] constexpr bool Check(uint8_t vec) const noexcept
    {
        switch (vec)
        {
#define CHECK_N(n) case n: return HAS_FIELD(Support, VTypeDimSupport::Support##n)
        CHECK_N(1);
        CHECK_N(2);
        CHECK_N(3);
        CHECK_N(4);
        CHECK_N(8);
        CHECK_N(16);
        CHECK_N(32);
        default: return false;
        }
    }
};

XCOMPBASAPI VTypeInfo ParseVDataType(const std::u32string_view type) noexcept;
XCOMPBASAPI std::u32string_view StringifyVDataType(const VTypeInfo vtype) noexcept;

}