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

class OCLUAPI oclContext_ : public common::NonCopyable, public common::NonMovable
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
    oclContext_(std::shared_ptr<const oclPlatform_> plat, std::vector<cl_context_properties> props, const std::vector<oclDevice>& devices);
public:
    std::vector<oclDevice> Devices;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img2DFormatSupport;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img3DFormatSupport;
    mutable common::Delegate<const std::u16string&> OnMessage;

    ~oclContext_();
    std::u16string GetPlatformName() const;
    Vendors GetVendor() const;
    oclDevice GetGPUDevice() const;
};
MAKE_ENABLER_IMPL(oclContext_)


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif