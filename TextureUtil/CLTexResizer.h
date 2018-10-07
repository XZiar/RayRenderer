#pragma once
#include "TexUtilRely.h"
#include "common/AsyncExecutor/AsyncManager.h"

namespace oglu::texutil
{

class TEXUTILAPI CLTexResizer : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
    oglContext GLContext;
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oclu::oclKernel ResizeToImg, ResizeToDat3, ResizeToDat4;
    common::PromiseResult<Image> ResizeToDat(const oclu::oclImg2D& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
public:
    CLTexResizer(oglContext&& glContext, const oclu::oclContext& clContext);
    ~CLTexResizer();
    //require in the shared context
    common::PromiseResult<Image> ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);

};



}

