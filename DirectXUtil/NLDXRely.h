#pragma once
#include "DxRely.h"
#include "NLDX.h"
#include "XComputeBase/XCompNailang.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"

namespace dxu
{
#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NLDXExecutor;
class NLDXRawExecutor;
class NLDXRuntime;
class NLDXContext;
class KernelExtension;


struct ResourceInfo
{
    std::string KernelIds;
    common::StringPiece<char> Name;
    common::StringPiece<char> DataType;
    uint16_t Space;
    uint16_t BindReg;
    uint16_t Count;
    xcomp::InstanceArgInfo::TexTypes TexType;
    BoundedResourceType Type;
};
struct ConstantInfo
{
    std::string KernelIds;
    common::StringPiece<char> Name;
    common::StringPiece<char> DataType;
    uint16_t Count;
};

struct KernelContext : public xcomp::InstanceContext
{
    friend NLDXRawExecutor;
    friend NLDXRuntime;
    std::u32string GroupIdVar, ItemIdVar, GlobalIdVar, TIdVar;
    KernelContext(uint8_t kerId) noexcept : KernelId(kerId) { }
    ~KernelContext() override {}
    
    forceinline bool AddAttribute(const std::u32string_view id, std::u32string_view content)
    {
        return Add(Attributes, id, content, {});
    }
    forceinline constexpr uint32_t GetWorkgroupSize() const noexcept { return WorkgroupSize; }
protected:
    std::vector<NamedText> Attributes;
    uint32_t WorkgroupSize = 0;
    uint8_t KernelId = 0;
};


class DXUAPI NLDXExtension : public xcomp::XCNLExtension
{
    friend NLDXRuntime;
    friend NLDXProgStub;
    friend NLDXContext;
protected:
    NLDXContext& Context;
public:
    NLDXExtension(NLDXContext& context);
    ~NLDXExtension() override;
};


class DXUAPI COMMON_EMPTY_BASES NLDXContext : public xcomp::XCNLContext
{
    friend NLDXExecutor;
    friend NLDXRawExecutor;
    friend NLDXProcessor;
    friend NLDXRuntime;
    friend NLDXProgStub;
private:
    struct DXUVar;
    xziar::nailang::Arg DXUArg;
public:
    const DxDevice Device;
    NLDXContext(DxDevice dev, const common::CLikeDefines& info);
    ~NLDXContext() override;
    [[nodiscard]] xziar::nailang::ArgLocator LocateArg(const xziar::nailang::LateBindVar& var, bool create) noexcept override;
    [[nodiscard]] VecTypeResult ParseVecType(const std::u32string_view type) const noexcept override;
    [[nodiscard]] std::u32string_view GetVecTypeName(common::simd::VecDataInfo info) const noexcept override;
    [[nodiscard]] static std::u32string_view GetDXTypeName(common::simd::VecDataInfo info) noexcept;
protected:
    std::vector<std::u32string_view> KernelNames;
    std::vector<ResourceInfo> BindResoures;
    std::vector<std::pair<xziar::nailang::Arg, uint32_t>> ReusableResIds;
    std::vector<ConstantInfo> ShaderConstants;
    std::vector<std::pair<xziar::nailang::Arg, uint32_t>> ReusableSCIds;
    std::vector<std::string> CompilerFlags;
    common::StringPool<char> StrPool;
    bool AllowDebug;
    void AddResource(const xziar::nailang::Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype, 
        uint16_t space, uint16_t reg, uint16_t count, xcomp::InstanceArgInfo::TexTypes texType, BoundedResourceType type);
    void AddConstant(const xziar::nailang::Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype, uint16_t count);
};


class DXUAPI NLDXExecutor : public xcomp::XCNLExecutor
{
    friend NLDXRuntime;
    friend NLDXRawExecutor;
    NLDXExecutor(NLDXRuntime* runtime);
    constexpr NLDXRuntime& GetRuntime() const noexcept;
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
};


class DXUAPI NLDXRawExecutor : public xcomp::XCNLRawExecutor
{
    friend NLDXRuntime;
private:
    NLDXRawExecutor(NLDXExecutor& executor);
    constexpr NLDXRuntime& GetRuntime() const noexcept { return static_cast<NLDXExecutor&>(Executor).GetRuntime(); }
    [[nodiscard]] bool HandleMetaFunc(xziar::nailang::MetaEvalPack& meta) override;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) final;
    [[nodiscard]] std::unique_ptr<xcomp::InstanceContext> PrepareInstance(const xcomp::OutputBlock& block) final;
    void ProcessStruct(const xcomp::OutputBlock& block, std::u32string& dst) final;
    void OutputInstance(const xcomp::OutputBlock& block, std::u32string& dst) final;
};


class DXUAPI COMMON_EMPTY_BASES NLDXRuntime : public xcomp::XCNLRuntime
{
    friend NLDXExecutor;
    friend NLDXRawExecutor;
    friend KernelExtension;
    friend NLDXProcessor;
private:
    xcomp::XCNLExecutor& GetBaseExecutor() noexcept final;
    xcomp::XCNLRawExecutor& GetRawExecutor() noexcept final;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLDXContext& Context;
    NLDXExecutor BaseExecutor;
    NLDXRawExecutor RawExecutor;

    [[nodiscard]] xcomp::OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept override;
    void HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const xziar::nailang::FuncPack& meta, const xziar::nailang::Arg* source) override;
    void BeforeFinishOutput(std::u32string& prefix, std::u32string& content) override;
public:
    NLDXRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLDXContext> evalCtx);
    ~NLDXRuntime() override;
};

inline constexpr NLDXRuntime& NLDXExecutor::GetRuntime() const noexcept 
{ 
    return static_cast<NLDXRuntime&>(XCNLExecutor::GetRuntime()); 
}


class DXUAPI NLDXBaseResult : public NLDXResult
{
private:
    mutable NLDXRuntime TempRuntime;
protected:
    std::shared_ptr<NLDXContext> Context;
public:
    NLDXBaseResult(const std::shared_ptr<NLDXContext>& context);
    ~NLDXBaseResult() override;
    NLDXResult::ResultType QueryResult(std::u32string_view name) const override;
};

class DXUAPI NLDXUnBuildResult : public NLDXBaseResult
{
protected:
    std::string Source;
public:
    NLDXUnBuildResult(const std::shared_ptr<NLDXContext>& context, std::string&& source);
    ~NLDXUnBuildResult() override;
    std::string_view GetNewSource() const noexcept override;
    common::span<const std::pair<std::string, DxShader>> GetShaders() const override;
};

class DXUAPI NLDXBuiltResult : public NLDXBaseResult
{
protected:
    std::string Source;
    std::vector<std::pair<std::string, DxShader>> Shaders;
public:
    NLDXBuiltResult(const std::shared_ptr<NLDXContext>& context, std::string source, std::vector<std::pair<std::string, DxShader>> shaders);
    ~NLDXBuiltResult() override;
    std::string_view GetNewSource() const noexcept override;
    common::span<const std::pair<std::string, DxShader>> GetShaders() const override;
};

class DXUAPI NLDXBuildFailResult : public NLDXUnBuildResult
{
protected:
    std::shared_ptr<common::ExceptionBasicInfo> Exception;
public:
    NLDXBuildFailResult(const std::shared_ptr<NLDXContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex);
    ~NLDXBuildFailResult() override;
    common::span<const std::pair<std::string, DxShader>> GetShaders() const override;
};


class DXUAPI NLDXProgStub : public xcomp::XCNLProgStub
{
    friend class NLDXProcessor;
private:
    void Prepare();
public:
    NLDXProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLDXContext> context, common::mlog::MiniLogger<false>& logger);
    NLDXProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLDXContext> context, std::unique_ptr<NLDXRuntime>&& runtime);

    std::shared_ptr<NLDXContext> GetContext() const noexcept
    {
        return std::static_pointer_cast<NLDXContext>(Context);
    }
    NLDXRuntime& GetRuntime() const noexcept
    {
        return *static_cast<NLDXRuntime*>(Runtime.get());
    }
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}