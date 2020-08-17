#pragma once
#include "dxRely.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXDevice_;
using DXDevice = const DXDevice_*;
class DXCopyCmdQue_;
class DXCmdList_;
class DXResource_;

class DXUAPI COMMON_EMPTY_BASES DXDevice_ : public common::NonCopyable, public common::NonMovable
{
    friend class DXCopyCmdQue_;
    friend class DXCmdList_;
    friend class DXResource_;
protected:
    struct AdapterProxy;
    struct DeviceProxy;
    PtrProxy<AdapterProxy> Adapter;
    PtrProxy<DeviceProxy> Device;
public:
    ~DXDevice_();

    [[nodiscard]] static common::span<const DXDevice> GetDevices();
public:
    enum class Architecture : uint8_t { None = 0x0, TBR = 0x1, UMA = 0x2, CacheCoherent = 0x4, IsolatedMMU = 0x8 };
    enum class ShaderDType  : uint8_t { None = 0x0, FP64 = 0x1, INT64 = 0x2, FP16 = 0x4, INT16 = 0x8 };
    std::u16string AdapterName;
    uint32_t SMVer;
    uint32_t WaveSize;
    std::pair<CPUPageProps, MemPrefer> HeapUpload, HeapDefault, HeapReadback;
    constexpr bool IsTBR() const noexcept;
    constexpr bool IsUMA() const noexcept;
    constexpr bool IsUMACacheCoherent() const noexcept;
    constexpr bool IsIsolatedMMU() const noexcept;
    constexpr bool SupportFP64() const noexcept;
    constexpr bool SupportINT64() const noexcept;
    constexpr bool SupportFP16() const noexcept;
    constexpr bool SupportINT16() const noexcept;
private:
    MAKE_ENABLER();
    Architecture Arch;
    ShaderDType DtypeSupport;
    DXDevice_(PtrProxy<AdapterProxy> adapter, PtrProxy<DeviceProxy> device, std::u16string_view name);
};
MAKE_ENUM_BITFIELD(DXDevice_::Architecture)
MAKE_ENUM_BITFIELD(DXDevice_::ShaderDType)

constexpr inline bool DXDevice_::IsTBR() const noexcept 
{ 
    return HAS_FIELD(Arch, Architecture::TBR); 
}
constexpr inline bool DXDevice_::IsUMA() const noexcept
{
    return HAS_FIELD(Arch, Architecture::UMA);
}
constexpr inline bool DXDevice_::IsUMACacheCoherent() const noexcept
{
    return IsUMA() && HAS_FIELD(Arch, Architecture::CacheCoherent);
}
constexpr inline bool DXDevice_::IsIsolatedMMU() const noexcept
{
    return HAS_FIELD(Arch, Architecture::IsolatedMMU);
}
constexpr inline bool DXDevice_::SupportFP64() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::FP64);
}
constexpr inline bool DXDevice_::SupportINT64() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::INT64);
}
constexpr inline bool DXDevice_::SupportFP16() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::FP16);
}
constexpr inline bool DXDevice_::SupportINT16() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::INT16);
}

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
