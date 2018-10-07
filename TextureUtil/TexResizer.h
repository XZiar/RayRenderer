#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{
using common::PromiseResult;
using namespace oclu;

enum class ResizeMethod : uint8_t { OpenGL, Compute, OpenCL, CPU };

class TEXUTILAPI TexResizer : public NonCopyable, public NonMovable
{
private:
    std::shared_ptr<TexUtilWorker> Worker;
    oglContext GLContext;
    oclContext CLContext;
    oclCmdQue CmdQue;
    oglDrawProgram GLResizer;
    oglComputeProgram GLResizer2;
    oglVBO ScreenBox;
    oglVAO NormalVAO, FlipYVAO;
    oglFBO OutputFrame;
    oclKernel KerToImg, KerToDat3, KerToDat4;
    common::PromiseResult<Image> ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format);
public:
    TexResizer(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexResizer();
   /* static void CheckOutputFormat(const ImageDataType format);
    static void CheckOutputFormat(const TextureInnerFormat format);
    */
    template<ResizeMethod>
    PromiseResult<oglTex2DS> ResizeToTex(const oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<oglTex2DS> ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<oglTex2DS> ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<oglTex2DS> ResizeToTex(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY = false);

    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);
    template<ResizeMethod>
    PromiseResult<Image> ResizeToImg(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY = false);

};



}

