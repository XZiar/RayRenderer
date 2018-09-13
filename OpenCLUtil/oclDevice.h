#pragma once

#include "oclRely.h"

namespace oclu
{



enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };

namespace detail
{


class OCLUAPI _oclDevice : public NonCopyable, public NonMovable
{
    friend class _oclContext;
    friend class _oclPlatform;
    friend class _oclProgram;
    friend class _oclKernel;
    friend class _oclCmdQue;
    friend class ::oclu::oclUtil;
private:
    const cl_device_id deviceID;
    _oclDevice(const cl_device_id dID);
public:
    const u16string Name, Vendor, Version;
    const common::container::FrozenDenseSet<string> Extensions;
    const uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemSize, GlobalCacheSize, GlobalCacheLine;
    const bool SupportProfiling, SupportOutOfOrder, SupportImplicitGLSync;
    const DeviceType Type;
    static u16string_view GetDeviceTypeName(const DeviceType type);
};


}
using oclDevice = Wrapper<detail::_oclDevice>;


}

