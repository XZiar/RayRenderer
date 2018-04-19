#include "TexCompressor.h"
#include "BCCompress.hpp"
#include "common/PromiseTaskSTD.hpp"
#include <future>


namespace oglu::texcomp
{


static void CheckImgSize(const Image& img)
{
    if (img.Width % 4 != 0 || img.Height % 4 != 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"image being comoressed should has a size of multiple of 4.");
}

common::AlignedBuffer<32> CompressToDat(const Image& img, const TextureInnerFormat format, const bool needAlpha)
{
    common::SimpleTimer timer;
    timer.Start();
    switch (format)
    {
    case TextureInnerFormat::BC1:
        return detail::CompressBC1(img);
    case TextureInnerFormat::BC3:
        return detail::CompressBC3(img);
    case TextureInnerFormat::BC7:
        return detail::CompressBC7(img, needAlpha);
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"not supported yet");
    }
    timer.Stop();
    texcLog().debug(u"Compressed a image of [{}x{}] to [{}], cost {}ms.\n", img.Width, img.Height, (uint32_t)format, timer.ElapseMs());
}

common::PromiseResult<oglTex2DV> CompressToTex(const Image& img, const TextureInnerFormat format, const bool needAlpha)
{
    const auto buffer = CompressToDat(img, format, needAlpha);
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
