#pragma once
#include "oclRely.h"
#include "oclProgram.h"


namespace xcomp
{
class XCNLProgram;
}

namespace oclu
{

class NLCLProgStub;


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class OCLUAPI NLCLResult
{
public:
    virtual ~NLCLResult();
    using ResultType = std::variant<std::monostate, bool, int64_t, uint64_t, double, std::any>;
    virtual ResultType QueryResult(std::u32string_view) const = 0;
    virtual std::string_view GetNewSource() const noexcept = 0;
    virtual oclProgram GetProgram() const = 0;
};


class OCLUAPI NLCLProcessor
{
public:
    using LoggerType = std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*>;
protected:
    mutable std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*> TheLogger;
    constexpr common::mlog::MiniLogger<false>& Logger() const noexcept
    { 
        return TheLogger.index() == 0 ? std::get<0>(TheLogger) : *std::get<1>(TheLogger);
    }
    virtual void ConfigureCL(NLCLProgStub& stub) const;
    virtual std::string GenerateCL(NLCLProgStub& stub) const;
    std::unique_ptr<NLCLResult> CompileIntoProgram(NLCLProgStub& stub, const oclContext& ctx, CLProgConfig config = {}) const;
    
public:
    NLCLProcessor();
    NLCLProcessor(common::mlog::MiniLogger<false>&& logger);
    virtual ~NLCLProcessor();

    virtual std::shared_ptr<xcomp::XCNLProgram> Parse(common::span<const std::byte> source, std::u16string fileName = {}) const;
    virtual std::unique_ptr<NLCLResult> ProcessCL(const std::shared_ptr<xcomp::XCNLProgram>& prog, const oclDevice dev, const common::CLikeDefines& info = {}) const;
    virtual std::unique_ptr<NLCLResult> CompileProgram(const std::shared_ptr<xcomp::XCNLProgram>& prog, const oclContext& ctx, const oclDevice dev, const common::CLikeDefines& info = {}, const CLProgConfig& config = {}) const;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}