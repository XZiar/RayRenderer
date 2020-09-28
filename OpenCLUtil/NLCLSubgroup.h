#pragma once
#include "oclNLCLRely.h"
#include "common/StrParsePack.hpp"

namespace oclu
{

struct SubgroupAttributes
{
    enum class MimicType { None, Auto, Local, Ptx };
    std::string Args;
    MimicType Mimic = MimicType::None;
};


struct NLCLRuntime_ : public NLCLRuntime
{
    friend struct SubgroupProvider;
    friend struct NLCLSubgroupExtension;
    friend struct KernelSubgroupExtension;
    friend class NLCLSubgroupLocal;
    friend class NLCLSubgroupPtx;
};

struct SubgroupCapbility
{
    bool SupportSubgroupKHR = false, SupportSubgroupIntel = false, SupportSubgroup8Intel = false, SupportSubgroup16Intel = false,
        SupportFP16 = false, SupportFP64 = false,
        SupportBasicSubgroup = false;
};


struct KernelSubgroupExtension;
struct SubgroupProvider
{
protected:
    [[nodiscard]] static std::optional<common::simd::VecDataInfo> DecideVtype(common::simd::VecDataInfo vtype,
        common::span<const common::simd::VecDataInfo> scalars) noexcept;
    [[nodiscard]] static std::u32string ScalarPatch(const std::u32string_view name, const std::u32string_view baseFunc, common::simd::VecDataInfo vtype,
        const std::u32string_view extraArg = {}, const std::u32string_view extraParam = {}) noexcept;
    [[nodiscard]] forceinline static std::u32string ScalarPatchBcastShuf(
        const std::u32string_view name, const std::u32string_view baseFunc, common::simd::VecDataInfo vtype) noexcept
    {
        return ScalarPatch(name, baseFunc, vtype, U", sgId", U", const uint sgId");
    }

    NLCLContext& Context;
    const SubgroupCapbility Cap;
    void AddWarning(common::str::StrVariant<char16_t> msg);
public:
    SubgroupProvider(NLCLContext& context, SubgroupCapbility cap);
    virtual ~SubgroupProvider() { }
    virtual std::u32string GetSubgroupLocalId() { return {}; };
    virtual std::u32string GetSubgroupSize(const uint32_t) { return {}; };
    virtual std::u32string SubgroupAll(const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupAny(const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupBroadcast(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupShuffle(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupSum(common::simd::VecDataInfo, const std::u32string_view) { return {}; };
    virtual void OnFinish(NLCLRuntime_&, const KernelSubgroupExtension&, KernelContext&);
private:
    std::vector<std::u16string> Warnings;
};


struct NLCLSubgroupExtension : public NLCLExtension
{
    std::shared_ptr<SubgroupProvider> DefaultProvider;
    common::mlog::MiniLogger<false>& Logger;

    NLCLSubgroupExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) : 
        NLCLExtension(context), DefaultProvider(Generate(logger, context, {}, {})), Logger(logger)
    { }
    ~NLCLSubgroupExtension() override { }
    [[nodiscard]] std::optional<xziar::nailang::Arg> NLCLFunc(NLCLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall> metas, const xziar::nailang::NailangRuntimeBase::FuncTargetType target) override;

    static SubgroupCapbility GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr);
    static std::shared_ptr<SubgroupProvider> Generate(common::mlog::MiniLogger<false>& logger, NLCLContext& context, std::u32string_view mimic, std::u32string_view args);
    inline static uint32_t ID = NLCLExtension::RegisterNLCLExtenstion(
        [](common::mlog::MiniLogger<false>& logger, NLCLContext& context, const xziar::nailang::Block&) -> std::unique_ptr<NLCLExtension>
        {
            return std::make_unique<NLCLSubgroupExtension>(logger, context);
        });
};


struct KernelSubgroupExtension : public KernelExtension
{
    KernelContext& Kernel;
    std::shared_ptr<SubgroupProvider> Provider;
    uint32_t SubgroupSize = 0;

    KernelSubgroupExtension(NLCLContext& context, KernelContext& kernel) :
        KernelExtension(context), Kernel(kernel),
        Provider(Context.GetNLCLExt<NLCLSubgroupExtension>()->DefaultProvider)
    { }
    ~KernelSubgroupExtension() override { }

    bool KernelMeta(NLCLRuntime& runtime, const xziar::nailang::FuncCall& meta, KernelContext& kernel) override;
    std::u32string ReplaceFunc(NLCLRuntime& runtime, const std::u32string_view func, const common::span<const std::u32string_view> args) override;
    void FinishKernel(NLCLRuntime& runtime, KernelContext& kernel) override;

    inline static uint32_t ID = NLCLExtension::RegisterKernelExtenstion(
        [](auto&, NLCLContext& context, KernelContext& kernel) -> std::unique_ptr<KernelExtension>
        {
            return std::make_unique<KernelSubgroupExtension>(context, kernel);
        });
};


class NLCLSubgroupKHR : public SubgroupProvider
{
protected:
    bool EnableSubgroup = false, EnableFP16 = false, EnableFP64 = false;
    void OnFinish(NLCLRuntime_& runtime, const KernelSubgroupExtension& ext, KernelContext& kernel) override;
public:
    using SubgroupProvider::SubgroupProvider;
    ~NLCLSubgroupKHR() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupAll(const std::u32string_view predicate) override;
    std::u32string SubgroupAny(const std::u32string_view predicate) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupSum(common::simd::VecDataInfo vtype, const std::u32string_view ele) override;
};

class NLCLSubgroupIntel : public NLCLSubgroupKHR
{
protected:
    bool EnableSubgroupIntel = false, EnableSubgroup16Intel = false, EnableSubgroup8Intel = false;
    [[nodiscard]] static std::u32string VectorPatch(const std::u32string_view funcName, const std::u32string_view baseFunc,
        common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid, 
        const std::u32string_view extraArg = {}, const std::u32string_view extraParam = {}) noexcept;
    [[nodiscard]] forceinline static std::u32string VectorPatchBcastShuf(const std::u32string_view funcName, 
        const std::u32string_view baseFunc, common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid) noexcept
    {
        return VectorPatch(funcName, baseFunc, vtype, mid, U", sgId", U", const uint sgId");
    }
    std::u32string DirectShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx);
    void OnFinish(NLCLRuntime_& runtime, const KernelSubgroupExtension& ext, KernelContext& kernel) override;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupIntel() override { }

    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupLocal : public NLCLSubgroupKHR
{
protected:
    bool NeedCommonInfo = false, NeedLocalTemp = false;
    void OnFinish(NLCLRuntime_& runtime, const KernelSubgroupExtension& ext, KernelContext& kernel) override;
    std::u32string BroadcastPatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string ShufflePatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupLocal() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupAll(const std::u32string_view predicate) override;
    std::u32string SubgroupAny(const std::u32string_view predicate) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupPtx : public SubgroupProvider
{
protected:
    std::u32string_view ExtraSync, ExtraMask;
    const uint32_t SMVersion;
    std::u32string Shuffle64Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string Shuffle32Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    NLCLSubgroupPtx(common::mlog::MiniLogger<false>& logger, NLCLContext& context, SubgroupCapbility cap, const uint32_t smVersion = 30);
    ~NLCLSubgroupPtx() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupAll(const std::u32string_view predicate) override;
    std::u32string SubgroupAny(const std::u32string_view predicate) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupSum(common::simd::VecDataInfo vtype, const std::u32string_view ele) override;

};



}