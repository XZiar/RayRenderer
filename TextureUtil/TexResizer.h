#pragma once
#include "TexUtilRely.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu::texutil
{

enum class ResizeMethod : uint8_t { OpenGL, Compute, OpenCL, CPU };

class TEXUTILAPI TexResizer : public common::NonCopyable, public common::NonMovable
{
private:
    std::shared_ptr<TexUtilWorker> Worker;
    oglContext GLContext;
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oglDrawProgram GLResizer;
    oglComputeProgram GLResizer2;
    oglVBO ScreenBox;
    oglVAO NormalVAO, FlipYVAO;
    oclu::oclKernel KerToImg, KerToDat3, KerToDat4;
    common::PromiseResult<xziar::img::Image> ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const xziar::img::ImgDType format);
public:
    TexResizer(const std::shared_ptr<TexUtilWorker>& worker);
    ~TexResizer();
   /* static void CheckOutputFormat(const xziar::img::ImageDataType format);
    static void CheckOutputFormat(const TextureInnerFormat format);
    */
    template<ResizeMethod>
    common::PromiseResult<oglTex2DS> ResizeToTex(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const xziar::img::TextureFormat output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<oglTex2DS> ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const xziar::img::TextureFormat output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<oglTex2DS> ResizeToTex(const xziar::img::ImageView& img, const uint16_t width, const uint16_t height, const xziar::img::TextureFormat output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<oglTex2DS> ResizeToTex(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const xziar::img::TextureFormat innerFormat, 
        const uint16_t width, const uint16_t height, const xziar::img::TextureFormat output, const bool flipY = false);

    template<ResizeMethod>
    common::PromiseResult<xziar::img::Image> ResizeToImg(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const xziar::img::ImgDType output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<xziar::img::Image> ResizeToImg(const oglTex2D& tex, const uint16_t width, const uint16_t height, const xziar::img::ImgDType output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<xziar::img::Image> ResizeToImg(const xziar::img::ImageView& img, const uint16_t width, const uint16_t height, const xziar::img::ImgDType output, const bool flipY = false);
    template<ResizeMethod>
    common::PromiseResult<xziar::img::Image> ResizeToImg(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const xziar::img::TextureFormat innerFormat,
        const uint16_t width, const uint16_t height, const xziar::img::ImgDType output, const bool flipY = false);

};



}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

