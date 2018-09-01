#include "oglRely.h"
#include "oglTexture.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"
#include "DSAWrapper.h"

namespace oglu
{
using xziar::img::ImageDataType;
using xziar::img::Image;

void TexFormatUtil::ParseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept
{
    switch (dformat & TextureDataFormat::TYPE_MASK)
    {
    case TextureDataFormat::U8_TYPE:        datatype = GL_UNSIGNED_BYTE; break;
    case TextureDataFormat::I8_TYPE:        datatype = GL_BYTE; break;
    case TextureDataFormat::U16_TYPE:       datatype = GL_UNSIGNED_SHORT; break;
    case TextureDataFormat::I16_TYPE:       datatype = GL_SHORT; break;
    case TextureDataFormat::U32_TYPE:       datatype = GL_UNSIGNED_INT; break;
    case TextureDataFormat::I32_TYPE:       datatype = GL_INT; break;
    case TextureDataFormat::HALF_TYPE:      datatype = GL_HALF_FLOAT; break;
    case TextureDataFormat::FLOAT_TYPE:     datatype = GL_FLOAT; break;
    default:                                break;
    }
    const bool normalized = HAS_FIELD(dformat, TextureDataFormat::NORMAL_MASK);
    switch (dformat & TextureDataFormat::RAW_FORMAT_MASK)
    {
    case TextureDataFormat::R_FORMAT:       comptype = normalized ? GL_RED : GL_RED_INTEGER; break;
    case TextureDataFormat::RG_FORMAT:      comptype = normalized ? GL_RG : GL_RG_INTEGER; break;
    case TextureDataFormat::RGB_FORMAT:     comptype = normalized ? GL_RGB : GL_RGB_INTEGER; break;
    case TextureDataFormat::BGR_FORMAT:     comptype = normalized ? GL_BGR : GL_BGR_INTEGER; break;
    case TextureDataFormat::RGBA_FORMAT:    comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case TextureDataFormat::BGRA_FORMAT:    comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    default:                                break;
    }
}
void TexFormatUtil::ParseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
{
    if (HAS_FIELD(dformat, ImageDataType::FLOAT_MASK))
        datatype = GL_FLOAT;
    else
        datatype = GL_UNSIGNED_BYTE;
    switch (REMOVE_MASK(dformat, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::GRAY:   comptype = normalized ? GL_RED : GL_RED_INTEGER; break;
    case ImageDataType::RA:     comptype = normalized ? GL_RG : GL_RG_INTEGER; break;
    case ImageDataType::RGB:    comptype = normalized ? GL_RGB : GL_RGB_INTEGER; break;
    case ImageDataType::BGR:    comptype = normalized ? GL_BGR : GL_BGR_INTEGER; break;
    case ImageDataType::RGBA:   comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case ImageDataType::BGRA:   comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    default:                    break;
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
    case TextureDataFormat::R_FORMAT:     return ImageDataType::RED  | isFloat;
    case TextureDataFormat::RG_FORMAT:    return ImageDataType::RA   | isFloat;
    case TextureDataFormat::RGB_FORMAT:   return ImageDataType::RGB  | isFloat;
    case TextureDataFormat::RGBA_FORMAT:  return ImageDataType::RGBA | isFloat;
    case TextureDataFormat::BGR_FORMAT:   return ImageDataType::BGR  | isFloat;
    case TextureDataFormat::BGRA_FORMAT:  return ImageDataType::BGRA | isFloat;
    default:                        return isFloat;
    }
}

TextureDataFormat TexFormatUtil::ConvertFormat(const ImageDataType dtype, const bool normalized) noexcept
{
    TextureDataFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ? TextureDataFormat::FLOAT_TYPE : TextureDataFormat::U8_TYPE;
    if (normalized) 
        baseFormat |= TextureDataFormat::NORMAL_MASK;
    switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::RGB:    return TextureDataFormat::RGB_FORMAT  | baseFormat;
    case ImageDataType::BGR:    return TextureDataFormat::BGR_FORMAT  | baseFormat;
    case ImageDataType::RGBA:   return TextureDataFormat::RGBA_FORMAT | baseFormat;
    case ImageDataType::BGRA:   return TextureDataFormat::BGRA_FORMAT | baseFormat;
    case ImageDataType::GRAY:   return TextureDataFormat::R_FORMAT    | baseFormat;
    case ImageDataType::GA:     return TextureDataFormat::RG_FORMAT   | baseFormat;
    default:                    return baseFormat;
    }
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

using namespace std::literals;

const u16string_view TexFormatUtil::GetTypeName(const TextureType type)
{
    switch (type)
    {
    case TextureType::Tex2D:             return u"Tex2D"sv;
    case TextureType::Tex2DArray:        return u"Tex2DArray"sv;
    case TextureType::TexBuf:            return u"TexBuffer"sv;
    default:                             return u"Wrong"sv;
    }
}

const u16string_view TexFormatUtil::GetFormatName(const TextureInnerFormat format)
{
    switch (format)
    {
    case TextureInnerFormat::BC1:        return u"BC1"sv;
    case TextureInnerFormat::BC2:        return u"BC2"sv;
    case TextureInnerFormat::BC3:        return u"BC3"sv;
    case TextureInnerFormat::BC4:        return u"BC4"sv;
    case TextureInnerFormat::BC5:        return u"BC5"sv;
    case TextureInnerFormat::BC6H:       return u"BC6H"sv;
    case TextureInnerFormat::BC7:        return u"BC7"sv;
    case TextureInnerFormat::BC1A:       return u"BC1A"sv;
    case TextureInnerFormat::BC1SRGB:    return u"BC1SRGB"sv;
    case TextureInnerFormat::BC1ASRGB:   return u"BC1ASRGB"sv;
    case TextureInnerFormat::BC3SRGB:    return u"BC3SRGB"sv;
    case TextureInnerFormat::BC7SRGB:    return u"BC7SRGB"sv;
    case TextureInnerFormat::R8:         return u"Gray8"sv;
    case TextureInnerFormat::RG8:        return u"RG8"sv;
    case TextureInnerFormat::RGB8:       return u"RGB8"sv;
    case TextureInnerFormat::SRGB8:      return u"sRGB8"sv;
    case TextureInnerFormat::RGBA8:      return u"RGBA8"sv;
    case TextureInnerFormat::SRGBA8:     return u"sRGBA8"sv;
    case TextureInnerFormat::Rh:         return u"Rh"sv;
    case TextureInnerFormat::RGh:        return u"RGh"sv;
    case TextureInnerFormat::RGBh:       return u"RGBh"sv;
    case TextureInnerFormat::RGBAh:      return u"RGBAh"sv;
    case TextureInnerFormat::Rf:         return u"Rf"sv;
    case TextureInnerFormat::RGf:        return u"RGf"sv;
    case TextureInnerFormat::RGBf:       return u"RGBf"sv;
    case TextureInnerFormat::RGBAf:      return u"RGBAf"sv;
    case TextureInnerFormat::RG11B10:    return u"RG11B10"sv;
    case TextureInnerFormat::RGB10A2:    return u"RGB10A2"sv;
    default:                             return u"Other"sv;
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

_oglTexBase::_oglTexBase(const TextureType type, const bool shouldBindType) noexcept : Type(type)
{
    glGenTextures(1, &textureID);
    if (shouldBindType)
        glBindTexture((GLenum)Type, textureID);
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
    DSA->ogluBindTextureUnit(pos, (GLenum)Type, textureID);
    //glBindMultiTextureEXT(GL_TEXTURE0 + pos, (GLenum)Type, textureID);
    //glActiveTexture(GL_TEXTURE0 + pos);
    //glBindTexture((GLenum)Type, textureID);
}

void _oglTexBase::unbind() const noexcept
{
    //glBindTexture((GLenum)Type, 0);
}

std::pair<uint32_t, uint32_t> _oglTexBase::GetInternalSize2() const
{
    GLint w = 0, h = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    return { (uint32_t)w,(uint32_t)h };
}

std::tuple<uint32_t, uint32_t, uint32_t> _oglTexBase::GetInternalSize3() const
{
    GLint w = 0, h = 0, z = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_DEPTH, &z);
    return { (uint32_t)w,(uint32_t)h,(uint32_t)z };
}

void _oglTexBase::SetProperty(const TextureFilterVal magFilter, const TextureFilterVal minFilter, const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_WRAP_S, (GLint)wrapS);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_WRAP_T, (GLint)wrapT);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
}

bool _oglTexBase::IsCompressed() const
{
    GLint ret = GL_FALSE;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_COMPRESSED, &ret);
    return ret != GL_FALSE;
}

void _oglTexture2D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    if (isSub)
        DSA->ogluTextureSubImage2D(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, comptype, datatype, data);
    else
        DSA->ogluTextureImage2D(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, comptype, datatype, data);
}

void _oglTexture2D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    if (isSub)
        DSA->ogluCompressedTextureSubImage2D(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, (GLint)InnerFormat, (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage2D(textureID, GL_TEXTURE_2D, 0, (GLint)InnerFormat, Width, Height, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture2D::GetCompressedData()
{
    if (!IsCompressed())
        return {};
    GLint size = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    DSA->ogluGetCompressedTextureImage(textureID, GL_TEXTURE_2D, 0, size, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture2D::GetData(const TextureDataFormat dformat)
{
    const auto[w, h] = GetInternalSize2();
    const auto size = w * h * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_2D, 0, comptype, datatype, size, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const ImageDataType format, const bool flipY)
{
    const auto[w, h] = GetInternalSize2();
    Image image(format);
    image.SetSize(w, h);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(format, true);
    DSA->ogluGetTextureImage(textureID, (GLenum)Type, 0, comptype, datatype, image.GetSize(), image.GetRawPtr());
    if (flipY)
        image.FlipVertical();
    return image;
}


_oglTexture2DStatic::_oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat, const uint8_t mipmap) : _oglTexture2D(true)
{
    if (width == 0 || height == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = iformat, Mipmap = mipmap;
    DSA->ogluTextureStorage2D(textureID, GL_TEXTURE_2D, mipmap, (GLenum)InnerFormat, Width, Height);
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
    if (img.GetWidth() != Width || img.GetHeight() != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex2D(S)");
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
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
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
    CheckAndSetMetadata(iformat, img.GetWidth(), img.GetHeight());
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
    : _oglTexBase(TextureType::Tex2DArray, true)
{
    if (width == 0 || height == 0 || layers == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2DArray.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Layers = layers, InnerFormat = iformat, Mipmap = mipmap;
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_2D_ARRAY, 1, (GLenum)InnerFormat, width, height, layers);
}

_oglTexture2DArray::_oglTexture2DArray(const Wrapper<_oglTexture2DArray>& old, const uint32_t layerAdd) :
    _oglTexture2DArray(old->Width, old->Height, old->Layers + layerAdd, old->InnerFormat)
{
    SetTextureLayers(0, old, 0, old->Layers);
}

void _oglTexture2DArray::CheckLayerRange(const uint32_t layer) const
{
    if (layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const oglTex2D& tex)
{
    CheckLayerRange(layer);
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
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
    if (img.GetWidth() % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"each line's should be aligned to 4 pixels");
    if (img.GetWidth() != Width || img.GetHeight() != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch"); 
    SetTextureLayer(layer, TexFormatUtil::ConvertFormat(img.GetDataType(), true), flipY ? img.FlipToVertical().GetRawPtr() : img.GetRawPtr());
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const TextureDataFormat dformat, const void *data)
{
    CheckLayerRange(layer);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, comptype, datatype, data);
}

void _oglTexture2DArray::SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size)
{
    CheckLayerRange(layer);
    DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, (GLint)InnerFormat, (GLsizei)size, data);
}

void _oglTexture2DArray::SetTextureLayers(const uint32_t destLayer, const oglTex2DArray& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    if (Width != tex->Width || Height != tex->Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (destLayer + layerCount > Layers || srcLayer + layerCount > tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
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
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
    oglTex2DV tex(new _oglTexture2DView(Width, Height, InnerFormat, Mipmap));
    const auto layerStr = std::to_string(layer);
    tex->Name = Name + u"-Layer" + u16string(layerStr.cbegin(), layerStr.cend());
    glTextureView(tex->textureID, GL_TEXTURE_2D, textureID, (GLenum)InnerFormat, 0, Mipmap, layer, 1);
    return tex;
}


void _oglTexture3D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    if (isSub)
        DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, comptype, datatype, data);
    else
        DSA->ogluTextureImage3D(textureID, GL_TEXTURE_3D, 0, (GLint)InnerFormat, Width, Height, Depth, 0, comptype, datatype, data);
}

void _oglTexture3D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    if (isSub)
        DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, (GLint)InnerFormat, (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage3D(textureID, GL_TEXTURE_3D, 0, (GLint)InnerFormat, Width, Height, Depth, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture3D::GetCompressedData()
{
    if (!IsCompressed())
        return {};
    GLint size = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    DSA->ogluGetCompressedTextureImage(textureID, GL_TEXTURE_2D, 0, size, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture3D::GetData(const TextureDataFormat dformat)
{
    const auto[w, h] = GetInternalSize2();
    const auto size = w * h * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_2D, 0, comptype, datatype, size, output.data());
    return output;
}

_oglTexture3DStatic::_oglTexture3DStatic(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureInnerFormat iformat, const uint8_t mipmap) : _oglTexture3D(false)
{
    if (width == 0 || height == 0 || depth == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Depth = depth, InnerFormat = iformat, Mipmap = mipmap;
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_3D, mipmap, (GLenum)InnerFormat, Width, Height, Depth);
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
    if (img.GetWidth() != Width || img.GetHeight() != Height * Depth)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex3D(S)");
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
//oglTex3DV _oglTexture3DStatic::GetTextureView() const
//{
//    oglTex3DV tex(new _oglTexture3DView(Width, Height, InnerFormat, Mipmap));
//    tex->Name = Name + u"-View";
//    glTextureView(tex->textureID, GL_TEXTURE_3D, textureID, (GLenum)InnerFormat, 0, Mipmap, 0, 1);
//    return tex;
//}


_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf, true)
{
}

void _oglBufferTexture::SetBuffer(const TextureInnerFormat iformat, const oglTBO& tbo)
{
    InnerBuf = tbo;
    InnerFormat = iformat;
    glTextureBufferEXT(textureID, GL_TEXTURE_BUFFER, (GLenum)iformat, tbo->bufferID);
}



static ContextResource<std::shared_ptr<TexImgManager>, false> CTX_TEXIMG_MAN;
TexImgManager& _oglImgBase::getImgMan() noexcept
{
    const auto texman = CTX_TEXIMG_MAN.GetOrInsert([](auto) { return std::make_shared<TexImgManager>(); });
    return *texman;
}

_oglImgBase::_oglImgBase(const Wrapper<detail::_oglTexBase>& tex, const TexImgUsage usage)
    : InnerTex(tex), Usage(usage) 
{
    if (!InnerTex)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Empty oglTex");
    if (TexFormatUtil::IsCompressType(InnerTex->GetInnerFormat()))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"TexImg does not support compressed texture type");
}

void _oglImgBase::bind(const uint16_t pos) const noexcept
{
    glBindImageTexture(pos, GetTextureID(), 0, GL_FALSE, 0, (GLenum)Usage, (GLenum)InnerTex->GetInnerFormat());
}

void _oglImgBase::unbind() const noexcept
{
}

_oglImg2D::_oglImg2D(const Wrapper<detail::_oglTexture2D>& tex, const TexImgUsage usage) : _oglImgBase(tex, usage) {}

}

}