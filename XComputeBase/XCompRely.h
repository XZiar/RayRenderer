#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef XCOMPBAS_EXPORT
#   define XCOMPBASAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define XCOMPBASAPI _declspec(dllimport)
# endif
#else
# ifdef XCOMPBAS_EXPORT
#   define XCOMPBASAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define XCOMPBASAPI
# endif
#endif

#include "MiniLogger/MiniLogger.h"

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"
#include "common/SIMD.hpp"
#include "gsl/gsl_assert"

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
    static constexpr uint8_t PackDims(uint32_t dim0, uint32_t dim1) noexcept
    {
        dim0 -= 1;
        dim1 -= 1;
        return static_cast<uint8_t>((dim0 & 0xfu) | (dim1 << 4));
    }
    using DataTypes = common::simd::VecDataInfo::DataTypes;
    enum class TypeFlags : uint8_t { Empty = 0, Unsupport = 0b00000001, MinBits = 0b00000010 };
    uint8_t Dims;
    uint8_t Bits;
    TypeFlags Flag;
    DataTypes Type;
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
        return (Dims & 0xfu) + 1;
    }
    forceinline constexpr uint8_t Dim1() const noexcept
    {
        return (Dims >> 4) + 1;
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

XCOMPBASAPI VTypeInfo ParseVDataType(const std::u32string_view type) noexcept;
XCOMPBASAPI std::u32string_view StringifyVDataType(const VTypeInfo vtype) noexcept;

}