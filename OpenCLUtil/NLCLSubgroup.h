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
    [[nodiscard]] static std::optional<common::simd::VecDataInfo> DecideVtype(common::simd::VecDataInfo vtype, 
        common::span<const common::simd::VecDataInfo> scalars) noexcept;
    [[nodiscard]] static std::u32string ScalarPatch(const std::u32string_view funcName, const std::u32string_view base, common::simd::VecDataInfo vtype) noexcept;
    
    NLCLRuntime_& Runtime;
    NLCLContext& Context;
    const SubgroupCapbility Cap;
    SubgroupProvider(NLCLRuntime_& runtime, SubgroupCapbility cap);
    virtual ~SubgroupProvider() { }
    virtual std::u32string GetSubgroupLocalId() { return {}; };
    virtual std::u32string GetSubgroupSize(const uint32_t) { return {}; };
    virtual std::u32string SubgroupBroadcast(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupShuffle(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual void OnFinish(const KernelSubgroupExtension&, KernelContext&) { }
};


struct NLCLSubgroupExtension : public NLCLExtension
{
    NLCLRuntime_& Runtime;
    std::shared_ptr<SubgroupProvider> DefaultProvider;

    NLCLSubgroupExtension(NLCLRuntime& runtime) : NLCLExtension(runtime), Runtime(static_cast<NLCLRuntime_&>(runtime)),
        DefaultProvider(Generate(Runtime, {}, {})) { }
    ~NLCLSubgroupExtension() override { }
    [[nodiscard]] std::optional<xziar::nailang::Arg> NLCLFunc(const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall> metas, const xziar::nailang::NailangRuntimeBase::FuncTargetType target) override;

    static SubgroupCapbility GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr);
    static std::shared_ptr<SubgroupProvider> Generate(NLCLRuntime_& runtime, std::u32string_view mimic, std::u32string_view args);
    inline static uint32_t ID = NLCLExtension::RegisterNLCLExtenstion(
        [](NLCLRuntime& runtime, const xziar::nailang::Block&) -> std::unique_ptr<NLCLExtension>
        {
            return std::make_unique<NLCLSubgroupExtension>(runtime);
        });
};


struct KernelSubgroupExtension : public ReplaceExtension
{
    NLCLRuntime_& Runtime;
    KernelContext& Kernel;
    std::shared_ptr<SubgroupProvider> Provider;
    uint32_t SubgroupSize = 0;
    KernelSubgroupExtension(NLCLRuntime& runtime, KernelContext& kernel) :
        ReplaceExtension(runtime), Runtime(static_cast<NLCLRuntime_&>(runtime)), Kernel(kernel),
        Provider(Context.GetNLCLExt<NLCLSubgroupExtension>(NLCLSubgroupExtension::ID).DefaultProvider)
    { }
    ~KernelSubgroupExtension() override { }

    void ThrowByReplacerArgCount(const std::u32string_view func, const common::span<const std::u32string_view> args,
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const; 

    bool KernelMeta(const xziar::nailang::FuncCall& meta, KernelContext& kernel) override;
    std::u32string ReplaceFunc(const std::u32string_view func, const common::span<const std::u32string_view> args) override;
    void FinishKernel(KernelContext& kernel) override;

    inline static uint32_t ID = NLCLExtension::RegisterKernelExtenstion(
        [](NLCLRuntime& runtime, KernelContext& kernel) -> std::unique_ptr<ReplaceExtension>
        {
            return std::make_unique<KernelSubgroupExtension>(runtime, kernel);
        });
};


class NLCLSubgroupKHR : public SubgroupProvider
{
protected:
    bool EnableSubgroup = false, EnableFP16 = false, EnableFP64 = false;
    void OnFinish(const KernelSubgroupExtension& ext, KernelContext& kernel) override;
public:
    using SubgroupProvider::SubgroupProvider;
    ~NLCLSubgroupKHR() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupIntel : public NLCLSubgroupKHR
{
protected:
    bool EnableSubgroupIntel = false, EnableSubgroup16Intel = false, EnableSubgroup8Intel = false;
    [[nodiscard]] std::u32string VectorPatch(const std::u32string_view funcName, const std::u32string_view base, 
        common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid) noexcept;
    std::u32string DirectShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx);
    void OnFinish(const KernelSubgroupExtension& ext, KernelContext& kernel) override;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupIntel() override { }

    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupLocal : public NLCLSubgroupKHR
{
protected:
    bool NeedLocalTemp = false;
    void CheckSubgroupSize(const size_t sgSize);
    void OnFinish(const KernelSubgroupExtension& ext, KernelContext& kernel) override;
    std::u32string BroadcastPatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string ShufflePatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupLocal() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupPtx : public SubgroupProvider
{
protected:
    std::u32string_view ShufFunc, ExtraMask;
    std::u32string Shuffle64Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string Shuffle32Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    NLCLSubgroupPtx(NLCLRuntime_& runtime, SubgroupCapbility cap, const uint32_t smVersion = 30);
    ~NLCLSubgroupPtx() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize(const uint32_t sgSize) override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};



}