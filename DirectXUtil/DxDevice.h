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
    std::u16string AdapterName;
    const xcomp::CommonDeviceInfo* XCompDevice = nullptr;
    uint32_t SMVer = 0;
    std::pair<uint32_t, uint32_t> WaveSize = { 0,0 };
    xcomp::PCI_BDF PCIEAddress;
    std::pair<CPUPageProps, MemPrefer> HeapUpload, HeapDefault, HeapReadback;
    bool IsTBR              : 1 = false;
    bool IsUMA              : 1 = false;
    bool IsUMACacheCoherent : 1 = false;
    bool IsIsolatedMMU      : 1 = false;
    bool SupportFP64        : 1 = false;
    bool SupportINT64       : 1 = false;
    bool SupportFP16        : 1 = false;
    bool SupportINT16       : 1 = false;
    bool SupportROV         : 1 = false;
    bool ExtComputeResState     : 1 = false;
    bool DepthBoundTest         : 1 = false;
    bool CopyQueueTimeQuery     : 1 = false;
    bool BackgroundProcessing   : 1 = false;
    [[nodiscard]] std::array<std::byte, 8> GetLUID() const noexcept;
private:
    MAKE_ENABLER();
    DxDevice_(PtrProxy<detail::Adapter> adapter, PtrProxy<detail::Device> device, std::u16string_view name);
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
