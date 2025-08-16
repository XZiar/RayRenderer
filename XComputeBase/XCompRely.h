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

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/FormatInclude.h"

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/EasyIterator.hpp"
#include "common/simd/SIMD.hpp"

#include <cstdio>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <memory>
#include <tuple>
#include <optional>
#include <variant>



namespace xcomp
{

struct PCI_BDF
{
    uint16_t Val;
    constexpr PCI_BDF() noexcept : Val(0) {}
    constexpr PCI_BDF(uint32_t bus, uint32_t dev, uint32_t func) noexcept :
        Val(static_cast<uint16_t>((bus << 8) | ((dev & 0b11111) << 3) | (func & 0b111))) {}
    [[nodiscard]] constexpr uint32_t Bus() const noexcept { return Val >> 8; }
    [[nodiscard]] constexpr uint32_t Device() const noexcept { return (Val >> 3) & 0b11111; }
    [[nodiscard]] constexpr uint32_t Function() const noexcept { return Val & 0b111; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return Val != 0 && Val != UINT16_MAX; }
    [[nodiscard]] constexpr bool operator==(const PCI_BDF& other) const noexcept { return Val == other.Val; }
    [[nodiscard]] constexpr bool operator!=(const PCI_BDF& other) const noexcept { return Val != other.Val; }
    XCOMPBASAPI void FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& context, const common::str::FormatSpec* spec) const;
    XCOMPBASAPI [[nodiscard]] std::string ToString() const noexcept;
};


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
        Expects(idx <= 0xffffffu);
    }
    [[nodiscard]] forceinline constexpr explicit operator bool() const noexcept
    { 
        return Type != DataTypes::Empty;
    }
    [[nodiscard]] forceinline constexpr bool IsCustomType() const noexcept
    {
        return Type == DataTypes::Custom;
    }
    [[nodiscard]] forceinline constexpr bool HasFlag(TypeFlags flag) const noexcept;
    [[nodiscard]] forceinline constexpr bool IsSupportVec() const noexcept
    {
        return !HasFlag(TypeFlags::Unsupport);
    }
    [[nodiscard]] forceinline constexpr operator uint32_t() const noexcept
    {
        return (static_cast<uint32_t>(Dims) << 0) | (static_cast<uint32_t>(Bits) << 8)
            | (static_cast<uint32_t>(Flag) << 16) | (static_cast<uint32_t>(Type) << 24);
    }
    [[nodiscard]] forceinline constexpr uint32_t ToIndex() const noexcept
    {
        return (static_cast<uint32_t>(Dims) << 0) | (static_cast<uint32_t>(Bits) << 8) | (static_cast<uint32_t>(Flag) << 16);
    }
    [[nodiscard]] forceinline constexpr common::simd::VecDataInfo ToVecDataInfo() const noexcept
    {
        return { Type, Bits, Dim0(), Dim1() };
    }
    [[nodiscard]] forceinline constexpr uint8_t Dim0() const noexcept
    {
        return DimUnPackMap[Dims & 0xfu];
    }
    [[nodiscard]] forceinline constexpr uint8_t Dim1() const noexcept
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
#undef CHECK_N
        default: return false;
        }
    }
};


struct CommonDeviceInfo
{
    enum class ICDTypes : uint32_t { OpenGL, OpenCL, Vulkan, LevelZero };
    std::u16string Name;
    std::array<std::byte, 16> Guid = { std::byte(0) };
    std::array<std::byte, 8>  Luid = { std::byte(0) };
    uint32_t VendorId = 0, DeviceId = 0;
    uint32_t DedicatedVRAM = 0, DedicatedSysRAM = 0, SharedSysRAM = 0;
    PCI_BDF PCIEAddress;
    bool IsSoftware : 1;
    bool SupportCompute : 1;
    bool SupportRender : 1;
    bool SupportDisplay : 1;
    bool SupportPV : 1;
    CommonDeviceInfo() noexcept;
    [[nodiscard]] virtual std::u16string_view GetICDPath(ICDTypes type) const noexcept;
    [[nodiscard]] virtual std::u16string_view GetDevicePath() const noexcept;
    [[nodiscard]] virtual uint32_t GetDisplay() const noexcept;
    [[nodiscard]] virtual bool CheckDisplay(uint32_t id) const noexcept;
    XCOMPBASAPI void FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& context, const common::str::FormatSpec* spec) const;
};
struct CommonDeviceContainer
{
private:
    [[nodiscard]] virtual const CommonDeviceInfo* LocateByDevicePath(std::u16string_view devPath) const noexcept = 0;
protected:
    [[nodiscard]] virtual const CommonDeviceInfo& Get(size_t index) const noexcept = 0;
    [[nodiscard]] virtual size_t GetSize() const noexcept = 0;
    using ItType = common::container::IndirectIterator<const CommonDeviceContainer, const CommonDeviceInfo&, &CommonDeviceContainer::Get>;
    friend ItType;
public:
    [[nodiscard]] constexpr ItType begin() const noexcept
    {
        return { this, 0 };
    }
    [[nodiscard]] constexpr ItType end() const noexcept
    {
        return { this, GetSize() };
    }
    [[nodiscard]] size_t GetDeviceIndex(const CommonDeviceInfo* dev) const noexcept
    {
        if (!dev) return SIZE_MAX;
        const auto size = GetSize();
        for (size_t i = 0; i < size; ++i)
        {
            if (dev == &Get(i))
                return i;
        }
        return SIZE_MAX;
    }
    XCOMPBASAPI [[nodiscard]] const CommonDeviceInfo* LocateExactDevice(const std::array<std::byte, 8>* luid,
        const std::array<std::byte, 16>* guid, const PCI_BDF* pcie, std::u16string_view devPath) const noexcept;
    XCOMPBASAPI [[nodiscard]] const CommonDeviceInfo* TryLocateDevice(const uint32_t* vid, const uint32_t* did, std::u16string_view name) const noexcept;
};
#if COMMON_OS_WIN
struct XCOMPBASAPI D3DDeviceLocator
{
    [[nodiscard]] virtual const CommonDeviceInfo* LocateExactDevice(void* d3dHandle) const noexcept = 0;
};
#endif

XCOMPBASAPI const CommonDeviceContainer& ProbeDevice();


struct XCOMPBASAPI RangeHolder
{
private:
    struct Range;
    std::weak_ptr<Range> CurrentRange;
    [[nodiscard]] virtual std::shared_ptr<const RangeHolder> BeginRange(std::u16string_view msg) const noexcept = 0;
    virtual void EndRange() const noexcept = 0;
protected:
    [[nodiscard]] bool CheckRangeEmpty() const noexcept
    {
        return CurrentRange.expired();
    }
    virtual ~RangeHolder();
public:
    [[nodiscard]] std::shared_ptr<void> DeclareRange(std::u16string_view msg);
    virtual void AddMarker(std::u16string_view name) const noexcept = 0;
};


XCOMPBASAPI VTypeInfo ParseVDataType(const std::u32string_view type) noexcept;
XCOMPBASAPI std::u32string_view StringifyVDataType(const VTypeInfo vtype) noexcept;

}

