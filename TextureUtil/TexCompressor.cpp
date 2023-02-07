#include "TexUtilPch.h"
#include "TexCompressor.h"
#include "ISPCCompress.inl"
#include "STBCompress.inl"


namespace oglu::texutil
{
using namespace xziar::img;


static void CheckImgSize(const ImageView& img)
{
    if (img.GetWidth() % 4 != 0 || img.GetHeight() % 4 != 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a size of multiple of 4.");
    if (img.GetWidth() == 0 || img.GetHeight() == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image being comoressed should has a non-zero size.");
}

common::AlignedBuffer CompressToDat(const ImageView& img, const TextureFormat format, const bool needAlpha, const bool preferIspc)
{
    constexpr std::u16string_view HostISPC = u"ISPC";
    constexpr std::u16string_view HostSTB  = u"STB";
    CheckImgSize(img);
    common::SimpleTimer timer;
    common::AlignedBuffer result;
    std::u16string_view host;
    timer.Start();
    switch (format)
    {
    case TextureFormat::BC1:
    case TextureFormat::BC1SRGB:
        host = HostISPC;
        result = detail::ispc::CompressBC1(img); 
        break;
    case TextureFormat::BC3:
    case TextureFormat::BC3SRGB:
        host = HostISPC;
        result = detail::ispc::CompressBC3(img);
        break;
    case TextureFormat::BC4:
        host = HostISPC;
        result = detail::ispc::CompressBC4(img);
        break;
    case TextureFormat::BC5:
        if (preferIspc)
        {
            host = HostISPC;
            result = detail::ispc::CompressBC5(img);
        }
        else
        {
            host = HostSTB;
            result = detail::stb::CompressBC5(img);
        }
        break;
    case TextureFormat::BC7:
    case TextureFormat::BC7SRGB:
        host = HostISPC;
        result = detail::ispc::CompressBC7(img, needAlpha);
        break;
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"not supported compression yet");
    }
    timer.Stop();
    texLog().Debug(u"Compressed a image of [{}x{}] to [{}] with [{}], cost {}ms.\n",
        img.GetWidth(), img.GetHeight(), xziar::img::TexFormatUtil::GetFormatName(format), host, timer.ElapseMs());
    return result;
}


}
