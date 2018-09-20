#pragma once
#include "RenderCoreRely.h"


namespace rayr
{

class PostProcessor
{
    friend class BasicTest;
private:
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oclu::oclProgram LutProg;
    oclu::oclKernel LutGenerator;
    oglu::oglContext GLContext;
    oglu::oglTex3DS LutTex;
    oglu::oglImg3D LutImg;
    oglu::oglComputeProgram LutGenerator2;
    float Exposure = 1.0f;
    bool ShouldUpdate = true;
    bool UpdateLut();
public:
    PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que);
    ~PostProcessor();
    float GetExposure() const { return Exposure; }
    void SetExposure(const float exposure) { Exposure = exposure; ShouldUpdate = true; }
};

}
