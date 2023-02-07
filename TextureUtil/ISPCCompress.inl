#pragma once

#include "TexUtilRely.h"
#include "ISPCTextureCompressor/ispc_texcomp/ispc_texcomp.h"

namespace oglu::texutil::detail::ispc
{
using xziar::img::ImageView;
using xziar::img::ImageDataType;


static common::AlignedBuffer CompressBC1(const ImageView& img)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC1");

    if (img.GetDataType() != ImageDataType::RGBA)
    {
        const auto tmpImg = img.ConvertTo(ImageDataType::RGBA);
        return CompressBC1(tmpImg);
    }
    const rgba_surface surface{ const_cast<uint8_t*>(img.GetRawPtr<uint8_t>()), (int32_t)img.GetWidth(), (int32_t)img.GetHeight(), (int32_t)img.RowSize() };
    const uint32_t blkCount = img.GetWidth() * img.GetHeight() / 4 / 4;
    common::AlignedBuffer buffer(8 * blkCount);
    CompressBlocksBC1(&surface, buffer.GetRawPtr<uint8_t>());
    return buffer;
}

static common::AlignedBuffer CompressBC3(const ImageView& img)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC3");

    if (img.GetDataType() != ImageDataType::RGBA)
    {
        const auto tmpImg = img.ConvertTo(ImageDataType::RGBA);
        return CompressBC3(tmpImg);
    }
    const rgba_surface surface{ const_cast<uint8_t*>(img.GetRawPtr<uint8_t>()), (int32_t)img.GetWidth(), (int32_t)img.GetHeight(), (int32_t)img.RowSize() };
    const uint32_t blkCount = img.GetWidth() * img.GetHeight() / 4 / 4;
    common::AlignedBuffer buffer(16 * blkCount);
    CompressBlocksBC3(&surface, buffer.GetRawPtr<uint8_t>());
    return buffer;
}

static common::AlignedBuffer CompressBC4(const ImageView& img)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC4");
    if (img.GetDataType() != ImageDataType::GRAY)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"only single channel supported in BC4");

    const rgba_surface surface{ const_cast<uint8_t*>(img.GetRawPtr<uint8_t>()), (int32_t)img.GetWidth(), (int32_t)img.GetHeight(), (int32_t)img.RowSize() };
    const uint32_t blkCount = img.GetWidth() * img.GetHeight() / 4 / 4;
    common::AlignedBuffer buffer(8 * blkCount);
    CompressBlocksBC4(&surface, buffer.GetRawPtr<uint8_t>());
    return buffer;
}

static common::AlignedBuffer CompressBC5(const ImageView& img)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC5");
    if (img.GetDataType() == ImageDataType::GRAY)
    {
        const auto tmpImg = img.ConvertTo(ImageDataType::RA);
        return CompressBC5(tmpImg);
    }
    if (img.GetDataType() != ImageDataType::RA)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"only two channel supported in BC5");

    const rgba_surface surface{ const_cast<uint8_t*>(img.GetRawPtr<uint8_t>()), (int32_t)img.GetWidth(), (int32_t)img.GetHeight(), (int32_t)img.RowSize() };
    const uint32_t blkCount = img.GetWidth() * img.GetHeight() / 4 / 4;
    common::AlignedBuffer buffer(16 * blkCount);
    CompressBlocksBC5(&surface, buffer.GetRawPtr<uint8_t>());
    return buffer;
}

static common::AlignedBuffer CompressBC7(const ImageView& img, const bool needAlpha)
{
    if (HAS_FIELD(img.GetDataType(), ImageDataType::FLOAT_MASK))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"float data type not supported in BC7");

    if (img.GetDataType() != ImageDataType::RGBA)
    {
        const auto tmpImg = img.ConvertTo(ImageDataType::RGBA);
        return CompressBC7(tmpImg, needAlpha);
    }
    const rgba_surface surface{ const_cast<uint8_t*>(img.GetRawPtr<uint8_t>()), (int32_t)img.GetWidth(), (int32_t)img.GetHeight(), (int32_t)img.RowSize() };
    const uint32_t blkCount = img.GetWidth() * img.GetHeight() / 4 / 4;
    common::AlignedBuffer buffer(16 * blkCount);
    bc7_enc_settings settings;
    needAlpha ? GetProfile_alpha_ultrafast(&settings) : GetProfile_ultrafast(&settings);
    CompressBlocksBC7(&surface, buffer.GetRawPtr<uint8_t>(), &settings);
    return buffer;
}


}
