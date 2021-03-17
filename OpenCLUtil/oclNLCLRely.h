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

class NLCLExecutor;
class NLCLRawExecutor;
class NLCLRuntime;
class NLCLContext;
class KernelExtension;
class NLCLProcessor;
class NLCLProgStub;


struct KernelContext : public xcomp::InstanceContext
{
    friend NLCLRawExecutor;
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
    forceinline void SetDebug(uint32_t size, std::shared_ptr<xcomp::debug::InfoProvider> infoProv)
    {
        Args.HasDebug = true;
        Args.DebugBuffer = std::max(Args.DebugBuffer, size);
        Args.InfoProv = std::move(infoProv);
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


class OCLUAPI NLCLExtension : public xcomp::XCNLExtension
{
    friend NLCLRuntime;
    friend NLCLProgStub;
    friend NLCLContext;
protected:
    NLCLContext& Context;
public:
    NLCLExtension(NLCLContext& context);
    ~NLCLExtension() override;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLContext : public xcomp::XCNLContext
{
    friend NLCLExecutor;
    friend NLCLRawExecutor;
    friend NLCLProcessor;
    friend NLCLRuntime;
    friend NLCLProgStub;
private:
    struct OCLUVar;
    xziar::nailang::Arg OCLUArg;
public:
    const oclDevice Device;
    const bool SupportFP16, SupportFP64, SupportNVUnroll,
        SupportSubgroupKHR, SupportSubgroupIntel, SupportSubgroup8Intel, SupportSubgroup16Intel, SupportBasicSubgroup;
    NLCLContext(oclDevice dev, const common::CLikeDefines& info);
    ~NLCLContext() override;
    [[nodiscard]] xziar::nailang::ArgLocator LocateArg(const xziar::nailang::LateBindVar& var, bool create) noexcept override;
    [[nodiscard]] VecTypeResult ParseVecType(const std::u32string_view type) const noexcept override;
    [[nodiscard]] std::u32string_view GetVecTypeName(common::simd::VecDataInfo info) const noexcept override;
    [[nodiscard]] static std::u32string_view GetCLTypeName(common::simd::VecDataInfo info) noexcept;
protected:
    std::vector<bool> EnabledExtensions;
    std::vector<std::string> CompilerFlags;
    std::vector<std::pair<std::string, KernelArgStore>> CompiledKernels;
    bool EnableUnroll, AllowDebug;
};


class OCLUAPI NLCLExecutor : public xcomp::XCNLExecutor
{
    friend NLCLRuntime;
    friend NLCLRawExecutor;
private:
    NLCLExecutor(NLCLRuntime* runtime);
protected:
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
public:
    constexpr NLCLRuntime& GetRuntime() const noexcept;
};


class OCLUAPI NLCLRawExecutor : public xcomp::XCNLRawExecutor
{
    friend NLCLRuntime;
private:
    NLCLRawExecutor(NLCLExecutor& executor);
protected:
    void StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg) const;
    [[nodiscard]] bool HandleMetaFunc(xziar::nailang::MetaEvalPack& meta) override;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) final;
    [[nodiscard]] std::unique_ptr<xcomp::InstanceContext> PrepareInstance(const xcomp::OutputBlock& block) final;
    void ProcessStruct(const xcomp::OutputBlock& block, std::u32string& dst) final;
    void OutputInstance(const xcomp::OutputBlock& block, std::u32string& dst) final;
public:
    constexpr NLCLRuntime& GetRuntime() const noexcept { return static_cast<NLCLExecutor&>(Executor).GetRuntime(); }
    using XCNLRawExecutor::ExtensionReplaceFunc;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLRuntime : public xcomp::XCNLRuntime
{
    friend NLCLExecutor;
    friend NLCLRawExecutor;
    friend KernelExtension;
    friend NLCLProcessor;
private:
    xcomp::XCNLExecutor& GetBaseExecutor() noexcept final;
    xcomp::XCNLRawExecutor& GetRawExecutor() noexcept final;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLCLContext& Context;
    NLCLExecutor BaseExecutor;
    NLCLRawExecutor RawExecutor;

    [[nodiscard]] xcomp::OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept override;
    void HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const xziar::nailang::FuncPack& meta, const xziar::nailang::Arg*) final;
    void BeforeFinishOutput(std::u32string& prefix, std::u32string& content) override;
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
    using XCNLRuntime::ParseVecType;
    using XCNLRuntime::GetVecTypeName;
};

inline constexpr NLCLRuntime& NLCLExecutor::GetRuntime() const noexcept
{
    return static_cast<NLCLRuntime&>(XCNLExecutor::GetRuntime());
}


class OCLUAPI NLCLBaseResult : public NLCLResult
{
protected:
    mutable xziar::nailang::NailangBasicRuntime Runtime;
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