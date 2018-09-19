#pragma once
#include "RenderCoreRely.h"


namespace rayr
{

class PostProcessor
{
private:
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oclu::oclProgram LutProg;
    oclu::oclKernel LutGenerator;
    oglu::oglTex3DS LutTex;
    oclu::oclGLImg3D LutImg;
    oclu::oclImg3D LutImgRaw;
    float Exposure = 1.0f;
    bool ShouldUpdate = true;
    void UpdateLut();
public:
    PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que);
    ~PostProcessor();
    float GetExposure() const { return Exposure; }
    void SetExposure(const float exposure) { Exposure = exposure; ShouldUpdate = true; }
};

}
