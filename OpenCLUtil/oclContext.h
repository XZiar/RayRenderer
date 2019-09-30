#pragma once

#include "oclRely.h"
#include "oclDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

class OCLUAPI oclContext_ : public NonCopyable, public NonMovable
{
    friend class GLInterop;
    friend class oclPlatform_;
    friend class oclCmdQue_;
    friend class oclProgram_;
    friend class oclBuffer_;
    friend class oclImage_;
private:
    MAKE_ENABLER();
    const std::shared_ptr<const oclPlatform_> Plat;
    const cl_context context;
    static cl_context CreateContext(vector<cl_context_properties>& props, const vector<oclDevice>& devices, void* self);
    oclContext_(const std::shared_ptr<const oclPlatform_>& plat, vector<cl_context_properties> props, const vector<oclDevice>& devices, const u16string name, const Vendor thevendor);
    oclDevice GetDevice(const cl_device_id devid) const;
public:
    const vector<oclDevice> Devices;
    const u16string PlatformName;
    const common::container::FrozenDenseSet<xziar::img::TextureFormat> Img2DFormatSupport;
    const common::container::FrozenDenseSet<xziar::img::TextureFormat> Img3DFormatSupport;
    const Vendor vendor;
    MessageCallBack onMessage = nullptr;
    ~oclContext_();
    oclDevice GetGPUDevice() const;
};
MAKE_ENABLER_IMPL(oclContext_)


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif