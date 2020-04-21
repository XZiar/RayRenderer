#pragma once

#include "oclRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
class oclDevice_;
using oclDevice = const oclDevice_*;

enum class DeviceType : uint8_t { Other, CPU, GPU, Accelerator, Custom };


class OCLUAPI oclDevice_ : public common::NonCopyable
{
    friend class oclUtil;
    friend class GLInterop;
    friend class oclContext_;
    friend class oclPlatform_;
    friend class oclProgram_;
    friend class oclKernel_;
    friend class oclCmdQue_;
private:
    std::weak_ptr<const oclPlatform_> Platform;
    cl_device_id DeviceID;
    oclDevice_(const std::weak_ptr<const oclPlatform_>& plat, const cl_device_id dID);
public:
    std::u16string Name, Vendor, Ver, CVer;
    common::container::FrozenDenseStringSet<char> Extensions;
    uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemSize, GlobalCacheSize;
    uint32_t GlobalCacheLine, MemBaseAddrAlign, ComputeUnits;
    uint32_t Version, CVersion;
    bool SupportProfiling, SupportOutOfOrder, SupportImplicitGLSync;
    DeviceType Type;

    [[nodiscard]] std::u16string_view GetTypeName() const { return GetDeviceTypeName(Type); }
    [[nodiscard]] std::shared_ptr<const oclPlatform_> GetPlatform() const;

    [[nodiscard]] static std::u16string_view GetDeviceTypeName(const DeviceType type);
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

