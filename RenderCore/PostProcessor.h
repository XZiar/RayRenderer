#pragma once
#include "RenderCoreRely.h"
#include "Controllable.hpp"

namespace rayr
{

class PostProcessor : public NonCopyable, public Controllable
{
private:
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oglu::oglContext GLContext;
    oglu::oglTex3DS LutTex;
    oglu::oglTex3DV LutTexView;
    oglu::oglImg3D LutImg;
    oglu::oglComputeProgram LutGenerator;
    const uint32_t LutSize;
    array<uint32_t, 3> GroupCount;
    float Exposure = 0.0f;
    bool ShouldUpdate = true;
public:
    PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que, const uint32_t lutSize = 32);
    ~PostProcessor();
    constexpr uint32_t GetLutSize() const { return LutSize; }
    oglu::oglTex3DV GetLut() const { return LutTexView; }
    float GetExposure() const { return Exposure; }
    void SetExposure(const float exposure) { Exposure = exposure; ShouldUpdate = true; }
    
    bool UpdateLut();
};

}
