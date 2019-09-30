#pragma once

#include "oclRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };


class OCLUAPI oclDevice_ : public NonCopyable, public NonMovable
{
    friend class oclContext_;
    friend class oclPlatform_;
    friend class oclProgram_;
    friend class oclKernel_;
    friend class oclCmdQue_;
    friend class oclUtil;
private:
    MAKE_ENABLER();
    const std::weak_ptr<const oclPlatform_> Plat;
    const cl_device_id deviceID;
    oclDevice_(std::weak_ptr<const oclPlatform_> plat, const cl_device_id dID);
public:
    const u16string Name, Vendor, Version;
    const common::container::FrozenDenseSet<string> Extensions;
    const uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemSize, GlobalCacheSize, GlobalCacheLine;
    const uint32_t MemBaseAddrAlign;
    const bool SupportProfiling, SupportOutOfOrder, SupportImplicitGLSync;
    const DeviceType Type;
    u16string_view GetTypeName() const { return GetDeviceTypeName(Type); }
    static u16string_view GetDeviceTypeName(const DeviceType type);
};
MAKE_ENABLER_IMPL(oclDevice_)


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

