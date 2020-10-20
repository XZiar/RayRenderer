#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "oclKernelDebug.h"
#include "XComputeBase/XCompNailang.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"

namespace oclu
{
#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


struct KernelContext : public xcomp::InstanceContext
{
    friend NLCLRuntime;
    friend debug::NLCLDebugExtension;
    ~KernelContext() override {}

    template<typename... Ts>
    forceinline void AddArg(Ts&&... args)
    {
        Args.AddArg(std::forward<Ts>(args)...);
    }
    template<typename... Ts>
    forceinline void AddTailArg(Ts&&... args)
    {
        TailArgs.AddArg(std::forward<Ts>(args)...);
    }
    forceinline void SetDebugBuffer(uint32_t size)
    {
        Args.HasDebug = true;
        Args.DebugBuffer = std::max(Args.DebugBuffer, size);
    }
    forceinline bool AddAttribute(const std::u32string_view id, std::u32string_view content)
    {
        return Add(Attributes, id, content, {});
    }

    forceinline constexpr uint32_t GetWorkgroupSize() const noexcept { return WorkgroupSize; }
protected:
    std::vector<NamedText> Attributes;
    KernelArgStore Args;
    KernelArgStore TailArgs; // args that won't be recorded
    uint32_t WorkgroupSize = 0;
};


struct KernelCookie : public xcomp::BlockCookie
{
    KernelContext Context;
    KernelCookie(const xcomp::OutputBlock& block) noexcept : xcomp::BlockCookie(block) { }
    ~KernelCookie() override { }
    xcomp::InstanceContext* GetInstanceCtx() noexcept override { return &Context; }
};


class OCLUAPI NLCLExtension : public xcomp::XCNLExtension
{
    friend class NLCLRuntime;
    friend class NLCLProgStub;
    friend class NLCLContext;
protected:
    NLCLContext& Context;
public:
    NLCLExtension(NLCLContext& context);
    ~NLCLExtension() override;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLContext : public xcomp::XCNLContext
{
    friend class NLCLProcessor;
    friend class NLCLRuntime;
    friend class NLCLProgStub;
protected:
    xziar::nailang::Arg LookUpCLArg(const xziar::nailang::LateBindVar& var) const;
public:
    const oclDevice Device;
    const bool SupportFP16, SupportFP64, SupportNVUnroll,
        SupportSubgroupKHR, SupportSubgroupIntel, SupportSubgroup8Intel, SupportSubgroup16Intel, SupportBasicSubgroup;
    NLCLContext(oclDevice dev, const common::CLikeDefines& info);
    ~NLCLContext() override;
    [[nodiscard]] xziar::nailang::Arg LookUpArg(const xziar::nailang::LateBindVar& var) const override;
    [[nodiscard]] VecTypeResult ParseVecType(const std::u32string_view type) const noexcept override;
    [[nodiscard]] std::u32string_view GetVecTypeName(common::simd::VecDataInfo info) const noexcept override;
    [[nodiscard]] static std::u32string_view GetCLTypeName(common::simd::VecDataInfo info) noexcept;
protected:
    std::vector<bool> EnabledExtensions;
    std::vector<std::string> CompilerFlags;
    std::vector<std::pair<std::string, KernelArgStore>> CompiledKernels;
    bool EnableUnroll, AllowDebug;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLRuntime : public xcomp::XCNLRuntime
{
    friend class KernelExtension;
    friend class NLCLProcessor;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLCLContext& Context;

    void StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg);

    void OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args) override;

    [[nodiscard]] xziar::nailang::Arg LookUpArg(const xziar::nailang::LateBindVar& var) const override;
    xziar::nailang::Arg EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas) override;
    
    [[nodiscard]] xcomp::OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept override;
    [[nodiscard]] std::unique_ptr<xcomp::BlockCookie> PrepareInstance(const xcomp::OutputBlock& block) override;
    void HandleInstanceMeta(const FuncCall& meta, xcomp::InstanceContext& ctx) override;
    void OutputStruct  (xcomp::BlockCookie& cookie, std::u32string& dst) override;
    void OutputInstance(xcomp::BlockCookie& cookie, std::u32string& dst) override;
    void BeforeFinishOutput(std::u32string& prefix, std::u32string& content) override;
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
};


class OCLUAPI NLCLBaseResult : public NLCLResult
{
protected:
    std::shared_ptr<NLCLContext> Context;
public:
    NLCLBaseResult(const std::shared_ptr<NLCLContext>& context);
    ~NLCLBaseResult() override;
    NLCLResult::ResultType QueryResult(std::u32string_view name) const override;
};

class OCLUAPI NLCLUnBuildResult : public NLCLBaseResult
{
protected:
    std::string Source;
public:
    NLCLUnBuildResult(const std::shared_ptr<NLCLContext>& context, std::string&& source);
    ~NLCLUnBuildResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuiltResult : public NLCLBaseResult
{
protected:
    oclProgram Prog;
public:
    NLCLBuiltResult(const std::shared_ptr<NLCLContext>& context, oclProgram prog);
    ~NLCLBuiltResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuildFailResult : public NLCLUnBuildResult
{
protected:
    std::shared_ptr<common::ExceptionBasicInfo> Exception;
public:
    NLCLBuildFailResult(const std::shared_ptr<NLCLContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex);
    ~NLCLBuildFailResult() override;
    oclProgram GetProgram() const override;
};


class OCLUAPI NLCLProgStub : public xcomp::XCNLProgStub
{
    friend class NLCLProcessor;
private:
    void Prepare();
public:
    NLCLProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLCLContext> context, common::mlog::MiniLogger<false>& logger);
    NLCLProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLCLContext> context, std::unique_ptr<NLCLRuntime>&& runtime);

    std::shared_ptr<NLCLContext> GetContext() const noexcept
    {
        return std::static_pointer_cast<NLCLContext>(Context);
    }
    NLCLRuntime& GetRuntime() const noexcept
    {
        return *static_cast<NLCLRuntime*>(Runtime.get());
    }
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}