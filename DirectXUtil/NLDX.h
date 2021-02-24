#pragma once
#include "DxRely.h"
#include "DxShader.h"


namespace xcomp
{
class XCNLProgram;
}

namespace dxu
{

class NLDXProgStub;


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class DXUAPI NLDXResult
{
public:
    virtual ~NLDXResult();
    using ResultType = std::variant<std::monostate, bool, int64_t, uint64_t, double, std::any>;
    virtual ResultType QueryResult(std::u32string_view) const = 0;
    virtual std::string_view GetNewSource() const noexcept = 0;
    virtual DxShader GetShader() const = 0;
};


class DXUAPI NLDXProcessor
{
public:
    using LoggerType = std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*>;
protected:
    mutable std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*> TheLogger;
    constexpr common::mlog::MiniLogger<false>& Logger() const noexcept
    { 
        return TheLogger.index() == 0 ? std::get<0>(TheLogger) : *std::get<1>(TheLogger);
    }
    virtual void ConfigureDX(NLDXProgStub& stub) const;
    virtual std::string GenerateDX(NLDXProgStub& stub) const;
    std::unique_ptr<NLDXResult> CompileIntoProgram(NLDXProgStub& stub, const DxDevice dev, DxShaderConfig config = {}) const;
    
public:
    NLDXProcessor();
    NLDXProcessor(common::mlog::MiniLogger<false>&& logger);
    virtual ~NLDXProcessor();

    virtual std::shared_ptr<xcomp::XCNLProgram> Parse(common::span<const std::byte> source, std::u16string fileName = {}) const;
    virtual std::unique_ptr<NLDXResult> ProcessDX(const std::shared_ptr<xcomp::XCNLProgram>& prog, const DxDevice dev, const common::CLikeDefines& info = {}) const;
    virtual std::unique_ptr<NLDXResult> CompileShader(const std::shared_ptr<xcomp::XCNLProgram>& prog, const DxDevice dev, const common::CLikeDefines& info = {}, const DxShaderConfig& config = {}) const;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}