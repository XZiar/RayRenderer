#include "oglRely.h"
#include "oglTexture.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"

namespace oglu::detail
{

static ContextResource<std::shared_ptr<TextureManager>, false> CTX_TEX_MAN;

TextureManager& _oglTexBase::getTexMan() noexcept
{
    const auto texman = CTX_TEX_MAN.GetOrInsert([](auto dummy) { return std::make_shared<TextureManager>(); });
    return *texman;
}

_oglTexBase::_oglTexBase(const TextureType type) noexcept : Type(type)
{
    glGenTextures(1, &textureID);
}

_oglTexBase::~_oglTexBase() noexcept
{
    //force unbind texture, since texID may be reused after releasaed
    getTexMan().forcePop(textureID);
    glDeleteTextures(1, &textureID);
}

void _oglTexBase::bind(const uint16_t pos) const noexcept
{
    glActiveTexture(GL_TEXTURE0 + pos);
    glBindTexture((GLenum)Type, textureID);
}

void _oglTexBase::unbind() const noexcept
{
    glBindTexture((GLenum)Type, 0);
}

std::pair<uint32_t, uint32_t> _oglTexBase::GetInternalSize2() const
{
    GLint w = 0, h = 0;
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    return { (uint32_t)w,(uint32_t)h };
}

std::tuple<uint32_t, uint32_t, uint32_t> _oglTexBase::GetInternalSize3() const
{
    GLint w = 0, h = 0, z = 0;
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_DEPTH, &z);
    return { (uint32_t)w,(uint32_t)h,(uint32_t)z };
}

void _oglTexBase::SetProperty(const TextureFilterVal magFilter, const TextureFilterVal minFilter, const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_WRAP_S, (GLint)wrapS);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_WRAP_T, (GLint)wrapT);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
}

bool _oglTexBase::IsCompressed() const
{
    GLint ret = GL_FALSE;
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_COMPRESSED, &ret);
    return ret != GL_FALSE;
}

//TextureInnerFormat _oglTexBase::ParseFormatStorage(const TextureDataFormat dformat) noexcept
//{
//    switch (dformat | TextureDataFormat::NORMAL_MASK)
//    {
//    case TextureDataFormat::R8:
//        return TextureInnerFormat::R8;
//    case TextureDataFormat::RG8:    
//        return GL_RG8;
//    case TextureDataFormat::RGB8:
//    case TextureDataFormat::BGR8:
//        return GL_RGB8;
//    case TextureDataFormat::RGBA8:
//    case TextureDataFormat::BGRA8:
//        return GL_RGBA8;
//    case TextureDataFormat::Rf:
//        return GL_R32F;
//    case TextureDataFormat::RGf:
//        return GL_RG32F;
//    case TextureDataFormat::RGBf:
//    case TextureDataFormat::BGRf:
//        return GL_RGB32F;
//    case TextureDataFormat::RGBAf:
//    case TextureDataFormat::BGRAf:
//        return GL_RGBA32F;
//    }
//    return GL_INVALID_ENUM;
//}
void _oglTexBase::ParseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept
{
    switch (dformat & TextureDataFormat::TYPE_MASK)
    {
    case TextureDataFormat::U8_TYPE:     datatype = GL_UNSIGNED_BYTE; break;
    case TextureDataFormat::I8_TYPE:     datatype = GL_BYTE; break;
    case TextureDataFormat::U16_TYPE:    datatype = GL_UNSIGNED_SHORT; break;
    case TextureDataFormat::I16_TYPE:    datatype = GL_SHORT; break;
    case TextureDataFormat::U32_TYPE:    datatype = GL_UNSIGNED_INT; break;
    case TextureDataFormat::I32_TYPE:    datatype = GL_INT; break;
    case TextureDataFormat::HALF_TYPE:   datatype = GL_HALF_FLOAT; break;
    case TextureDataFormat::FLOAT_TYPE:  datatype = GL_FLOAT; break;
    }
    const bool normalized = HAS_FIELD(dformat, TextureDataFormat::NORMAL_MASK);
    switch (dformat & TextureDataFormat::RAW_FORMAT_MASK)
    {
    case TextureDataFormat::R_FORMAT:     comptype = normalized ? GL_RED  : GL_RED_INTEGER; break;
    case TextureDataFormat::RG_FORMAT:    comptype = normalized ? GL_RG   : GL_RG_INTEGER; break;
    case TextureDataFormat::RGB_FORMAT:   comptype = normalized ? GL_RGB  : GL_RGB_INTEGER; break;
    case TextureDataFormat::BGR_FORMAT:   comptype = normalized ? GL_BGR  : GL_BGR_INTEGER; break;
    case TextureDataFormat::RGBA_FORMAT:  comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case TextureDataFormat::BGRA_FORMAT:  comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    }
}
void _oglTexBase::ParseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
{
    if (HAS_FIELD(dformat, ImageDataType::FLOAT_MASK))
        datatype = GL_FLOAT;
    else
        datatype = GL_UNSIGNED_BYTE;
    switch (REMOVE_MASK(dformat, { ImageDataType::FLOAT_MASK }))
    {
    case ImageDataType::GRAY:  comptype = normalized ? GL_RED  : GL_RED_INTEGER; break;
    case ImageDataType::RA:    comptype = normalized ? GL_RG   : GL_RG_INTEGER; break;
    case ImageDataType::RGB:   comptype = normalized ? GL_RGB  : GL_RGB_INTEGER; break;
    case ImageDataType::BGR:   comptype = normalized ? GL_BGR  : GL_BGR_INTEGER; break;
    case ImageDataType::RGBA:  comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case ImageDataType::BGRA:  comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    }
}
ImageDataType _oglTexBase::ConvertFormat(const TextureDataFormat dformat) noexcept
{
    const ImageDataType isFloat = HAS_FIELD(dformat, TextureDataFormat::FLOAT_TYPE) ? ImageDataType::FLOAT_MASK : ImageDataType::EMPTY_MASK;
    switch (REMOVE_MASK(dformat, TextureDataFormat::TYPE_MASK, TextureDataFormat::NORMAL_MASK))
    {
    case TextureDataFormat::R8:     return ImageDataType::RED  | isFloat;
    case TextureDataFormat::RG8:    return ImageDataType::RA   | isFloat;
    case TextureDataFormat::RGB8:   return ImageDataType::RGB  | isFloat;
    case TextureDataFormat::RGBA8:  return ImageDataType::RGBA | isFloat;
    case TextureDataFormat::BGR8:   return ImageDataType::BGR  | isFloat;
    case TextureDataFormat::BGRA8:  return ImageDataType::BGRA | isFloat;
    }
    return isFloat;//fallback
}
size_t _oglTexBase::ParseFormatSize(const TextureDataFormat dformat) noexcept
{
    size_t size = 0;
    switch (dformat & TextureDataFormat::TYPE_MASK)
    {
    case TextureDataFormat::U8_TYPE:
    case TextureDataFormat::I8_TYPE:
        size = 8; break;
    case TextureDataFormat::U16_TYPE:
    case TextureDataFormat::I16_TYPE:
    case TextureDataFormat::HALF_TYPE:
        size = 16; break;
    case TextureDataFormat::U32_TYPE:
    case TextureDataFormat::I32_TYPE:
    case TextureDataFormat::FLOAT_TYPE:
        size = 32; break;
    default:
        return 0;
    }
    switch (dformat & TextureDataFormat::RAW_FORMAT_MASK)
    {
    case TextureDataFormat::R_FORMAT:
        size *= 1; break;
    case TextureDataFormat::RG_FORMAT:
        size *= 2; break;
    case TextureDataFormat::RGB_FORMAT:
    case TextureDataFormat::BGR_FORMAT:
        size *= 3; break;
    case TextureDataFormat::RGBA_FORMAT:
    case TextureDataFormat::BGRA_FORMAT:
        size *= 4; break;
    default:
        return 0;
    }
    return size / 8;
}


_oglTexture2D::_oglTexture2D() noexcept : _oglTexBase(TextureType::Tex2D), Width(0), Height(0)
{
    glGenTextures(1, &textureID);
}

void _oglTexture2D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    if (isSub)
        glTextureSubImage2DEXT(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, comptype, datatype, data);
    else
        glTextureImage2DEXT(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, comptype, datatype, data);
}

void _oglTexture2D::SetData(const bool isSub, const TextureDataFormat dformat, const void *data) noexcept
{
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    SetData(isSub, datatype, comptype, data);
}

void _oglTexture2D::SetData(const bool isSub, const TextureDataFormat dformat, const oglPBO& buf) noexcept
{
    buf->bind();
    SetData(isSub, dformat, nullptr);
    buf->unbind();
}

void _oglTexture2D::SetData(const bool isSub, const Image& img, const bool normalized) noexcept
{
    GLenum datatype, comptype;
    ParseFormat(img.DataType, normalized, datatype, comptype);
    SetData(isSub, datatype, comptype, img.GetRawPtr());
}

void _oglTexture2D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    if (isSub)
        glCompressedTextureSubImage2DEXT(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, (GLint)InnerFormat, (GLsizei)size, data);
    else
        glCompressedTextureImage2DEXT(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, (GLsizei)size, data);
}

void _oglTexture2D::SetCompressedData(const bool isSub, const oglPBO& buf, const size_t size) noexcept
{
    buf->bind();
    SetCompressedData(isSub, nullptr, size);
    buf->unbind();
}

optional<vector<uint8_t>> _oglTexture2D::GetCompressedData()
{
    if (!IsCompressed())
        return {};
    GLint size = 0;
    glGetTextureLevelParameterivEXT(textureID, GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    glGetCompressedTextureImageEXT(textureID, GL_TEXTURE_2D, 0, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture2D::GetData(const TextureDataFormat dformat)
{
    const auto[w, h] = GetInternalSize2();
    auto size = w * h * ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    glGetTextureImageEXT(textureID, GL_TEXTURE_2D, 0, comptype, datatype, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const TextureDataFormat dformat)
{
    const auto[w, h] = GetInternalSize2();
    Image image(ConvertFormat(dformat));
    image.SetSize(w, h);
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    glGetTextureImageEXT(textureID, (GLenum)Type, 0, comptype, datatype, image.GetRawPtr());
    return image;
}


_oglTexture2DStatic::_oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat)
{
    if (width == 0 || height == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = iformat;
    glTextureStorage2DEXT(textureID, GL_TEXTURE_2D, 1, (GLenum)InnerFormat, Width, Height);
}

void _oglTexture2DStatic::SetData(const TextureDataFormat dformat, const void *data)
{
    _oglTexture2D::SetData(true, dformat, data);
}

void _oglTexture2DStatic::SetData(const TextureDataFormat dformat, const oglPBO& buf)
{
    _oglTexture2D::SetData(true, dformat, buf);
}

void _oglTexture2DStatic::SetData(const Image & img, const bool normalized)
{
    if (img.Width != Width || img.Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"image's size msmatch with oglTex2D(S)");
    _oglTexture2D::SetData(true, img, normalized);
}

void _oglTexture2DStatic::SetCompressedData(const void *data, const size_t size)
{
    _oglTexture2D::SetCompressedData(true, data, size);
}

void _oglTexture2DStatic::SetCompressedData(const oglPBO & buf, const size_t size)
{
    _oglTexture2D::SetCompressedData(true, buf, size);
}


void _oglTexture2DDynamic::CheckAndSetMetadata(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h)
{
    if (w % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    InnerFormat = iformat, Width = w, Height = h;
}

void _oglTexture2DDynamic::SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const void *data)
{
    CheckAndSetMetadata(iformat, w, h);
    _oglTexture2D::SetData(false, dformat, data);
}

void _oglTexture2DDynamic::SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const oglPBO& buf)
{
    CheckAndSetMetadata(iformat, w, h);
    _oglTexture2D::SetData(false, dformat, buf);
}

void _oglTexture2DDynamic::SetData(const TextureInnerFormat iformat, const xziar::img::Image& img, const bool normalized)
{
    CheckAndSetMetadata(iformat, img.Width, img.Height);
    _oglTexture2D::SetData(false, img, normalized);
}

void _oglTexture2DDynamic::SetCompressedData(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h, const void *data, const size_t size)
{
    CheckAndSetMetadata(iformat, w, h);
    _oglTexture2D::SetCompressedData(false, data, size);
}

void _oglTexture2DDynamic::SetCompressedData(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h, const oglPBO& buf, const size_t size)
{
    CheckAndSetMetadata(iformat, w, h);
    _oglTexture2D::SetCompressedData(false, buf, size);
}


_oglTexture2DArray::_oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureInnerFormat iformat)
    : _oglTexBase(TextureType::Tex2DArray)
{
    if (width == 0 || height == 0 || layers == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2DArray."); 
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Layers = layers, InnerFormat = iformat;
    glTextureStorage3DEXT(textureID, GL_TEXTURE_2D_ARRAY, 1, (GLenum)InnerFormat, width, height, layers);
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const Wrapper<_oglTexture2D>& tex)
{
    if (layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    glCopyImageSubData(tex->textureID, (GLenum)tex->Type, 0, 0, 0, 0,
        textureID, (GLenum)Type, 0, 0, 0, layer,
        tex->Width, tex->Height, 1);
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const Image& img)
{
    if (img.Width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
    if (img.Width != Width || img.Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    GLenum datatype, comptype;
    ParseFormat(img.DataType, true, datatype, comptype);
    glTextureSubImage3DEXT(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, img.Width, img.Height, 1, comptype, datatype, img.GetRawPtr());
}

void _oglTexture2DArray::SetTextureLayers(const uint32_t destLayer, const Wrapper<_oglTexture2DArray>& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    if (Width != tex->Width || Height != tex->Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    if (destLayer + layerCount > Layers || srcLayer + layerCount > tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    glCopyImageSubData(tex->textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, srcLayer,
        textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, destLayer,
        tex->Width, tex->Height, layerCount);
}

Wrapper<_oglTexture2DView> _oglTexture2DArray::ViewTextureLayer(const uint32_t layer) const
{
    if(layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    oglTex2DV tex(new _oglTexture2DView(Width, Height, InnerFormat));
    glTextureView(tex->textureID, GL_TEXTURE_2D, textureID, (GLenum)InnerFormat, 0, 1, layer, 1);
    return tex;
}


_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf)
{
}

void _oglBufferTexture::SetBuffer(const TextureInnerFormat iformat, const oglTBO& tbo)
{
    InnerBuf = tbo;
    InnerFormat = iformat;
    glTextureBufferEXT(textureID, GL_TEXTURE_BUFFER, (GLenum)iformat, tbo->bufferID);
}



}