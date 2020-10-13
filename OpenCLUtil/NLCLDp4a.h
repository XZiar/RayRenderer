#pragma once
#include "oclNLCLRely.h"
#include "common/StrParsePack.hpp"

namespace oclu
{

struct NLCLDp4aExtension;
struct Dp4aProvider;

struct NLCLRuntime_ : public NLCLRuntime
{
    friend Dp4aProvider;
    friend NLCLDp4aExtension;
};


struct Dp4aProvider
{
protected:
    NLCLContext& Context;
public:
    enum class Signedness { UU, US, SU, SS };
    Dp4aProvider(NLCLContext& context);
    virtual ~Dp4aProvider() { }
    virtual xcomp::ReplaceResult DP4A(Signedness, const common::span<const std::u32string_view>) { return {}; };
    virtual void OnFinish(NLCLRuntime_&) { }
};

struct NLCLDp4aExtension : public NLCLExtension
{
    common::mlog::MiniLogger<false>& Logger;
    std::shared_ptr<Dp4aProvider> DefaultProvider;
    std::shared_ptr<Dp4aProvider> Provider;
    bool HasIntelDp4a = false;

    NLCLDp4aExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) :
        NLCLExtension(context), Logger(logger)
    { }
    ~NLCLDp4aExtension() override { }
    [[nodiscard]] std::optional<xziar::nailang::Arg> XCNLFunc(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall>) override;
    xcomp::ReplaceResult ReplaceFunc(xcomp::XCNLRuntime& runtime, std::u32string_view func, const common::span<const std::u32string_view> args) override;
    void InstanceMeta(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& meta, xcomp::InstanceContext& ctx);
    void FinishInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) override;

    std::shared_ptr<Dp4aProvider> Generate(std::u32string_view mimic, std::u32string_view args) const;

    XCNL_EXT_REG(NLCLDp4aExtension,
    {
        if(const auto ctx = dynamic_cast<NLCLContext*>(&context); ctx)
            return std::make_unique<NLCLDp4aExtension>(logger, *ctx);
        return {};
    });
};


class NLCLDp4aPlain : public Dp4aProvider
{
protected:
public:
    using Dp4aProvider::Dp4aProvider;
    ~NLCLDp4aPlain() override { }
    xcomp::ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aIntel : public NLCLDp4aPlain
{
protected:
    const bool SupportDp4a;
public:
    NLCLDp4aIntel(NLCLContext& context, const bool supportDp4a);
    ~NLCLDp4aIntel() override { }
    xcomp::ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aArm : public NLCLDp4aPlain
{
protected:
    const bool SupportDPA8, SupportDP8;
    bool EnableDPA8 = false, EnableDP8 = false;
    void OnFinish(NLCLRuntime_& runtime) override;
public:
    NLCLDp4aArm(NLCLContext& context, const bool supportDPA8, const bool supportDP8);
    ~NLCLDp4aArm() override { }
    xcomp::ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aPtx : public NLCLDp4aPlain
{
protected:
    const uint32_t SMVersion;
public:
    NLCLDp4aPtx(NLCLContext& context, const uint32_t smVersion = 30);
    ~NLCLDp4aPtx() override { }
    xcomp::ReplaceResult DP4A(Signedness signedness, const common::span<const std::u32string_view> args) override;
};


}