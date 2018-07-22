#pragma once
#include "TexUtilRely.h"
#include "common/AsyncExecutor/AsyncManager.h"


namespace oglu::texutil
{

class TEXUTILAPI TexResizer : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
    oglContext GLContext;
    oglProgram GLResizer;
    oglVBO ScreenBox;
    oglVAO NormalVAO, FlipYVAO;
    oglFBO OutputFrame;
    void Init();
public:
    TexResizer(const oglContext& baseContext);
    ~TexResizer();
    common::PromiseResult<Image> ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const bool flipY = false)
    {
        return ResizeToDat(img, width, height, img.DataType, flipY);
    }
    common::PromiseResult<Image> ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<oglTex2DS> ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY = true);
    common::PromiseResult<oglTex2DS> ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY = false);
};




}

