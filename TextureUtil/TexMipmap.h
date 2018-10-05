#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{
using common::PromiseResult;
using namespace oclu;

class TEXUTILAPI TexMipmap : public NonCopyable, public NonMovable
{
private:
    std::shared_ptr<TexUtilWorker> Worker;
    oglContext GLContext;
    oclContext CLContext;
    oclCmdQue CmdQue;
    oclKernel DownsampleSrc;
    oclKernel DownsampleMid;
    oclKernel DownsampleRaw;
public:
    TexMipmap(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexMipmap();
    PromiseResult<vector<Image>> GenerateMipmaps(Image&& raw, const bool isSRGB = true, const uint8_t levels = 255);


    void Test();
};

}
