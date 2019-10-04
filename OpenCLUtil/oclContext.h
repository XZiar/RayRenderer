#pragma once

#include "oclRely.h"
#include "oclDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
class oclContext_;
using oclContext = std::shared_ptr<const oclContext_>;

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
    cl_context Context = nullptr;
    oclContext_(std::shared_ptr<const oclPlatform_> plat, vector<cl_context_properties> props, const vector<oclDevice>& devices);
public:
    vector<oclDevice> Devices;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img2DFormatSupport;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img3DFormatSupport;
    mutable MessageCallBack onMessage = nullptr;

    ~oclContext_();
    u16string GetPlatformName() const;
    Vendors GetVendor() const;
    void OnMessage(const std::u16string& msg) const;
    oclDevice GetGPUDevice() const;
};
MAKE_ENABLER_IMPL(oclContext_)


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif