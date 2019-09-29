#pragma once
#include "TexUtilRely.h"
#include "AsyncExecutor/AsyncManager.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu::texutil
{

class TEXUTILAPI GLTexResizer : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
    oglContext GLContext;
    oglDrawProgram GLResizer;
    oglComputeProgram GLResizer2;
    oglVBO ScreenBox;
    oglVAO NormalVAO, FlipYVAO;
    oglFBO OutputFrame;
    common::PromiseResult<Image> ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format);
public:
    GLTexResizer(oglContext&& glContext);
    ~GLTexResizer();
    common::PromiseResult<Image> ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const bool flipY = false)
    {
        return ResizeToDat(img, width, height, img.GetDataType(), flipY);
    }
    common::PromiseResult<Image> ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    //require in the shared context
    common::PromiseResult<Image> ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);

    common::PromiseResult<oglTex2DS> ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY = true);
    //require in the shared context
    common::PromiseResult<oglTex2DS> ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY = false);
    common::PromiseResult<oglTex2DS> ResizeToTex(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat dataFormat, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY = false);
};




}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

