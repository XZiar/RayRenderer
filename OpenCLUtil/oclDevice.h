#pragma once

#include "oclRely.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
class oclDevice_;
using oclDevice = const oclDevice_*;

enum class DeviceType : uint8_t { Other, CPU, GPU, Accelerator, Custom };

enum class FPConfig : uint64_t 
{ 
    Empty = 0, Denorm = 0x1, InfNaN = 0x2, 
    RoundNearest = 0x4, RoundZero = 0x8, RoundInf = 0x10,
    FMA = 0x20, SoftFloat = 0x40, DivSqrtCorrect = 0x80,
};
MAKE_ENUM_BITFIELD(FPConfig)

class OCLUAPI oclDevice_ : public detail::oclCommon
{
    friend class oclPlatform_;
private:
    oclDevice_(const oclPlatform_* plat, void* dID);
public:
    CLHandle<detail::CLDevice> DeviceID;
    const oclPlatform_* Platform;
    std::u16string Name, Vendor, Ver, CVer;
    common::container::FrozenDenseStringSet<char> Extensions;
    FPConfig F64Caps, F32Caps, F16Caps;
    uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemAllocSize, GlobalCacheSize;
    size_t MaxWorkItemSize[3], MaxWorkGroupSize;
    uint32_t GlobalCacheLine, MemBaseAddrAlign, ComputeUnits, MaxSubgroupCount, WaveSize;
    uint32_t VendorId, PCIEBus, PCIEDev, PCIEFunc;
    uint32_t Version, CVersion;
    bool SupportProfiling, SupportOutOfOrder, SupportImplicitGLSync, SupportImage;
    bool LittleEndian, HasCompiler;
    Vendors PlatVendor;
    DeviceType Type;

    COMMON_NO_COPY(oclDevice_)
    COMMON_DEF_MOVE(oclDevice_)
    [[nodiscard]] std::u16string_view GetTypeName() const noexcept { return GetDeviceTypeName(Type); }
    [[nodiscard]] std::optional<uint32_t> GetNvidiaSMVersion() const noexcept;

    [[nodiscard]] static std::u16string_view GetDeviceTypeName(const DeviceType type);
    [[nodiscard]] static std::string GetFPCapabilityStr(const FPConfig cap);
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

