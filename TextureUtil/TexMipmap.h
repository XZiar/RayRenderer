#pragma once
#include "TexUtilRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

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
    oclKernel DownsampleTest;
public:
    TexMipmap(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexMipmap();
    PromiseResult<vector<Image>> GenerateMipmaps(const ImageView& src, const bool isSRGB = true, const uint8_t levels = 255);

    void Test();
    void Test2();
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
