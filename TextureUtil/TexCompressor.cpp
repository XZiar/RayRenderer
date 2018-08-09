#include "TexUtilRely.h"
#include "TexCompressor.h"
#include "ISPCCompress.inl"
#include "STBCompress.inl"
#include "OpenGLUtil/oglException.h"
#include "common/PromiseTaskSTD.hpp"
#include <future>


namespace oglu::texutil
{


static void CheckImgSize(const Image& img)
{
    if (img.Width % 4 != 0 || img.Height % 4 != 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a size of multiple of 4.");
    if (img.Width == 0 || img.Height == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a non-zero size.");
}

common::AlignedBuffer<32> CompressToDat(const Image& img, const TextureInnerFormat format, const bool needAlpha)
{
    CheckImgSize(img);
    common::SimpleTimer timer;
    common::AlignedBuffer<32> result;
    timer.Start();
    switch (format)
    {
    case TextureInnerFormat::BC1:
    case TextureInnerFormat::BC1SRGB:
        result = detail::CompressBC1(img); break;
    case TextureInnerFormat::BC3:
    case TextureInnerFormat::BC3SRGB:
        result = detail::CompressBC3(img); break;
    case TextureInnerFormat::BC5:
        result = detail::CompressBC5(img); break;
    case TextureInnerFormat::BC7:
    case TextureInnerFormat::BC7SRGB:
        result = detail::CompressBC7(img, needAlpha); break;
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"not supported compression yet");
    }
    timer.Stop();
    texLog().debug(u"Compressed a image of [{}x{}] to [{}], cost {}ms.\n",
        img.Width, img.Height, oglu::TexFormatUtil::GetFormatName(format), timer.ElapseMs());
    return result;
}

common::PromiseResult<oglTex2DV> CompressToTex(const Image& img, const TextureInnerFormat format, const bool needAlpha)
{
    auto buffer = CompressToDat(img, format, needAlpha);
    const auto pms = std::make_shared<std::promise<oglTex2DV>>();
    auto ret = std::make_shared<common::PromiseResultSTD<oglTex2DV, true>>(*pms);
    oglUtil::invokeSyncGL([buf = std::move(buffer), w=img.Width, h=img.Height, iformat = format, pms](const auto& agent)
    {
        oglTex2DS tex(w, h, iformat);
        tex->SetCompressedData(buf.GetRawPtr(), buf.GetSize());
        const auto texv = tex->GetTextureView();
        agent.Await(oglu::oglUtil::SyncGL());
        pms->set_value(texv);
    });
    return ret;
}


}
