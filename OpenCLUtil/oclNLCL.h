#pragma once
#include "oclRely.h"
#include "oclProgram.h"

namespace oclu
{

class OCLUAPI NLCLProcessor
{
protected:
    class NailangHolder;
    std::unique_ptr<NailangHolder> Holder;
    common::mlog::MiniLogger<false> Logger;
    NLCLProcessor(common::mlog::MiniLogger<false>&& logger, common::span<const std::byte> source);
    virtual void InitHolder();
public:
    NLCLProcessor(common::span<const std::byte> source);
    virtual ~NLCLProcessor();

    virtual void ConfigureCL();
    virtual std::u32string GenerateCL();
    virtual oclProgram CompileProgram();
};

}