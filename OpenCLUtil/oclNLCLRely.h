#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "oclKernelDebug.h"
#include "XComputeBase/XCompNailang.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"

namespace oclu
{
#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NLCLExecutor;
class NLCLConfigurator;
class NLCLRawExecutor;
class NLCLRuntime;
class NLCLContext;
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
    forceinline static constexpr NLCLRuntime& GetRuntime(xcomp::XCNLExecutor& executor);
public:
    NLCLExtension(NLCLContext& context);
    ~NLCLExtension() override;
};


struct SubgroupCapbility
{
    bool SupportKHR             :1;
    bool SupportKHRExtType      :1;
    bool SupportKHRShuffle      :1;
    bool SupportKHRShuffleRel   :1;
    bool SupportIntel           :1;
    bool SupportIntel8          :1;
    bool SupportIntel16         :1;
    bool SupportIntel64         :1;
    constexpr SubgroupCapbility() noexcept : 
        SupportKHR(false), SupportKHRExtType(false), SupportKHRShuffle(false), SupportKHRShuffleRel(false),
        SupportIntel(false), SupportIntel8(false), SupportIntel16(false), SupportIntel64(false)
    { }
    SubgroupCapbility(oclDevice dev) noexcept;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLContext : public xcomp::XCNLContext
{
    friend NLCLExecutor;
    friend NLCLConfigurator;
    friend NLCLRawExecutor;
    friend NLCLProcessor;
    friend NLCLRuntime;
    friend NLCLProgStub;
private:
    struct OCLUVar;
    xziar::nailang::Arg OCLUArg;
public:
    const oclDevice Device;
    const bool SupportFP16, SupportFP64, SupportNVUnroll;
    const SubgroupCapbility SubgroupCaps;
    NLCLContext(oclDevice dev, const common::CLikeDefines& info);
    ~NLCLContext() override;
    [[nodiscard]] xziar::nailang::Arg LocateArg(const xziar::nailang::LateBindVar& var, bool create) noexcept override;
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
protected:
    NLCLExecutor(NLCLRuntime* runtime);
public:
    forceinline constexpr NLCLRuntime& GetRuntime() const noexcept;
};


class OCLUAPI NLCLConfigurator final : public NLCLExecutor, public xcomp::XCNLConfigurator
{
private:
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] MetaFuncResult HandleMetaFunc(const xziar::nailang::FuncCall& meta, xziar::nailang::MetaSet& allMetas) final;
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
    void EvaluateRawBlock(const xziar::nailang::RawBlock& block, xcomp::MetaFuncs metas) final;
public:
    NLCLConfigurator(NLCLRuntime* runtime);
    ~NLCLConfigurator() override;
};


class OCLUAPI NLCLRawExecutor final : public NLCLExecutor, public xcomp::XCNLRawExecutor
{
    friend NLCLRuntime;
private:
    NLCLRawExecutor(NLCLRuntime* runtime);
    void StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg) const;
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] const XCNLExecutor& GetExecutor() const noexcept final;
    [[nodiscard]] std::unique_ptr<xcomp::InstanceContext> PrepareInstance(const xcomp::OutputBlock& block) final;
    void OutputInstance(const xcomp::OutputBlock& block, std::u32string& dst) final;
    void ProcessStruct(const xcomp::OutputBlock& block, std::u32string& dst) final;
    [[nodiscard]] bool HandleInstanceMeta(xziar::nailang::FuncPack& meta);
    [[nodiscard]] MetaFuncResult HandleMetaFunc(xziar::nailang::FuncPack& meta, xziar::nailang::MetaSet& allMeta) final;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) final;
protected:
    forceinline KernelContext& GetCurInstance() const { return static_cast<KernelContext&>(GetInstanceInfo()); }
public:
    ~NLCLRawExecutor() override;
    using NLCLExecutor::GetRuntime;
    using NLCLExecutor::HandleException;
    using XCNLRawExecutor::ExtensionReplaceFunc;
};


class OCLUAPI NLCLStructHandler final : public NLCLExecutor, public xcomp::XCNLStructHandler
{
    friend NLCLRuntime;
private:
    NLCLStructHandler(NLCLRuntime* runtime);
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] const XCNLExecutor& GetExecutor() const noexcept final;
    void OnNewField(xcomp::XCNLStruct& target, xcomp::XCNLStruct::Field& field, xziar::nailang::MetaSet& allMetas) final;
    void OutputStruct(const xcomp::XCNLStruct& target, std::u32string& output) final;
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
public:
    ~NLCLStructHandler() override;
    using NLCLExecutor::GetRuntime;
    using NLCLExecutor::HandleException;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLRuntime final : public xcomp::XCNLRuntime
{
    friend NLCLExtension;
    friend NLCLExecutor;
    friend NLCLConfigurator;
    friend NLCLRawExecutor;
    friend NLCLProcessor;
private:
    xcomp::XCNLConfigurator& GetConfigurator() noexcept final;
    xcomp::XCNLRawExecutor& GetRawExecutor() noexcept final;
    xcomp::XCNLStructHandler& GetStructHandler() noexcept final;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLCLContext& Context;
    NLCLConfigurator Configurator;
    NLCLRawExecutor RawExecutor;
    NLCLStructHandler StructHandler;

    [[nodiscard]] xcomp::OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept final;
    void HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const xziar::nailang::FuncPack& meta, const xziar::nailang::Arg*) final;
    void BeforeFinishOutput(std::u32string& prefixes, std::u32string& structs, std::u32string& globals, std::u32string& kernels) final;
public:
    [[nodiscard]] static std::u32string_view GetCLTypeName(xcomp::VTypeInfo info) noexcept;
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx);
    ~NLCLRuntime() override;
    [[nodiscard]] xcomp::VTypeInfo TryParseVecType(const std::u32string_view type, bool allowMinBits) const noexcept final;
    [[nodiscard]] std::u32string_view GetVecTypeName(xcomp::VTypeInfo info) const noexcept final;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
};

inline constexpr NLCLRuntime& NLCLExecutor::GetRuntime() const noexcept
{
    return static_cast<NLCLRuntime&>(XCNLExecutor::GetRuntime());
}
inline constexpr NLCLRuntime& NLCLExtension::GetRuntime(xcomp::XCNLExecutor& executor)
{
    return static_cast<NLCLExecutor&>(executor).GetRuntime();
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


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
}