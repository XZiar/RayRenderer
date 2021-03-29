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
class NLDXConfigurator;
class NLDXRawExecutor;
class NLDXRuntime;
class NLDXContext;


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


struct NLDXStruct : public xcomp::XCNLStruct
{
    enum class LayoutTarget : uint8_t { StructBuf, ConstBuf };
    LayoutTarget Layout = LayoutTarget::StructBuf;
    using XCNLStruct::XCNLStruct;
    std::string_view GetLayoutName() const noexcept;
};


class DXUAPI COMMON_EMPTY_BASES NLDXContext : public xcomp::XCNLContext
{
    friend NLDXExecutor;
    friend NLDXConfigurator;
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
    size_t GetKernelCount() const noexcept;
protected:
    std::vector<std::u32string_view> KernelNames;
    std::vector<ResourceInfo> BindResoures;
    std::vector<std::pair<xziar::nailang::Arg, uint32_t>> ReusableResIds;
    std::vector<ConstantInfo> ShaderConstants;
    std::vector<std::pair<xziar::nailang::Arg, uint32_t>> ReusableSCIds;
    std::vector<std::string> CompilerFlags;
    common::StringPool<char> StrPool;
    uint32_t MinSMVer = 0;
    bool AllowDebug;
    bool Enable16Bit;
    void AddResource(const xziar::nailang::Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype, 
        uint16_t space, uint16_t reg, uint16_t count, xcomp::InstanceArgInfo::TexTypes texType, BoundedResourceType type);
    void AddConstant(const xziar::nailang::Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype, uint16_t count);
};


class DXUAPI NLDXExecutor : public xcomp::XCNLExecutor
{
    friend NLDXRuntime;
    friend NLDXRawExecutor;
protected:
    NLDXExecutor(NLDXRuntime* runtime);
    constexpr NLDXRuntime& GetRuntime() const noexcept;
};


class DXUAPI NLDXConfigurator final : public NLDXExecutor, public xcomp::XCNLConfigurator
{
private:
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] MetaFuncResult HandleMetaFunc(const xziar::nailang::FuncCall& meta, xziar::nailang::MetaSet& allMetas) final;
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
    void EvaluateRawBlock(const xziar::nailang::RawBlock& block, xcomp::MetaFuncs metas) final;
public:
    NLDXConfigurator(NLDXRuntime* runtime);
    ~NLDXConfigurator() override;
};


class DXUAPI NLDXRawExecutor final : public NLDXExecutor, public xcomp::XCNLRawExecutor
{
    friend NLDXRuntime;
private:
    NLDXRawExecutor(NLDXRuntime* runtime);
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] const XCNLExecutor& GetExecutor() const noexcept final;
    [[nodiscard]] std::unique_ptr<xcomp::InstanceContext> PrepareInstance(const xcomp::OutputBlock& block) final;
    void OutputInstance(const xcomp::OutputBlock& block, std::u32string& output) final;
    void ProcessStruct(const xcomp::OutputBlock& block, std::u32string& output) final;
    [[nodiscard]] bool HandleInstanceMeta(xziar::nailang::FuncPack& meta);
    [[nodiscard]] MetaFuncResult HandleMetaFunc(xziar::nailang::FuncPack& meta, xziar::nailang::MetaSet& allMetas) final;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) final;
protected:
    forceinline KernelContext& GetCurInstance() const { return static_cast<KernelContext&>(GetInstanceInfo()); }
public:
    ~NLDXRawExecutor() override;
    using NLDXExecutor::GetRuntime;
    using NLDXExecutor::HandleException;
};


class DXUAPI NLDXStructHandler final : public NLDXExecutor, public xcomp::XCNLStructHandler
{
    friend NLDXRuntime;
private:
    NLDXStructHandler(NLDXRuntime* runtime);
    [[nodiscard]] XCNLExecutor& GetExecutor() noexcept final;
    [[nodiscard]] const XCNLExecutor& GetExecutor() const noexcept final;
    [[nodiscard]] std::unique_ptr<xcomp::XCNLStruct> GenerateStruct(std::u32string_view name, xziar::nailang::MetaSet& metas) final;
    void OnNewField(xcomp::XCNLStruct& target, xcomp::XCNLStruct::Field& field, xziar::nailang::MetaSet& allMetas) final;
    void FillFieldOffsets(xcomp::XCNLStruct& target) final;
    void OutputStruct(const xcomp::XCNLStruct& target, std::u32string& output) final;
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) final;
public:
    ~NLDXStructHandler() override;
    using NLDXExecutor::GetRuntime;
    using NLDXExecutor::HandleException;
    void OutputStructContent(const NLDXStruct& target, std::u32string& output) const;
};


class DXUAPI COMMON_EMPTY_BASES NLDXRuntime final : public xcomp::XCNLRuntime
{
    friend NLDXExtension;
    friend NLDXExecutor;
    friend NLDXConfigurator;
    friend NLDXRawExecutor;
    friend NLDXProcessor;
private:
    xcomp::XCNLConfigurator& GetConfigurator() noexcept final;
    xcomp::XCNLRawExecutor& GetRawExecutor() noexcept final;
    xcomp::XCNLStructHandler& GetStructHandler() noexcept final;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLDXContext& Context;
    NLDXConfigurator Configurator;
    NLDXRawExecutor RawExecutor;
    NLDXStructHandler StructHandler;

    [[nodiscard]] xcomp::OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept final;
    void HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const xziar::nailang::FuncPack& meta, const xziar::nailang::Arg* source) final;
    void BeforeFinishOutput(std::u32string& prefixes, std::u32string& structs, std::u32string& globals, std::u32string& kernels) final;
public:
    [[nodiscard]] static std::u32string_view GetDXTypeName(xcomp::VTypeInfo info) noexcept;
    NLDXRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLDXContext> evalCtx);
    ~NLDXRuntime() override;
    [[nodiscard]] xcomp::VTypeInfo TryParseVecType(const std::u32string_view type, bool allowMinBits) const noexcept final;
    [[nodiscard]] std::u32string_view GetVecTypeName(xcomp::VTypeInfo info) const noexcept final;
};

inline constexpr NLDXRuntime& NLDXExecutor::GetRuntime() const noexcept 
{ 
    return static_cast<NLDXRuntime&>(XCNLExecutor::GetRuntime()); 
}


class DXUAPI NLDXBaseResult : public NLDXResult
{
protected:
    mutable xziar::nailang::NailangBasicRuntime Runtime;
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