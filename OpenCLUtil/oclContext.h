#pragma once

#include "oclRely.h"
#include "oclDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oclu
{

namespace detail
{


class OCLUAPI _oclContext : public NonCopyable, public NonMovable
{
    friend class _oclPlatform;
    friend class _oclCmdQue;
    friend class _oclProgram;
    friend class _oclBuffer;
    friend class _oclImage;
    friend class GLInterOP;
private:
    const cl_context context;
    static cl_context CreateContext(vector<cl_context_properties>& props, const vector<oclDevice>& devices, void* self);
    _oclContext(vector<cl_context_properties> props, const vector<oclDevice>& devices, const u16string name, const Vendor thevendor);
    oclDevice GetDevice(const cl_device_id devid) const;
public:
    const vector<oclDevice> Devices;
    const u16string PlatformName;
    const common::container::FrozenDenseSet<oglu::TextureDataFormat> Img2DFormatSupport;
    const common::container::FrozenDenseSet<oglu::TextureDataFormat> Img3DFormatSupport;
    const Vendor vendor;
    MessageCallBack onMessage = nullptr;
    ~_oclContext();
    oclDevice GetGPUDevice() const;
};

}
using oclContext = Wrapper<detail::_oclContext>;


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif