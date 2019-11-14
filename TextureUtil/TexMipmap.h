#pragma once
#include "TexUtilRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu::texutil
{


class TEXUTILAPI TexMipmap : public common::NonCopyable, public common::NonMovable
{
private:
    std::shared_ptr<TexUtilWorker> Worker;
    oglContext GLContext;
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oclu::oclKernel DownsampleSrc;
    oclu::oclKernel DownsampleMid;
    oclu::oclKernel DownsampleRaw;
    oclu::oclKernel DownsampleTest;
    common::PromiseResult<std::vector<xziar::img::Image>> GenerateMipmapsCL
        (const xziar::img::ImageView src, const bool isSRGB, const uint8_t levels);
    common::PromiseResult<std::vector<xziar::img::Image>> GenerateMipmapsCPU
        (const xziar::img::ImageView src, const bool isSRGB, const uint8_t levels);
public:
    TexMipmap(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexMipmap();
    common::PromiseResult<std::vector<xziar::img::Image>> GenerateMipmaps
        (const xziar::img::ImageView src, const bool isSRGB = true, const uint8_t levels = 255);

    void Test();
    void Test2();
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
