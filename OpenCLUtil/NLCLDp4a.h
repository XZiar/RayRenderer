#pragma once
#include "oclNLCLRely.h"
#include "common/StrParsePack.hpp"

namespace oclu
{

struct NLCLDp4aExtension;

struct Dp4aProvider
{
protected:
    NLCLContext& Context;
public:
    enum class Signedness : uint8_t { UU, US, SU, SS };
    class Dp4aType
    {
        uint8_t Val;
        static constexpr uint8_t SatBit    = 0x80;
        static constexpr uint8_t PackedBit = 0x40;
    public:
        constexpr Dp4aType(Signedness sign, bool isSat, bool isPacked) noexcept :
            Val(static_cast<uint8_t>(common::enum_cast(sign) + (isSat ? SatBit : 0u) + (isPacked ? PackedBit : 0u)))
        {}
        constexpr Signedness GetSignedness() const noexcept { return static_cast<Signedness>(Val & 0x0fu); }
        constexpr bool IsSat() const noexcept { return Val & SatBit; }
        constexpr bool IsPacked() const noexcept { return Val & PackedBit; }
    };
    Dp4aProvider(NLCLContext& context);
    virtual ~Dp4aProvider() { }
    virtual xcomp::ReplaceResult DP4A(const Dp4aType, const common::span<const std::u32string_view>) { return {}; };
    virtual void OnFinish(NLCLRuntime&) { }
};

struct NLCLDp4aExtension : public NLCLExtension
{
private:
    mutable std::shared_ptr<Dp4aProvider> DefaultProvider;
public:
    common::mlog::MiniLogger<false>& Logger;
    std::shared_ptr<Dp4aProvider> Provider;
    bool HasIntelDp4a = false;

    NLCLDp4aExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) :
        NLCLExtension(context), Logger(logger)
    { }
    ~NLCLDp4aExtension() override { }

    void  BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) final;
    void FinishInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) final;
    void InstanceMeta(xcomp::XCNLExecutor& executor, const xziar::nailang::FuncPack& meta, xcomp::InstanceContext& ctx) final;

    [[nodiscard]] xcomp::ReplaceResult ReplaceFunc(xcomp::XCNLRawExecutor& executor, std::u32string_view func,
        common::span<const std::u32string_view> args) final;
    [[nodiscard]] std::optional<xziar::nailang::Arg> ConfigFunc(xcomp::XCNLExecutor& executor, xziar::nailang::FuncEvalPack& func) final;

    std::shared_ptr<Dp4aProvider> GetDefaultProvider() const;
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
    xcomp::ReplaceResult DP4A(const Dp4aType type, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aKhr : public NLCLDp4aPlain
{
protected:
    const bool SupportUnpacked;
public:
    NLCLDp4aKhr(NLCLContext& context, const bool supportUnpacked);
    ~NLCLDp4aKhr() override { }
    xcomp::ReplaceResult DP4A(const Dp4aType type, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aIntel : public NLCLDp4aPlain
{
protected:
    const bool SupportDp4a;
public:
    NLCLDp4aIntel(NLCLContext& context, const bool supportDp4a);
    ~NLCLDp4aIntel() override { }
    xcomp::ReplaceResult DP4A(const Dp4aType type, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aArm : public NLCLDp4aPlain
{
protected:
    const bool SupportDP8, SupportDPA8, SupportDPA8S;
    bool EnableDP8 = false, EnableDPA8 = false, EnableDPA8S = false;
    void OnFinish(NLCLRuntime& runtime) override;
public:
    NLCLDp4aArm(NLCLContext& context, const bool supportDP8, const bool supportDPA8, const bool supportDPA8S);
    ~NLCLDp4aArm() override { }
    xcomp::ReplaceResult DP4A(const Dp4aType type, const common::span<const std::u32string_view> args) override;
};

class NLCLDp4aPtx : public NLCLDp4aPlain
{
protected:
    const uint32_t SMVersion;
public:
    NLCLDp4aPtx(NLCLContext& context, const uint32_t smVersion = 30);
    ~NLCLDp4aPtx() override { }
    xcomp::ReplaceResult DP4A(const Dp4aType type, const common::span<const std::u32string_view> args) override;
};


}