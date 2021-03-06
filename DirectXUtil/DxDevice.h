#pragma once
#include "DxRely.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxDevice_;
using DxDevice = const DxDevice_*;
class DxQueryProvider;
class DxCmdQue_;
class DxCmdList_;
class DxResource_;
class DxShader_;
class DxProgram_;
class DxBindingManager;

class DXUAPI COMMON_EMPTY_BASES DxDevice_ : public common::NonCopyable, public common::NonMovable
{
    friend DxQueryProvider;
    friend DxCmdQue_;
    friend DxCmdList_;
    friend DxResource_;
    friend DxShader_;
    friend DxProgram_;
    friend DxBindingManager;
protected:
    PtrProxy<detail::Adapter> Adapter;
    PtrProxy<detail::Device> Device;
public:
    ~DxDevice_();

    [[nodiscard]] static common::span<const DxDevice> GetDevices();
public:
    enum class Architecture    : uint8_t { None = 0x0, TBR = 0x1, UMA = 0x2, CacheCoherent = 0x4, IsolatedMMU = 0x8 };
    enum class ShaderDType     : uint8_t { None = 0x0, FP64 = 0x1, INT64 = 0x2, FP16 = 0x4, INT16 = 0x8 };
    enum class OptionalSupport : uint8_t 
    { None = 0x0, DepthBoundTest = 0x1, CopyQueueTimeQuery = 0x2, BackgroundProcessing = 0x4 };
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
    constexpr OptionalSupport GetOptionalSupport() const noexcept { return OptSupport; }
private:
    MAKE_ENABLER();
    Architecture Arch;
    ShaderDType DtypeSupport;
    OptionalSupport OptSupport;
    DxDevice_(PtrProxy<detail::Adapter> adapter, PtrProxy<detail::Device> device, std::u16string_view name);
};
MAKE_ENUM_BITFIELD(DxDevice_::Architecture)
MAKE_ENUM_BITFIELD(DxDevice_::ShaderDType)
MAKE_ENUM_BITFIELD(DxDevice_::OptionalSupport)

constexpr inline bool DxDevice_::IsTBR() const noexcept 
{ 
    return HAS_FIELD(Arch, Architecture::TBR); 
}
constexpr inline bool DxDevice_::IsUMA() const noexcept
{
    return HAS_FIELD(Arch, Architecture::UMA);
}
constexpr inline bool DxDevice_::IsUMACacheCoherent() const noexcept
{
    return IsUMA() && HAS_FIELD(Arch, Architecture::CacheCoherent);
}
constexpr inline bool DxDevice_::IsIsolatedMMU() const noexcept
{
    return HAS_FIELD(Arch, Architecture::IsolatedMMU);
}
constexpr inline bool DxDevice_::SupportFP64() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::FP64);
}
constexpr inline bool DxDevice_::SupportINT64() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::INT64);
}
constexpr inline bool DxDevice_::SupportFP16() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::FP16);
}
constexpr inline bool DxDevice_::SupportINT16() const noexcept
{
    return HAS_FIELD(DtypeSupport, ShaderDType::INT16);
}


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
