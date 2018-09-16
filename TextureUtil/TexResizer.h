#pragma once
#include "TexUtilRely.h"
#include "common/AsyncExecutor/AsyncManager.h"

namespace oglu::texutil
{
using common::PromiseResult;
using namespace oclu;

enum class ResizeMethod : uint8_t { OpenGL, Compute, OpenCL, CPU };

class TEXUTILAPI TexResizer : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
    oglContext GLContext;
    oclContext CLContext;
    oglDrawProgram GLResizer;
    oglComputeProgram GLResizer2;
    oglVBO ScreenBox;
    oglVAO NormalVAO, FlipYVAO;
    oglFBO OutputFrame;
    oclCmdQue CmdQue;
    oclKernel KerToImg, KerToDat3, KerToDat4;
    common::PromiseResult<Image> ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format);
    PromiseResult<oglTex2D> ConvertToTex(const oclImg2D& img);
public:
    TexResizer(oglContext&& glContext, const oclu::oclContext& clContext);
    ~TexResizer();
   /* static void CheckOutputFormat(const ImageDataType format);
    static void CheckOutputFormat(const TextureInnerFormat format);
    
    common::PromiseResult<oglTex2DS> ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY = true);
    common::PromiseResult<oglTex2DS> ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY = false);
    common::PromiseResult<oglTex2DS> ResizeToTex(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY = false);

    common::PromiseResult<Image> ResizeToDat(const oclu::oclImg2D& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
    common::PromiseResult<Image> ResizeToDat(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY = false);
*/

    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);

};



}

