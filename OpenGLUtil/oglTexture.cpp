#include "oglRely.h"
#include "oglTexture.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"

namespace oglu
{
using xziar::img::ImageDataType;
using xziar::img::Image;

void TexFormatUtil::ParseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept
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
    case TextureDataFormat::R_FORMAT:     comptype = normalized ? GL_RED : GL_RED_INTEGER; break;
    case TextureDataFormat::RG_FORMAT:    comptype = normalized ? GL_RG : GL_RG_INTEGER; break;
    case TextureDataFormat::RGB_FORMAT:   comptype = normalized ? GL_RGB : GL_RGB_INTEGER; break;
    case TextureDataFormat::BGR_FORMAT:   comptype = normalized ? GL_BGR : GL_BGR_INTEGER; break;
    case TextureDataFormat::RGBA_FORMAT:  comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case TextureDataFormat::BGRA_FORMAT:  comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    }
}
void TexFormatUtil::ParseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
{
    if (HAS_FIELD(dformat, ImageDataType::FLOAT_MASK))
        datatype = GL_FLOAT;
    else
        datatype = GL_UNSIGNED_BYTE;
    switch (REMOVE_MASK(dformat, { ImageDataType::FLOAT_MASK }))
    {
    case ImageDataType::GRAY:  comptype = normalized ? GL_RED : GL_RED_INTEGER; break;
    case ImageDataType::RA:    comptype = normalized ? GL_RG : GL_RG_INTEGER; break;
    case ImageDataType::RGB:   comptype = normalized ? GL_RGB : GL_RGB_INTEGER; break;
    case ImageDataType::BGR:   comptype = normalized ? GL_BGR : GL_BGR_INTEGER; break;
    case ImageDataType::RGBA:  comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case ImageDataType::BGRA:  comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    }
}

TextureDataFormat TexFormatUtil::DecideFormat(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::BC4:
    case TextureInnerFormat::R8:        return TextureDataFormat::R8;
    case TextureInnerFormat::BC5:
    case TextureInnerFormat::RG8:       return TextureDataFormat::RG8;
    case TextureInnerFormat::SRGB8:  
    case TextureInnerFormat::BC1:
    case TextureInnerFormat::BC1SRGB:
    case TextureInnerFormat::RGB8:      return TextureDataFormat::RGB8;
    case TextureInnerFormat::SRGBA8:    
    case TextureInnerFormat::BC1A:
    case TextureInnerFormat::BC1ASRGB:
    case TextureInnerFormat::BC2:
    case TextureInnerFormat::BC3:
    case TextureInnerFormat::BC7:
    case TextureInnerFormat::BC7SRGB:
    case TextureInnerFormat::RGBA8:     return TextureDataFormat::RGBA8;
    case TextureInnerFormat::R8U:       return TextureDataFormat::R8U;
    case TextureInnerFormat::RG8U:      return TextureDataFormat::RG8U;
    case TextureInnerFormat::RGB8U:     return TextureDataFormat::RGB8U;
    case TextureInnerFormat::RGBA8U:    return TextureDataFormat::RGBA8U;
    case TextureInnerFormat::Rh:        return TextureDataFormat::Rh;
    case TextureInnerFormat::RGh:       return TextureDataFormat::RGh;
    case TextureInnerFormat::RG11B10:
    case TextureInnerFormat::RGBh:      return TextureDataFormat::RGBh;
    case TextureInnerFormat::RGB10A2:
    case TextureInnerFormat::RGBAh:     return TextureDataFormat::RGBAh;
    case TextureInnerFormat::Rf:        return TextureDataFormat::Rf;
    case TextureInnerFormat::RGf:       return TextureDataFormat::RGf;
    case TextureInnerFormat::BC6H:
    case TextureInnerFormat::RGBf:      return TextureDataFormat::RGBf;
    case TextureInnerFormat::RGBAf:     return TextureDataFormat::RGBAf;
    default:                            return static_cast<TextureDataFormat>(0xff);
    }
}

ImageDataType TexFormatUtil::ConvertFormat(const TextureDataFormat dformat) noexcept
{
    const ImageDataType isFloat = HAS_FIELD(dformat, TextureDataFormat::FLOAT_TYPE) ? ImageDataType::FLOAT_MASK : ImageDataType::EMPTY_MASK;
    switch (REMOVE_MASK(dformat, TextureDataFormat::TYPE_MASK, TextureDataFormat::NORMAL_MASK))
    {
    case TextureDataFormat::R8:     return ImageDataType::RED | isFloat;
    case TextureDataFormat::RG8:    return ImageDataType::RA | isFloat;
    case TextureDataFormat::RGB8:   return ImageDataType::RGB | isFloat;
    case TextureDataFormat::RGBA8:  return ImageDataType::RGBA | isFloat;
    case TextureDataFormat::BGR8:   return ImageDataType::BGR | isFloat;
    case TextureDataFormat::BGRA8:  return ImageDataType::BGRA | isFloat;
    }
    return isFloat;//fallback
}

TextureDataFormat TexFormatUtil::ConvertFormat(const ImageDataType dtype, const bool normalized) noexcept
{
    TextureDataFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ? TextureDataFormat::FLOAT_TYPE : TextureDataFormat::U8_TYPE;
    if (normalized) 
        baseFormat |= TextureDataFormat::NORMAL_MASK;
    switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::RGB:    return TextureDataFormat::RGB_FORMAT | baseFormat;
    case ImageDataType::BGR:    return TextureDataFormat::BGR_FORMAT | baseFormat;
    case ImageDataType::RGBA:   return TextureDataFormat::RGBA_FORMAT | baseFormat;
    case ImageDataType::BGRA:   return TextureDataFormat::BGRA_FORMAT | baseFormat;
    case ImageDataType::GRAY:   return TextureDataFormat::R_FORMAT | baseFormat;
    case ImageDataType::GA:     return TextureDataFormat::RG_FORMAT | baseFormat;
    }
    return baseFormat;//fallback
}

size_t TexFormatUtil::ParseFormatSize(const TextureDataFormat dformat) noexcept
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

bool TexFormatUtil::IsCompressType(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::BC1:
    case TextureInnerFormat::BC1A:
    case TextureInnerFormat::BC2:
    case TextureInnerFormat::BC3:
    case TextureInnerFormat::BC4:
    case TextureInnerFormat::BC5:
    case TextureInnerFormat::BC6H:
    case TextureInnerFormat::BC7:
    case TextureInnerFormat::BC1ASRGB:
    case TextureInnerFormat::BC2SRGB:
    case TextureInnerFormat::BC3SRGB:
    case TextureInnerFormat::BC7SRGB:
        return true;
    default:
        return false;
    }
}

bool TexFormatUtil::IsGrayType(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::R8:
    case TextureInnerFormat::RG8:
    case TextureInnerFormat::R8U:
    case TextureInnerFormat::RG8U:
    case TextureInnerFormat::Rh:
    case TextureInnerFormat::RGh:
    case TextureInnerFormat::Rf:
    case TextureInnerFormat::RGf:
    case TextureInnerFormat::BC4:
    case TextureInnerFormat::BC5:
        return true;
    default:
        return false;
    }
}

bool TexFormatUtil::HasAlphaType(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::RG8:
    case TextureInnerFormat::RGBA8:
    case TextureInnerFormat::SRGBA8:
    case TextureInnerFormat::RG8U:
    case TextureInnerFormat::RGBA8U:
    case TextureInnerFormat::RGh:
    case TextureInnerFormat::RGBAh:
    case TextureInnerFormat::RGf:
    case TextureInnerFormat::RGBAf:
    case TextureInnerFormat::BC1A:
    case TextureInnerFormat::BC2:
    case TextureInnerFormat::BC3:
    case TextureInnerFormat::BC7:
    case TextureInnerFormat::BC1ASRGB:
    case TextureInnerFormat::BC2SRGB:
    case TextureInnerFormat::BC3SRGB:
    case TextureInnerFormat::BC7SRGB:
    case TextureInnerFormat::RGB10A2:
        return true;
    default:
        return false;
    }
}

const char16_t* TexFormatUtil::GetTypeName(const TextureType type)
{
    switch (type)
    {
    case TextureType::Tex2D:             return u"Tex2D";
    case TextureType::Tex2DArray:        return u"Tex2DArray";
    case TextureType::TexBuf:            return u"TexBuffer";
    default:                             return u"Wrong";
    }
}

const char16_t* TexFormatUtil::GetFormatName(const TextureInnerFormat format)
{
    switch (format)
    {
    case TextureInnerFormat::BC1:        return u"BC1";
    case TextureInnerFormat::BC2:        return u"BC2";
    case TextureInnerFormat::BC3:        return u"BC3";
    case TextureInnerFormat::BC4:        return u"BC4";
    case TextureInnerFormat::BC5:        return u"BC5";
    case TextureInnerFormat::BC6H:       return u"BC6H";
    case TextureInnerFormat::BC7:        return u"BC7";
    case TextureInnerFormat::BC1A:       return u"BC1A";
    case TextureInnerFormat::BC1SRGB:    return u"BC1SRGB";
    case TextureInnerFormat::BC1ASRGB:   return u"BC1ASRGB";
    case TextureInnerFormat::BC3SRGB:    return u"BC3SRGB";
    case TextureInnerFormat::BC7SRGB:    return u"BC7SRGB";
    case TextureInnerFormat::R8:         return u"Gray8";
    case TextureInnerFormat::RG8:        return u"RG8";
    case TextureInnerFormat::RGB8:       return u"RGB8";
    case TextureInnerFormat::SRGB8:      return u"sRGB8";
    case TextureInnerFormat::RGBA8:      return u"RGBA8";
    case TextureInnerFormat::SRGBA8:     return u"sRGBA8";
    case TextureInnerFormat::Rh:         return u"Rh";
    case TextureInnerFormat::RGh:        return u"RGh";
    case TextureInnerFormat::RGBh:       return u"RGBh";
    case TextureInnerFormat::RGBAh:      return u"RGBAh";
    case TextureInnerFormat::Rf:         return u"Rf";
    case TextureInnerFormat::RGf:        return u"RGf";
    case TextureInnerFormat::RGBf:       return u"RGBf";
    case TextureInnerFormat::RGBAf:      return u"RGBAf";
    case TextureInnerFormat::RG11B10:    return u"RG11B10";
    case TextureInnerFormat::RGB10A2:    return u"RGB10A2";
    default:                             return u"Other";
    }
}


namespace detail
{

struct TexLogItem
{
    uint64_t ThreadId;
    GLuint TexId;
    TextureType TexType;
    TexLogItem(const _oglTexBase& tex) : ThreadId(common::ThreadObject::GetCurrentThreadId()), TexId(tex.textureID), TexType(tex.Type) {}
    bool operator<(const TexLogItem& other) { return TexId < other.TexId; }
};

static ContextResource<std::shared_ptr<TextureManager>, false> CTX_TEX_MAN;
static ContextResource<std::shared_ptr<map<GLuint, TexLogItem>>, true> CTX_TEX_LOG;
static std::atomic_flag CTX_TEX_LOG_LOCK = { 0 };

static void RegistTexture(const _oglTexBase& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] create texture [{}], type[{}].\n", item.ThreadId, item.TexId, TexFormatUtil::GetTypeName(item.TexType));
#endif
    auto texmap = CTX_TEX_LOG.GetOrInsert([](const auto&) { return std::make_shared<map<GLuint, TexLogItem>>(); });
    common::SpinLocker locker(CTX_TEX_LOG_LOCK);
    texmap->insert_or_assign(item.TexId, item);
}
static void UnregistTexture(const _oglTexBase& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] delete texture [{}][{}], type[{}].\n", item.ThreadId, item.TexId, tex.Name, TexFormatUtil::GetTypeName(item.TexType));
#endif
    if (auto texmap = CTX_TEX_LOG.TryGet())
    {
        common::SpinLocker locker(CTX_TEX_LOG_LOCK);
        (*texmap)->erase(item.TexId);
    }
    else
    {
        oglLog().warning(u"delete before TexLogMap created.");
    }
}


TextureManager& _oglTexBase::getTexMan() noexcept
{
    const auto texman = CTX_TEX_MAN.GetOrInsert([](auto) { return std::make_shared<TextureManager>(); });
    return *texman;
}

_oglTexBase::_oglTexBase(const TextureType type) noexcept : Type(type)
{
    glGenTextures(1, &textureID);
    RegistTexture(*this);
}

_oglTexBase::~_oglTexBase() noexcept
{
    UnregistTexture(*this);
    //force unbind texture, since texID may be reused after releasaed
    getTexMan().forcePop(textureID);
    glDeleteTextures(1, &textureID);
}

void _oglTexBase::bind(const uint16_t pos) const noexcept
{
    glBindMultiTextureEXT(GL_TEXTURE0 + pos, (GLenum)Type, textureID);
    //glActiveTexture(GL_TEXTURE0 + pos);
    //glBindTexture((GLenum)Type, textureID);
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

void _oglTexture2D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    if (isSub)
        glTextureSubImage2DEXT(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, comptype, datatype, data);
    else
        glTextureImage2DEXT(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, comptype, datatype, data);
}

void _oglTexture2D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    if (isSub)
        glCompressedTextureSubImage2DEXT(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, (GLint)InnerFormat, (GLsizei)size, data);
    else
        glCompressedTextureImage2DEXT(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, (GLsizei)size, data);
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
    const auto size = w * h * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    glGetTextureImageEXT(textureID, GL_TEXTURE_2D, 0, comptype, datatype, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const ImageDataType format, const bool flipY)
{
    const auto[w, h] = GetInternalSize2();
    Image image(format);
    image.SetSize(w, h);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(format, true);
    glGetTextureImageEXT(textureID, (GLenum)Type, 0, comptype, datatype, image.GetRawPtr());
    if (flipY)
        image.FlipVertical();
    return image;
}


_oglTexture2DStatic::_oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat, const uint8_t mipmap)
{
    if (width == 0 || height == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = iformat, Mipmap = mipmap;
    //bind(0);
    //glTexStorage2D(GL_TEXTURE_2D, mipmap, (GLenum)InnerFormat, Width, Height);
    glTextureStorage2DEXT(textureID, GL_TEXTURE_2D, mipmap, (GLenum)InnerFormat, Width, Height);
}

void _oglTexture2DStatic::SetData(const TextureDataFormat dformat, const void *data)
{
    _oglTexture2D::SetData(true, dformat, data);
}

void _oglTexture2DStatic::SetData(const TextureDataFormat dformat, const oglPBO& buf)
{
    _oglTexture2D::SetData(true, dformat, buf);
}

void _oglTexture2DStatic::SetData(const Image & img, const bool normalized, const bool flipY)
{
    if (img.Width != Width || img.Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"image's size msmatch with oglTex2D(S)");
    _oglTexture2D::SetData(true, img, normalized, flipY);
}

void _oglTexture2DStatic::SetCompressedData(const void *data, const size_t size)
{
    _oglTexture2D::SetCompressedData(true, data, size);
}

void _oglTexture2DStatic::SetCompressedData(const oglPBO & buf, const size_t size)
{
    _oglTexture2D::SetCompressedData(true, buf, size);
}

void _oglTexture2DStatic::GenerateMipmap()
{
    glGenerateTextureMipmapEXT(textureID, GL_TEXTURE_2D);
}

oglTex2DV _oglTexture2DStatic::GetTextureView() const
{
    oglTex2DV tex(new _oglTexture2DView(Width, Height, InnerFormat, Mipmap));
    tex->Name = Name + u"-View";
    glTextureView(tex->textureID, GL_TEXTURE_2D, textureID, (GLenum)InnerFormat, 0, Mipmap, 0, 1);
    return tex;
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

void _oglTexture2DDynamic::SetData(const TextureInnerFormat iformat, const xziar::img::Image& img, const bool normalized, const bool flipY)
{
    CheckAndSetMetadata(iformat, img.Width, img.Height);
    _oglTexture2D::SetData(false, img, normalized, flipY);
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


_oglTexture2DArray::_oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureInnerFormat iformat, const uint8_t mipmap)
    : _oglTexBase(TextureType::Tex2DArray)
{
    if (width == 0 || height == 0 || layers == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2DArray.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Layers = layers, InnerFormat = iformat, Mipmap = mipmap;
    //bind(0);
    //glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, (GLenum)InnerFormat, width, height, layers);
    glTextureStorage3DEXT(textureID, GL_TEXTURE_2D_ARRAY, 1, (GLenum)InnerFormat, width, height, layers);
}

_oglTexture2DArray::_oglTexture2DArray(const Wrapper<_oglTexture2DArray>& old, const uint32_t layerAdd) :
    _oglTexture2DArray(old->Width, old->Height, old->Layers + layerAdd, old->InnerFormat)
{
    SetTextureLayers(0, old, 0, old->Layers);
}

void _oglTexture2DArray::CheckLayerRange(const uint32_t layer) const
{
    if (layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const oglTex2D& tex)
{
    CheckLayerRange(layer);
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"too few mipmap level");
    if (tex->InnerFormat != InnerFormat)
        oglLog().warning(u"tex[{}][{}] has different innerFormat with texarr[{}][{}].\n", tex->textureID, (uint32_t)tex->InnerFormat, textureID, (uint32_t)InnerFormat);
    for (uint8_t i = 0; i < Mipmap; ++i)
    {
        glCopyImageSubData(tex->textureID, (GLenum)tex->Type, i, 0, 0, 0,
            textureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, layer,
            tex->Width, tex->Height, 1);
    }
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const Image& img, const bool flipY)
{
    if (img.Width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
    if (img.Width != Width || img.Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch"); 
    SetTextureLayer(layer, TexFormatUtil::ConvertFormat(img.DataType, true), flipY ? img.FlipToVertical().GetRawPtr() : img.GetRawPtr());
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const TextureDataFormat dformat, const void *data)
{
    CheckLayerRange(layer);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    glTextureSubImage3DEXT(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, comptype, datatype, data);
}

void _oglTexture2DArray::SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size)
{
    CheckLayerRange(layer);
    glCompressedTextureSubImage3DEXT(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, (GLint)InnerFormat, (GLsizei)size, data);
}

void _oglTexture2DArray::SetTextureLayers(const uint32_t destLayer, const oglTex2DArray& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    if (Width != tex->Width || Height != tex->Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture size mismatch");
    if (destLayer + layerCount > Layers || srcLayer + layerCount > tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"too few mipmap level");
    for (uint8_t i = 0; i < Mipmap; ++i)
    {
        glCopyImageSubData(tex->textureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, srcLayer,
            textureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, destLayer,
            tex->Width, tex->Height, layerCount);
    }
}

oglTex2DV _oglTexture2DArray::ViewTextureLayer(const uint32_t layer) const
{
    if (layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"layer range outflow");
    oglTex2DV tex(new _oglTexture2DView(Width, Height, InnerFormat, Mipmap));
    const auto layerStr = std::to_string(layer);
    tex->Name = Name + u"-Layer" + u16string(layerStr.cbegin(), layerStr.cend());
    glTextureView(tex->textureID, GL_TEXTURE_2D, textureID, (GLenum)InnerFormat, 0, Mipmap, layer, 1);
    return tex;
}


void _oglTexture3D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    if (isSub)
        glTextureSubImage3DEXT(textureID, GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, comptype, datatype, data);
    else
        glTextureImage3DEXT(textureID, GL_TEXTURE_3D, 0, (GLint)InnerFormat, Width, Height, Depth, 0, comptype, datatype, data);
}

void _oglTexture3D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    if (isSub)
        glCompressedTextureSubImage2DEXT(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, (GLint)InnerFormat, (GLsizei)size, data);
    else
        glCompressedTextureImage2DEXT(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture3D::GetCompressedData()
{
    if (!IsCompressed())
        return {};
    GLint size = 0;
    glGetTextureLevelParameterivEXT(textureID, GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    glGetCompressedTextureImageEXT(textureID, GL_TEXTURE_2D, 0, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture3D::GetData(const TextureDataFormat dformat)
{
    const auto[w, h] = GetInternalSize2();
    const auto size = w * h * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    glGetTextureImageEXT(textureID, GL_TEXTURE_2D, 0, comptype, datatype, output.data());
    return output;
}

_oglTexture3DStatic::_oglTexture3DStatic(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureInnerFormat iformat, const uint8_t mipmap)
{
    if (width == 0 || height == 0 || depth == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Depth = depth, InnerFormat = iformat, Mipmap = mipmap;
    glTextureStorage3DEXT(textureID, GL_TEXTURE_3D, mipmap, (GLenum)InnerFormat, Width, Height, Depth);
}

void _oglTexture3DStatic::SetData(const TextureDataFormat dformat, const void *data)
{
    _oglTexture3D::SetData(true, dformat, data);
}

void _oglTexture3DStatic::SetData(const TextureDataFormat dformat, const oglPBO& buf)
{
    _oglTexture3D::SetData(true, dformat, buf);
}

void _oglTexture3DStatic::SetData(const Image & img, const bool normalized, const bool flipY)
{
    if (img.Width != Width || img.Height != Height * Depth)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"image's size msmatch with oglTex3D(S)");
    _oglTexture3D::SetData(true, img, normalized, flipY);
}

void _oglTexture3DStatic::SetCompressedData(const void *data, const size_t size)
{
    _oglTexture3D::SetCompressedData(true, data, size);
}

void _oglTexture3DStatic::SetCompressedData(const oglPBO & buf, const size_t size)
{
    _oglTexture3D::SetCompressedData(true, buf, size);
}

void _oglTexture3DStatic::GenerateMipmap()
{
    glGenerateTextureMipmapEXT(textureID, GL_TEXTURE_3D);
}
//
//oglTex2DV _oglTexture2DStatic::GetTextureView() const
//{
//    oglTex2DV tex(new _oglTexture2DView(Width, Height, InnerFormat, Mipmap));
//    tex->Name = Name + u"-View";
//    glTextureView(tex->textureID, GL_TEXTURE_2D, textureID, (GLenum)InnerFormat, 0, Mipmap, 0, 1);
//    return tex;
//}


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

}