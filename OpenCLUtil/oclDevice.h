#pragma once

#include "oclRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclDevice_;
using oclDevice = const oclDevice_*;

enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };


class OCLUAPI oclDevice_ : public NonCopyable
{
    friend class oclUtil;
    friend class GLInterop;
    friend class oclContext_;
    friend class oclPlatform_;
    friend class oclProgram_;
    friend class oclKernel_;
    friend class oclCmdQue_;
private:
    cl_device_id DeviceID;
    oclDevice_(const cl_device_id dID);
public:
    u16string Name, Vendor, Version;
    common::container::FrozenDenseSet<string> Extensions;
    uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemSize, GlobalCacheSize, GlobalCacheLine;
    uint32_t MemBaseAddrAlign;
    bool SupportProfiling, SupportOutOfOrder, SupportImplicitGLSync;
    DeviceType Type;

    u16string_view GetTypeName() const { return GetDeviceTypeName(Type); }

    static u16string_view GetDeviceTypeName(const DeviceType type);
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

