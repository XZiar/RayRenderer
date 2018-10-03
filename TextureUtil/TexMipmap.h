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
    oclKernel Downsample;
public:
    TexMipmap(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexMipmap();

    void Test();
};

}
