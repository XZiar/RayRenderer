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

class OCLUAPI oclContext_ : public detail::oclCommon
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
    CLHandle<detail::CLContext> Context;
    const oclPlatform_* Plat;
    mutable std::atomic_bool DebugResource = false;
    oclContext_(const oclPlatform_* plat, std::vector<intptr_t> props, common::span<const oclDevice> devices);
public:
    std::vector<oclDevice> Devices;
    uint32_t Version;
    COMMON_NO_COPY(oclContext_)
    COMMON_NO_MOVE(oclContext_)
    ~oclContext_();
    [[nodiscard]] std::u16string GetPlatformName() const;
    [[nodiscard]] Vendors GetVendor() const;
    [[nodiscard]] oclDevice GetGPUDevice() const;
    void SetDebugResource(const bool shouldEnable) const;
    [[nodiscard]] bool ShouldDebugResurce() const;
    [[nodiscard]] bool CheckExtensionSupport(const std::string_view name) const;
    [[nodiscard]] bool CheckIncludeDevice(const oclDevice dev) const noexcept;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img2DFormatSupport;
    common::container::FrozenDenseSet<xziar::img::TextureFormat> Img3DFormatSupport;
    mutable common::Delegate<const std::u16string&> OnMessage;

    common::PromiseResult<void> CreateUserEvent(common::PmsCore pms);
};
MAKE_ENABLER_IMPL(oclContext_)


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif