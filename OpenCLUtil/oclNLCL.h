#pragma once
#include "oclRely.h"
#include "oclProgram.h"

namespace oclu
{

class NLCLProgram;
class NLCLProgStub;

class OCLUAPI NLCLProcessor
{
public:
    using LoggerType = std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*>;
protected:
    mutable std::variant<common::mlog::MiniLogger<false>, common::mlog::MiniLogger<false>*> TheLogger;
    common::mlog::MiniLogger<false>& Logger() 
    { 
        return TheLogger.index() == 0 ? std::get<0>(TheLogger) : *std::get<1>(TheLogger);
    }
    virtual void ConfigureCL(NLCLProgStub& stub);
public:
    NLCLProcessor();
    NLCLProcessor(common::mlog::MiniLogger<false>&& logger);
    virtual ~NLCLProcessor();

    virtual std::shared_ptr<NLCLProgram> Parse(common::span<const std::byte> source);
    virtual NLCLProgStub ConfigureCL(const std::shared_ptr<NLCLProgram>& prog, const oclDevice dev);
    virtual oclProgram CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, const oclDevice dev);
}; 

}