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
    if (img.GetWidth() % 4 != 0 || img.GetHeight() % 4 != 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a size of multiple of 4.");
    if (img.GetWidth() == 0 || img.GetHeight() == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a non-zero size.");
}

common::AlignedBuffer CompressToDat(const Image& img, const TextureInnerFormat format, const bool needAlpha)
{
    CheckImgSize(img);
    common::SimpleTimer timer;
    common::AlignedBuffer result;
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
        img.GetWidth(), img.GetHeight(), oglu::TexFormatUtil::GetFormatName(format), timer.ElapseMs());
    return result;
}


}
