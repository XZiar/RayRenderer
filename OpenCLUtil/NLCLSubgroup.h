#pragma once
#include "oclNLCLRely.h"
#include "common/StrParsePack.hpp"

namespace oclu
{
struct SubgroupProvider;
struct NLCLSubgroupExtension;

struct SubgroupAttributes
{
    enum class MimicType { None, Auto, Local, Ptx };
    std::string Args;
    MimicType Mimic = MimicType::None;
};
enum class SubgroupReduceOp { Sum, Min, Max, And, Or, Xor };


struct NLCLRuntime_ : public NLCLRuntime
{
    friend SubgroupProvider;
    friend NLCLSubgroupExtension;
    friend class NLCLSubgroupLocal;
    friend class NLCLSubgroupPtx;
};

struct SubgroupCapbility
{
    bool SupportSubgroupKHR = false, SupportSubgroupIntel = false, SupportSubgroup8Intel = false, SupportSubgroup16Intel = false,
        SupportFP16 = false, SupportFP64 = false,
        SupportBasicSubgroup = false;
};

struct NLCLSubgroupExtension : public NLCLExtension
{
    std::shared_ptr<SubgroupProvider> DefaultProvider;
    common::mlog::MiniLogger<false>& Logger;
    std::shared_ptr<SubgroupProvider> Provider;
    uint32_t SubgroupSize = 0;

    NLCLSubgroupExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) : 
        NLCLExtension(context), DefaultProvider(Generate(logger, context, {}, {})), Logger(logger)
    { }
    ~NLCLSubgroupExtension() override { }
    
    void  BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) override;
    void FinishInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) override;
    void InstanceMeta(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& meta, xcomp::InstanceContext& ctx) override;

    [[nodiscard]] xcomp::ReplaceResult ReplaceFunc(xcomp::XCNLRuntime& runtime, std::u32string_view func, 
        const common::span<const std::u32string_view> args) override;
    [[nodiscard]] std::optional<xziar::nailang::Arg> XCNLFunc(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall>) override;

    static SubgroupCapbility GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr);
    static std::shared_ptr<SubgroupProvider> Generate(common::mlog::MiniLogger<false>& logger, NLCLContext& context, std::u32string_view mimic, std::u32string_view args);
    
    XCNL_EXT_REG(NLCLSubgroupExtension,
    {
        if(const auto ctx = dynamic_cast<NLCLContext*>(&context); ctx)
            return std::make_unique<NLCLSubgroupExtension>(logger, *ctx);
        return {};
    });
};


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
    [[nodiscard]] static std::u32string HiLoPatch(const std::u32string_view name, const std::u32string_view baseFunc, common::simd::VecDataInfo vtype,
        const std::u32string_view extraArg = {}, const std::u32string_view extraParam = {}) noexcept;

    NLCLContext& Context;
    const SubgroupCapbility Cap;
    void AddWarning(common::str::StrVariant<char16_t> msg);
public:
    SubgroupProvider(NLCLContext& context, SubgroupCapbility cap);
    virtual ~SubgroupProvider() { }
    virtual xcomp::ReplaceResult GetSubgroupSize() { return {}; };
    virtual xcomp::ReplaceResult GetMaxSubgroupSize() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupCount() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupId() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupLocalId() { return {}; };
    virtual xcomp::ReplaceResult SubgroupAll(const std::u32string_view) { return {}; };
    virtual xcomp::ReplaceResult SubgroupAny(const std::u32string_view) { return {}; };
    virtual xcomp::ReplaceResult SubgroupBroadcast(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual xcomp::ReplaceResult SubgroupShuffle(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp, common::simd::VecDataInfo, const std::u32string_view) { return {}; };
    virtual void OnBegin(NLCLRuntime_&, const NLCLSubgroupExtension&, KernelContext&) {}
    virtual void OnFinish(NLCLRuntime_&, const NLCLSubgroupExtension&, KernelContext&);
private:
    std::vector<std::u16string> Warnings;
};


class NLCLSubgroupKHR : public SubgroupProvider
{
protected:
    bool EnableSubgroup = false, EnableFP16 = false, EnableFP64 = false;
    void OnFinish(NLCLRuntime_& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    void WarnFP(common::simd::VecDataInfo vtype, const std::u16string_view func);
    // internal use, won't check type
    xcomp::ReplaceResult SubgroupReduceArith(SubgroupReduceOp op, common::simd::VecDataInfo vtype, common::simd::VecDataInfo realType, const std::u32string_view ele);
public:
    using SubgroupProvider::SubgroupProvider;
    ~NLCLSubgroupKHR() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;
};

class NLCLSubgroupIntel : public NLCLSubgroupKHR
{
protected:
    std::vector<common::simd::VecDataInfo> CommonSupport;
    bool EnableSubgroupIntel = false, EnableSubgroup16Intel = false, EnableSubgroup8Intel = false;
    [[nodiscard]] static std::u32string VectorPatch(const std::u32string_view funcName, const std::u32string_view baseFunc,
        common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid, 
        const std::u32string_view extraArg = {}, const std::u32string_view extraParam = {}) noexcept;
    [[nodiscard]] forceinline static std::u32string VectorPatchBcastShuf(const std::u32string_view funcName, 
        const std::u32string_view baseFunc, common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid) noexcept
    {
        return VectorPatch(funcName, baseFunc, vtype, mid, U", sgId", U", const uint sgId");
    }
    void OnFinish(NLCLRuntime_& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    xcomp::ReplaceResult DirectShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx);
    xcomp::ReplaceResult SubgroupReduceArith(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
    xcomp::ReplaceResult SubgroupReduceBitwise(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
public:
    NLCLSubgroupIntel(NLCLContext& context, SubgroupCapbility cap);
    ~NLCLSubgroupIntel() override { }

    xcomp::ReplaceResult SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;

};

class NLCLSubgroupLocal : public NLCLSubgroupKHR
{
protected:
    std::u32string_view KernelName;
    bool NeedSubgroupSize = false, NeedLocalTemp = false;
    void OnBegin(NLCLRuntime_& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    void OnFinish(NLCLRuntime_& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    std::u32string BroadcastPatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string ShufflePatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupLocal() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupPtx : public SubgroupProvider
{
protected:
    std::u32string_view ExtraSync, ExtraMask;
    const uint32_t SMVersion;
    std::u32string ShufflePatch(const uint8_t bit) noexcept;
    xcomp::ReplaceResult SubgroupReduceSM80(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
    xcomp::ReplaceResult SubgroupReduceSM30(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
public:
    NLCLSubgroupPtx(common::mlog::MiniLogger<false>& logger, NLCLContext& context, SubgroupCapbility cap, const uint32_t smVersion = 30);
    ~NLCLSubgroupPtx() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;

};



}