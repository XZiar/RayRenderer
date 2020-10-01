#pragma once
#include "oclNLCLRely.h"
#include "common/StrParsePack.hpp"

namespace oclu
{


struct NLCLRuntime_ : public NLCLRuntime
{
    friend struct Dp4aProvider;
    friend struct KernelDp4aExtension;
    friend struct NLCLDp4aExtension;
    friend class NLCLDp4aIntel;
    friend class NLCLDp4aArm;
    friend class NLCLDp4aPtx;
    friend class NLCLDp4aPlain;
};


struct KernelDp4aExtension;
struct Dp4aProvider
{
protected:
    NLCLContext& Context;
public:
    enum class Signedness { UU, US, SU, SS };
    Dp4aProvider(NLCLContext& context);
    virtual ~Dp4aProvider() { }
    virtual ReplaceResult DP4A(Signedness, const common::span<const std::u32string_view>) { return {}; };
    virtual void OnFinish(NLCLRuntime_&, const KernelDp4aExtension&, KernelContext&) { }
};

struct NLCLDp4aExtension : public NLCLExtension
{
    common::mlog::MiniLogger<false>& Logger;
    bool HasIntelDp4a = false;

    NLCLDp4aExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) :
        NLCLExtension(context), Logger(logger)
    { }
    ~NLCLDp4aExtension() override { }
    [[nodiscard]] std::optional<xziar::nailang::Arg> NLCLFunc(NLCLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall> metas, const xziar::nailang::NailangRuntimeBase::FuncTargetType target) override;

    std::shared_ptr<Dp4aProvider> Generate(std::u32string_view mimic, std::u32string_view args) const;
    inline static uint32_t ID = NLCLExtension::RegisterNLCLExtenstion(
        [](common::mlog::MiniLogger<false>& logger, NLCLContext& context, const xziar::nailang::Block&) -> std::unique_ptr<NLCLExtension>
        {
            return std::make_unique<NLCLDp4aExtension>(logger, context);
        });
};

struct KernelDp4aExtension : public KernelExtension
{
    KernelContext& Kernel;
    std::shared_ptr<Dp4aProvider> Provider;
    uint32_t SubgroupSize = 0;

    KernelDp4aExtension(NLCLContext& context, KernelContext& kernel) :
        KernelExtension(context), Kernel(kernel),
        Provider(Context.GetNLCLExt<NLCLDp4aExtension>()->Generate({}, {}))
    { }
    ~KernelDp4aExtension() override { }

    bool KernelMeta(NLCLRuntime& runtime, const xziar::nailang::FuncCall& meta, KernelContext& kernel) override;
    ReplaceResult ReplaceFunc(NLCLRuntime& runtime, const std::u32string_view func, const common::span<const std::u32string_view> args) override;
    void FinishKernel(NLCLRuntime& runtime, KernelContext& kernel) override;

    inline static uint32_t ID = NLCLExtension::RegisterKernelExtenstion(
        [](auto&, NLCLContext& context, KernelContext& kernel) -> std::unique_ptr<KernelExtension>
        {
            return std::make_unique<KernelDp4aExtension>(context, kernel);
        });
};


class NLCLDp4aPlain : public Dp4aProvider
{
protected:
public:
    using Dp4aProvider::Dp4aProvider;
    ~NLCLDp4aPlain() override { }
    ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aIntel : public NLCLDp4aPlain
{
protected:
    const bool SupportDp4a;
public:
    NLCLDp4aIntel(NLCLContext& context, const bool supportDp4a);
    ~NLCLDp4aIntel() override { }
    ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aArm : public NLCLDp4aPlain
{
protected:
    const bool SupportDPA8, SupportDP8;
    bool EnableDPA8 = false, EnableDP8 = false;
    void OnFinish(NLCLRuntime_& runtime, const KernelDp4aExtension& ext, KernelContext& kernel) override;
public:
    NLCLDp4aArm(NLCLContext& context, const bool supportDPA8, const bool supportDP8);
    ~NLCLDp4aArm() override { }
    ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aPtx : public NLCLDp4aPlain
{
protected:
    const uint32_t SMVersion;
public:
    NLCLDp4aPtx(NLCLContext& context, const uint32_t smVersion = 30);
    ~NLCLDp4aPtx() override { }
    ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};


}