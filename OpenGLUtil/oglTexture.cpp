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

void _oglTexBase::SetProperty(const TextureFilterVal magFilter, const TextureFilterVal minFilter, const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_WRAP_S, (GLint)wrapS);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_WRAP_T, (GLint)wrapT);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
    glTextureParameteriEXT(textureID, (GLenum)Type, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
}

GLenum _oglTexBase::ParseFormat(const TextureDataFormat dformat) noexcept
{
    switch (dformat | TextureDataFormat::NORMAL_MASK)
    {
    case TextureDataFormat::R8:
        return GL_R8;
    case TextureDataFormat::RG8:    
        return GL_RG8;
    case TextureDataFormat::RGB8:
    case TextureDataFormat::BGR8:
        return GL_RGB8;
    case TextureDataFormat::RGBA8:
    case TextureDataFormat::BGRA8:
        return GL_RGBA8;
    case TextureDataFormat::Rf:
        return GL_R32F;
    case TextureDataFormat::RGf:
        return GL_RG32F;
    case TextureDataFormat::RGBf:
    case TextureDataFormat::BGRf:
        return GL_RGB32F;
    case TextureDataFormat::RGBAf:
    case TextureDataFormat::BGRAf:
        return GL_RGBA32F;
    }
    return GL_INVALID_ENUM;
}
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

void _oglTexture2D::SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void *data)
{
    if (w % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
    //bind(0);
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    //glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, data);
    glTextureImage2DEXT(textureID, (GLenum)Type, 0, (GLint)iformat, w, h, 0, comptype, datatype, data);
    InnerFormat = iformat;
    Width = w, Height = h;
    //unbind();
}

void _oglTexture2D::SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglPBO& buf)
{
    //bind(0);
    buf->bind();
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    //glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, nullptr);
    glTextureImage2DEXT(textureID, (GLenum)Type, 0, (GLint)iformat, w, h, 0, comptype, datatype, nullptr);
    InnerFormat = iformat;
    Width = w, Height = h;
    buf->unbind();
    //unbind();
}

void _oglTexture2D::SetData(const TextureInnerFormat iformat, const xziar::img::Image& img, const bool normalized)
{
    if (img.Width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
    //bind(0);
    GLenum datatype, comptype;
    ParseFormat(img.DataType, normalized, datatype, comptype);
    //glTexImage2D((GLenum)type, 0, (GLint)iformat, img.Width, img.Height, 0, comptype, datatype, img.GetRawPtr());
    glTextureImage2DEXT(textureID, (GLenum)Type, 0, (GLint)iformat, img.Width, img.Height, 0, comptype, datatype, img.GetRawPtr());
    InnerFormat = iformat;
    Width = img.Width, Height = img.Height;
}

void _oglTexture2D::SetCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const void *data, const size_t size)
{
    //bind(0);
    //glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, (GLsizei)size, data);
    glCompressedTextureImage2DEXT(textureID, (GLenum)Type, 0, (GLint)iformat, w, h, 0, (GLsizei)size, data);
    InnerFormat = iformat;
    Width = w, Height = h;
    //unbind();
}

void _oglTexture2D::SetCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const oglPBO& buf, const GLsizei size)
{
    //bind(0);
    buf->bind();
    //glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, size, nullptr);
    glCompressedTextureImage2DEXT(textureID, (GLenum)Type, 0, (GLint)iformat, w, h, 0, size, nullptr);
    InnerFormat = iformat;
    Width = w, Height = h;
    buf->unbind();
    //unbind();
}

optional<vector<uint8_t>> _oglTexture2D::GetCompressedData()
{
    //bind(0);
    GLint ret = GL_FALSE;
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED, &ret);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_COMPRESSED, &ret);
    if (ret == GL_FALSE)//non-compressed
        return {};
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &ret);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &ret);
    optional<vector<uint8_t>> output(std::in_place, ret);
    //glGetCompressedTexImage((GLenum)type, 0, (*output).data());
    glGetCompressedTextureImageEXT(textureID, (GLenum)Type, 0, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture2D::GetData(const TextureDataFormat dformat)
{
    //bind(0);
    GLint w = 0, h = 0;
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_HEIGHT, &h);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    auto size = w * h * ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    //glGetTexImage((GLenum)type, 0, comptype, datatype, output.data());
    glGetTextureImageEXT(textureID, (GLenum)Type, 0, comptype, datatype, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const TextureDataFormat dformat)
{
    //bind(0);
    GLint w = 0, h = 0;
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_WIDTH, &w);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    //glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_HEIGHT, &h);
    glGetTextureLevelParameterivEXT(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    Image image(ConvertFormat(dformat));
    image.SetSize(w, h);
    GLenum datatype, comptype;
    ParseFormat(dformat, datatype, comptype);
    //glGetTexImage((GLenum)type, 0, comptype, datatype, image.GetRawPtr());
    glGetTextureImageEXT(textureID, (GLenum)Type, 0, comptype, datatype, image.GetRawPtr());
    return image;
}


_oglTexture2DArray::_oglTexture2DArray() noexcept : _oglTexBase(TextureType::Tex2DArray), Width(0), Height(0), Layers(0)
{
}

_oglTexture2DArray::_oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureDataFormat dformat) noexcept
    : _oglTexture2DArray()
{
    InitSize(width, height, layers, dformat);
}

void _oglTexture2DArray::InitSize(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureDataFormat dformat)
{
    if (Width != 0 || Height != 0 || Layers != 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Repeat set size of Tex2DArray.");
    if(width == 0 || height == 0 || layers == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2DArray.");
    Width = width, Height = height, Layers = layers;
    bind(0);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, ParseFormat(dformat), width, height, layers);
    unbind();
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const Wrapper<detail::_oglTexture2D>& tex)
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

void _oglTexture2DArray::SetTextureLayers(const uint32_t destLayer, const Wrapper<detail::_oglTexture2DArray>& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    if (Width != tex->Width || Height != tex->Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    if (destLayer + layerCount > Layers || srcLayer + layerCount > tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    glCopyImageSubData(tex->textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, srcLayer,
        textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, destLayer,
        tex->Width, tex->Height, layerCount);
}


_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf)
{
}

void _oglBufferTexture::setBuffer(const TextureDataFormat dformat, const oglTBO& tbo)
{
    //bind(0);
    innerBuf = tbo;
    //glTexBuffer(GL_TEXTURE_BUFFER, ParseFormat(dformat), tbo->bufferID);
    glTextureBufferEXT(textureID, GL_TEXTURE_BUFFER, ParseFormat(dformat), tbo->bufferID);
    //unbind();
}



}