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
    uint8_t SubgroupSize = 0;
};
constexpr auto SubgroupMimicParser = SWITCH_PACK(Hash,
    (U"local",  SubgroupAttributes::MimicType::Local),
    (U"ptx",    SubgroupAttributes::MimicType::Ptx),
    (U"auto",   SubgroupAttributes::MimicType::Auto),
    (U"none",   SubgroupAttributes::MimicType::None),
    (U"",       SubgroupAttributes::MimicType::None));

class NLCLSubgroup : public ReplaceExtension
{
    friend class NLCLRuntime;
protected:
    struct SubgroupCapbility
    {
        uint8_t SubgroupSize = 0;
        bool SupportSubgroupKHR = false, SupportSubgroupIntel = false, SupportSubgroup8Intel = false, SupportSubgroup16Intel = false,
            SupportFP16 = false, SupportFP64 = false,
            SupportBasicSubgroup = false;
    };
    NLCLRuntime& Runtime;
    NLCLContext& Context;
    const SubgroupCapbility Cap;
    std::map<std::u32string, std::u32string, std::less<>> Injects;
    KernelCookie* Cookie = nullptr;
    bool EnableSubgroup = false;

    void ThrowByReplacerArgCount(const std::u32string_view func, const common::span<const std::u32string_view> args,
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const;
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const;
    void AddArg(std::u32string id, std::u32string content);
    void AddInject(std::u32string id, std::u32string content);
    template<typename T, typename F, typename... Args>
    forceinline bool AddPatchedBlock(T& obj, std::u32string_view id, F generator, Args&&... args)
    {
        return Context.AddPatchedBlock(id, [&]() { return (obj.*generator)(std::forward<Args>(args)...); });
        //return Runtime.AddPatchedBlock(obj, id, generator, std::forward<Args>(args)...);
    }
    template<typename F>
    forceinline bool AddPatchedBlock(std::u32string_view id, F&& generator)
    {
        return Context.AddPatchedBlock(id, std::forward<F>(generator));
    }
    forceinline common::mlog::MiniLogger<false>& Logger() const noexcept
    {
        return Runtime.Logger;
    }
    [[nodiscard]] std::u32string ScalarPatch(const std::u32string_view funcName, const std::u32string_view base, common::simd::VecDataInfo vtype) noexcept;
    [[nodiscard]] std::optional<common::simd::VecDataInfo> DecideVtype(common::simd::VecDataInfo vtype, common::span<const common::simd::VecDataInfo> scalars) noexcept;
    
    virtual void OnFinish() {}
public:
    NLCLSubgroup(NLCLRuntime* runtime, SubgroupCapbility cap);
    ~NLCLSubgroup() override { }

    virtual std::u32string GetSubgroupLocalId() { return {}; };
    virtual std::u32string GetSubgroupSize() { return {}; };
    virtual std::u32string SubgroupBroadcast(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };
    virtual std::u32string SubgroupShuffle(common::simd::VecDataInfo, const std::u32string_view, const std::u32string_view) { return {}; };

    std::u32string ReplaceFunc(const std::u32string_view func, const common::span<const std::u32string_view> args, void* cookie) override;
    void Finish(void* cookie) override;

    static SubgroupCapbility GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr);
    static std::unique_ptr<NLCLSubgroup> Generate(NLCLRuntime* runtime, const SubgroupAttributes& attr);
};

class NLCLSubgroupKHR : public NLCLSubgroup
{
protected:
    bool EnableFP16 = false, EnableFP64 = false;
    void OnFinish() override;
public:
    using NLCLSubgroup::NLCLSubgroup;
    ~NLCLSubgroupKHR() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize() override;
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
    void OnFinish() override;
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
    void OnFinish() override;
    std::u32string BroadcastPatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string ShufflePatch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    using NLCLSubgroupKHR::NLCLSubgroupKHR;
    ~NLCLSubgroupLocal() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize() override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};

class NLCLSubgroupPtx : public NLCLSubgroup
{
protected:
    std::u32string_view ShufFunc, ExtraMask;
    std::u32string Shuffle64Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
    std::u32string Shuffle32Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept;
public:
    NLCLSubgroupPtx(NLCLRuntime* runtime, SubgroupCapbility cap, const uint32_t smVersion = 30);
    ~NLCLSubgroupPtx() override { }

    std::u32string GetSubgroupLocalId() override;
    std::u32string GetSubgroupSize() override;
    std::u32string SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;
    std::u32string SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx) override;

};


}