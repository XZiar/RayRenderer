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
enum class SubgroupReduceOp  : uint8_t { Add, Mul, Min, Max, And, Or, Xor };
enum class SubgroupShuffleOp : uint8_t { Broadcast, Shuffle, ShuffleXor, ShuffleDown, ShuffleUp };


struct NLCLSubgroupCapbility : public SubgroupCapbility
{
    bool SupportFP16            :1;
    bool SupportFP64            :1;
    bool SupportBasicSubgroup   :1;
    explicit constexpr NLCLSubgroupCapbility() noexcept : SupportFP16(false), SupportFP64(false), SupportBasicSubgroup(false) {}
    explicit constexpr NLCLSubgroupCapbility(NLCLContext& context) noexcept : SubgroupCapbility(context.SubgroupCaps),
        SupportFP16(context.SupportFP16), SupportFP64(context.SupportFP64), SupportBasicSubgroup(false) {}
};

struct NLCLSubgroupExtension : public NLCLExtension
{
private:
    template<typename T, typename... Args>
    static std::shared_ptr<SubgroupProvider> GenerateProvider(Args&&... args);
public:
    std::shared_ptr<SubgroupProvider> DefaultProvider;
    common::mlog::MiniLogger<false>& Logger;
    std::shared_ptr<SubgroupProvider> Provider;
    uint32_t SubgroupSize = 0;

    NLCLSubgroupExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) : 
        NLCLExtension(context), DefaultProvider(Generate(logger, context, {}, {})), Logger(logger)
    { }
    ~NLCLSubgroupExtension() override { }
    
    void  BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) final;
    void FinishInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) final;
    void InstanceMeta(xcomp::XCNLExecutor& executor, const xziar::nailang::FuncPack& meta, xcomp::InstanceContext& ctx) final;

    [[nodiscard]] xcomp::ReplaceResult ReplaceFunc(xcomp::XCNLRawExecutor& executor, std::u32string_view func,
        common::span<const std::u32string_view> args) final;
    [[nodiscard]] std::optional<xziar::nailang::Arg> ConfigFunc(xcomp::XCNLExecutor& executor, xziar::nailang::FuncEvalPack& call) final;

    static NLCLSubgroupCapbility GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr);
    static std::shared_ptr<SubgroupProvider> Generate(common::mlog::MiniLogger<false>& logger, NLCLContext& context, std::u32string_view mimic, std::u32string_view args);
    
    XCNL_EXT_REG(NLCLSubgroupExtension,
    {
        if(const auto ctx = dynamic_cast<NLCLContext*>(&context); ctx)
            return std::make_unique<NLCLSubgroupExtension>(logger, *ctx);
        return {};
    });
};

struct ExtraParam
{
    std::u32string Param;
    std::u32string Arg;
    explicit operator bool() const noexcept
    {
        return !Arg.empty();
    }
};

struct SubgroupProvider
{
    friend NLCLSubgroupExtension;
public:
    struct TypedAlgoResult
    {
        static constexpr common::simd::VecDataInfo Dummy{ common::simd::VecDataInfo::DataTypes::Empty, 0, 0, 0 };
        common::str::StrVariant<char32_t> Func;
        ExtraParam Extra;
        std::u32string_view AlgorithmSuffix;
        uint8_t Features;
        common::simd::VecDataInfo SrcType;
        common::simd::VecDataInfo DstType;
        bool PatchedFunc;
        TypedAlgoResult() noexcept : Features(0), SrcType(Dummy), DstType(Dummy), PatchedFunc(false) { }
        TypedAlgoResult(common::str::StrVariant<char32_t> func, const std::u32string_view algoSfx, const uint8_t features,
            const common::simd::VecDataInfo src, const common::simd::VecDataInfo dst, bool patchedFunc) noexcept :
            Func(std::move(func)), AlgorithmSuffix(algoSfx), Features(features), SrcType(src), DstType(dst), PatchedFunc(patchedFunc)
        { }
        TypedAlgoResult(common::str::StrVariant<char32_t> func, const std::u32string_view algoSfx, const uint8_t features, 
            const common::simd::VecDataInfo type, bool patchedFunc) noexcept :
            Func(std::move(func)), AlgorithmSuffix(algoSfx), Features(features), SrcType(type), DstType(type), PatchedFunc(patchedFunc)
        { }
        std::u32string_view GetFuncName() const noexcept
        {
            return Func.StrView();
        }
        std::u32string_view GetDepend() const noexcept
        {
            if (PatchedFunc)
                return Func.StrView();
            else
                return {};
        }
        constexpr explicit operator bool() const noexcept
        {
            return !Func.empty();
        }
    };
protected:
    struct Algorithm
    {
        struct AlgoID
        {
            uint8_t Raw;
            constexpr AlgoID(uint8_t level, uint8_t id) noexcept :
                Raw(static_cast<uint8_t>(((id & 0x10) << 3) | ((level & 0x7) << 4) | (id & 0x0f)))
            { }
            constexpr uint8_t ID() const noexcept { return static_cast<uint32_t>(((Raw & 0x80) >> 3) | (Raw & 0xf)); }
            constexpr uint8_t Level() const noexcept { return static_cast<uint32_t>((Raw & 0x70) >> 4); }
        };
        struct DTypeSupport
        {
            xcomp::VecDimSupport Int = xcomp::VTypeDimSupport::Empty;
            xcomp::VecDimSupport FP  = xcomp::VTypeDimSupport::Empty;
            constexpr void Set(xcomp::VecDimSupport support) noexcept
            {
                Int = FP = support;
            }
            constexpr void Set(xcomp::VecDimSupport supInt, xcomp::VecDimSupport supFp) noexcept
            {
                Int = supInt; FP = supFp;
            }
        };
        AlgoID ID;
        uint8_t Features;
        DTypeSupport Bit64;
        DTypeSupport Bit32;
        DTypeSupport Bit16;
        DTypeSupport Bit8;
        constexpr Algorithm(AlgoID id, uint8_t feats) noexcept : ID(id), Features(feats) {}
        constexpr Algorithm(uint8_t level, uint8_t id, uint8_t feats) noexcept : ID(level, id), Features(feats) {}
    };
    struct AlgoChecker
    {
        uint8_t Level;
        uint8_t Feature;
        template<typename T>
        constexpr AlgoChecker(uint8_t level, T feature) noexcept : 
            Level(level), Feature(static_cast<uint8_t>(feature)) { }
        constexpr bool Check(const Algorithm& algo) const noexcept
        {
            return ((algo.Features & Feature) == Feature) && (Level == 0 || Level == algo.ID.Level());
        }
    };
    [[nodiscard]] static std::u32string GenerateFuncName(std::u32string_view op, std::u32string_view suffix, 
        common::simd::VecDataInfo vtype) noexcept;
    [[nodiscard]] static std::optional<common::simd::VecDataInfo> DecideVtype(common::simd::VecDataInfo vtype,
        common::span<const common::simd::VecDataInfo> scalars) noexcept;
    [[nodiscard]] static std::u32string ScalarPatch(const std::u32string_view name, const std::u32string_view baseFunc, 
        common::simd::VecDataInfo vtype, common::simd::VecDataInfo castType,
        std::u32string_view pfxParam, std::u32string_view pfxArg,
        std::u32string_view sfxParam, std::u32string_view sfxArg) noexcept;
    template<typename Op, typename... Args>
    [[nodiscard]] TypedAlgoResult VectorizePatch(const common::simd::VecDataInfo vtype, TypedAlgoResult& scalarAlgo, Args&&... args);
    [[nodiscard]] TypedAlgoResult ShufReducePatch(const common::simd::VecDataInfo vtype, SubgroupReduceOp op, uint8_t level);
    [[nodiscard]] static std::u32string HiLoPatch(const std::u32string_view name, const std::u32string_view baseFunc, common::simd::VecDataInfo vtype,
        const std::u32string_view extraArg = {}, const std::u32string_view extraParam = {}) noexcept;
    struct Shuffle;
    boost::container::small_vector<Algorithm, 4> ShuffleSupport;
    struct Reduce;
    boost::container::small_vector<Algorithm, 4> ReduceSupport;

    common::mlog::MiniLogger<false>& Logger;
    NLCLContext& Context;
    const NLCLSubgroupCapbility Cap;
    bool EnableFP16 = false, EnableFP64 = false;
    void EnableVecType(common::simd::VecDataInfo type) noexcept;
    void AddWarning(common::str::StrVariant<char16_t> msg);
    /**
     * @brief return funcname and suffix for the given dtype
     * @param algo reference of algorithm
     * @param vtype datatype
     * @param op shuffle op
     * @return TypedAlgoResult
    */
    virtual TypedAlgoResult HandleShuffleAlgo(const Algorithm&, common::simd::VecDataInfo, SubgroupShuffleOp) { return {}; }
    /**
     * @brief return funcname and suffix for the given dtype
     * @param algo reference of algorithm
     * @param vtype datatype
     * @param op shuffle op
     * @return TypedAlgoResult
    */
    virtual TypedAlgoResult HandleReduceAlgo(const Algorithm&, common::simd::VecDataInfo, SubgroupReduceOp) { return {}; }
    virtual std::u32string_view GetAlgoSuffix([[maybe_unused]] uint8_t level) noexcept { return {}; }
private:
    std::vector<std::u16string> Warnings;
    void Prepare();
    template<typename Op, bool ExactBit, typename... Args>
    TypedAlgoResult HandleSubgroupOperation(common::simd::VecDataInfo vtype, AlgoChecker algoCheck, Args&&... args);
    TypedAlgoResult SubgroupShuffle(common::simd::VecDataInfo vtype, SubgroupShuffleOp op, bool nonUniform, uint8_t level = 0);
    TypedAlgoResult SubgroupReduce (common::simd::VecDataInfo vtype, SubgroupReduceOp  op, bool nonUniform, uint8_t level = 0);
public:
    SubgroupProvider(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap);
    virtual ~SubgroupProvider() { }
    virtual xcomp::ReplaceResult GetSubgroupSize() { return {}; };
    virtual xcomp::ReplaceResult GetMaxSubgroupSize() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupCount() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupId() { return {}; };
    virtual xcomp::ReplaceResult GetSubgroupLocalId() { return {}; };
    virtual xcomp::ReplaceResult SubgroupAll(const std::u32string_view) { return {}; };
    virtual xcomp::ReplaceResult SubgroupAny(const std::u32string_view) { return {}; };
    xcomp::ReplaceResult SubgroupShuffle(common::simd::VecDataInfo vtype, xcomp::U32StrSpan args, SubgroupShuffleOp op, bool nonUniform);
    xcomp::ReplaceResult SubgroupReduce (common::simd::VecDataInfo vtype, xcomp::U32StrSpan args, SubgroupReduceOp  op, bool nonUniform);
    virtual void OnBegin(NLCLRuntime&, const NLCLSubgroupExtension&, KernelContext&) {}
    virtual void OnFinish(NLCLRuntime&, const NLCLSubgroupExtension&, KernelContext&);
};


class NLCLSubgroupKHR : public SubgroupProvider
{
protected:
    static constexpr uint8_t LevelKHR           = 0x01;
    static constexpr uint8_t AlgoKHRBroadcast   = 0x01;
    static constexpr uint8_t AlgoKHRBroadcastNU = 0x02;
    static constexpr uint8_t AlgoKHRShuffle     = 0x03;
    static constexpr uint8_t AlgoKHRReduce      = 0x01;
    static constexpr uint8_t AlgoKHRReduceNU    = 0x02;
    static constexpr uint8_t AlgoKHRReduceShuf  = 0x10;
    bool EnableKHRBasic = false, EnableKHRExtType = false, EnableKHRShuffle = false, EnableKHRShuffleRel = false, 
        EnableKHRBallot = false, EnableKHRNUArith = false;
    void OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    void WarnFP(common::simd::VecDataInfo vtype, const std::u16string_view func);
    // internal use, won't check type
    //xcomp::ReplaceResult SubgroupReduceArith(SubgroupReduceOp op, common::simd::VecDataInfo vtype, common::simd::VecDataInfo realType, const std::u32string_view ele);
    TypedAlgoResult HandleShuffleAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupShuffleOp op) override;
    TypedAlgoResult HandleReduceAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupReduceOp op) override;
    std::u32string_view GetAlgoSuffix(uint8_t level) noexcept override;
public:
    NLCLSubgroupKHR(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap);
    ~NLCLSubgroupKHR() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;
    //xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;
};

class NLCLSubgroupIntel : public NLCLSubgroupKHR
{
private:
    /**
     * @brief check if need specific extention
     * @param type vector type
    */
    void EnableExtraVecType(common::simd::VecDataInfo type) noexcept;
protected:
    static constexpr uint8_t LevelIntel          = 0x02;
    static constexpr uint8_t AlgoIntelShuffle    = 0x04;
    static constexpr uint8_t AlgoIntelReduceShuf = 0x14;
    bool EnableIntel = false, EnableIntel16 = false, EnableIntel8 = false;
    void OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    /*xcomp::ReplaceResult SubgroupReduceArith(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
    xcomp::ReplaceResult SubgroupReduceBitwise(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);*/
    TypedAlgoResult HandleShuffleAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupShuffleOp op) override;
    TypedAlgoResult HandleReduceAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupReduceOp op) override;
    std::u32string_view GetAlgoSuffix(uint8_t level) noexcept override;
public:
    NLCLSubgroupIntel(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap);
    ~NLCLSubgroupIntel() override { }

    //xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;

};

class NLCLSubgroupLocal : public NLCLSubgroupKHR
{
protected:
    static constexpr uint8_t LevelLocal         = 0x02;
    static constexpr uint8_t AlgoLocalBroadcast = 0x04;
    static constexpr uint8_t AlgoLocalShuffle   = 0x05;
    std::u32string_view KernelName;
    bool NeedSubgroupSize = false, NeedLocalTemp = false;
    void OnBegin(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    void OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel) override;
    std::u32string GenerateKID(std::u32string_view type) const noexcept;
    TypedAlgoResult BroadcastPatch (const common::simd::VecDataInfo vtype) noexcept;
    TypedAlgoResult ShufflePatch   (const common::simd::VecDataInfo vtype) noexcept;
    TypedAlgoResult ShuffleXorPatch(const common::simd::VecDataInfo vtype) noexcept;
    TypedAlgoResult HandleShuffleAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupShuffleOp op) override;
    std::u32string_view GetAlgoSuffix(uint8_t level) noexcept override;
public:
    NLCLSubgroupLocal(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap);
    ~NLCLSubgroupLocal() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;

};

class NLCLSubgroupPtx : public SubgroupProvider
{
protected:
    static constexpr uint8_t LevelPtx          = 0x02;
    static constexpr uint8_t AlgoPtxShuffle    = 0x04;
    static constexpr uint8_t AlgoPtxReduceShuf = 0x14;
    std::u32string_view ExtraSync, ExtraMask;
    const uint32_t SMVersion;
    TypedAlgoResult ShufflePatch(common::simd::VecDataInfo vtype, SubgroupShuffleOp op) noexcept;
    TypedAlgoResult SubgroupReduceSM80(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
    TypedAlgoResult SubgroupReduceSM30(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele);
    TypedAlgoResult HandleShuffleAlgo(const Algorithm& algo, common::simd::VecDataInfo vtype, SubgroupShuffleOp op) override;
    std::u32string_view GetAlgoSuffix(uint8_t level) noexcept override;
public:
    NLCLSubgroupPtx(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap, const uint32_t smVersion = 30);
    ~NLCLSubgroupPtx() override { }

    xcomp::ReplaceResult GetSubgroupSize() override;
    xcomp::ReplaceResult GetMaxSubgroupSize() override;
    xcomp::ReplaceResult GetSubgroupCount() override;
    xcomp::ReplaceResult GetSubgroupId() override;
    xcomp::ReplaceResult GetSubgroupLocalId() override;
    xcomp::ReplaceResult SubgroupAll(const std::u32string_view predicate) override;
    xcomp::ReplaceResult SubgroupAny(const std::u32string_view predicate) override;
    //xcomp::ReplaceResult SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele) override;

};



}