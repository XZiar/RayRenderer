#pragma once

#include "oclRely.h"

namespace oclu
{



enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };

namespace detail
{


class OCLUAPI _oclDevice : public NonCopyable
{
    friend class _oclContext;
    friend class _oclPlatform;
    friend class _oclProgram;
    friend class _oclKernel;
    friend class _oclCmdQue;
    friend class ::oclu::oclUtil;
private:
    const cl_device_id deviceID;
    DeviceType getDevType() const;
    _oclDevice(const cl_device_id dID);
public:
    const uint64_t ConstantBufSize, GlobalMemSize, LocalMemSize, MaxMemSize, GlobalCacheSize, GlobalCacheLine;
    const bool supportProfiling, supportOutOfOrder;
    const u16string name, vendor, version;
    const DeviceType type;
};


}
using oclDevice = Wrapper<detail::_oclDevice>;


}

