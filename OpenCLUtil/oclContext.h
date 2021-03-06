#pragma once

#include "oclRely.h"
#include "oclDevice.h"


#if COMMON_COMPILER_MSVC
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
    friend class NLCLContext;
    friend class oclPlatform_;
    friend class oclCmdQue_;
    friend class oclProgram_;
    friend class oclProgStub;
    friend class oclMem_;
    friend class oclBuffer_;
    friend class oclImage_;
private:
    MAKE_ENABLER();
    oclContext_(std::shared_ptr<const oclPlatform_> plat, std::vector<cl_context_properties> props, const std::vector<oclDevice>& devices);
public:
    ~oclContext_();
    [[nodiscard]] std::u16string GetPlatformName() const;
    [[nodiscard]] Vendors GetVendor() const;
    [[nodiscard]] oclDevice GetGPUDevice() const;
    void SetDebugResource(const bool shouldEnable) const;
    [[nodiscard]] bool ShouldDebugResurce() const;
    [[nodiscard]] bool CheckExtensionSupport(const std::string_view name) const;
    [[nodiscard]] bool CheckIncludeDevice(const oclDevice dev) const noexcept;

private:
    const std::shared_ptr<const oclPlatform_> Plat;
    cl_context Context = nullptr;
public:
    const std::vector<oclDevice> Devices;
    const uint32_t Version;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img2DFormatSupport;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img3DFormatSupport;
    mutable common::Delegate<const std::u16string&> OnMessage;

    common::PromiseResult<void> CreateUserEvent(common::PmsCore pms);
private:
    mutable std::atomic_bool DebugResource = false;
};
MAKE_ENABLER_IMPL(oclContext_)


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif