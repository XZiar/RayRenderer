#include "oglPch.h"
#include "oglTexture.h"
#include "oglContext.h"
#include "oglException.h"
#include "oglUtil.h"
#include "BindingManager.h"
#include "DSAWrapper.h"

namespace oglu
{
using std::string;
using std::u16string;
using std::u16string_view;
using std::optional;
using std::vector;
using xziar::img::ImageDataType;
using xziar::img::TextureFormat;
using xziar::img::TexFormatUtil;
using xziar::img::Image;


namespace detail
{

struct TexLogItem
{
    uint64_t ThreadId;
    GLuint TexId;
    TextureType TexType;
    TextureFormat InnerFormat;
    TexLogItem(const _oglTexBase& tex) : 
        ThreadId(common::ThreadObject::GetCurrentThreadId()), TexId(tex.textureID), 
        TexType(tex.Type), InnerFormat(tex.InnerFormat) { }
    bool operator<(const TexLogItem& other) const { return TexId < other.TexId; }
};
struct TexLogMap
{
    std::map<GLuint, TexLogItem> TexMap;
    std::atomic_flag MapLock = { }; 
    TexLogMap() {}
    TexLogMap(TexLogMap&& other) noexcept
    {
        common::SpinLocker locker(MapLock);
        common::SpinLocker locker2(other.MapLock);
        TexMap.swap(other.TexMap);
    }
    void Insert(TexLogItem&& item) 
    {
        common::SpinLocker locker(MapLock);
        TexMap.insert_or_assign(item.TexId, item);
    }
    void Remove(const GLuint texId) 
    {
        common::SpinLocker locker(MapLock);
        TexMap.erase(texId);
    }
};

struct TLogCtxConfig : public CtxResConfig<false, TexLogMap>
{
    TexLogMap Construct() const { return {}; }
};
static TLogCtxConfig TEXLOG_CTXCFG;

static void RegistTexture(const _oglTexBase& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] create texture [{}], type[{}].\n", item.ThreadId, item.TexId, OGLTexUtil::GetTypeName(item.TexType));
#endif
    oglContext::CurrentContext()->GetOrCreate<true>(TEXLOG_CTXCFG).Insert(std::move(item));
}
static void UnregistTexture(const _oglTexBase& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] delete texture [{}][{}], type[{}], detail[{}].\n", item.ThreadId, item.TexId, tex.Name,
        OGLTexUtil::GetTypeName(item.TexType), TexFormatUtil::GetFormatName(item.InnerFormat));
#endif
    oglContext::CurrentContext()->GetOrCreate<true>(TEXLOG_CTXCFG).Remove(item.TexId);
}


struct TManCtxConfig : public CtxResConfig<false, TextureManager>
{
    TextureManager Construct() const { return {}; }
};
static TManCtxConfig TEXMAN_CTXCFG;

TextureManager& _oglTexBase::getTexMan() noexcept
{
    return oglContext::CurrentContext()->GetOrCreate<false>(TEXMAN_CTXCFG);
}

_oglTexBase::_oglTexBase(const TextureType type, const bool shouldBindType) noexcept :
    Type(type), InnerFormat(TextureFormat::ERROR), textureID(GL_INVALID_INDEX), Mipmap(1)
{
    if (shouldBindType)
        DSA->ogluCreateTextures(common::enum_cast(Type), 1, &textureID);
    else
        glGenTextures(1, &textureID);
    if (const auto e = oglUtil::GetError(); e.has_value())
        oglLog().warning(u"oglTexBase occurs error due to {}.\n", e.value());
    /*if (shouldBindType)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture((GLenum)Type, textureID);
    }*/
    RegistTexture(*this);
}

_oglTexBase::~_oglTexBase() noexcept
{
    if (!EnsureValid()) return;
    UnregistTexture(*this);
    //force unbind texture, since texID may be reused after releasaed
    getTexMan().forcePop(textureID);
    glDeleteTextures(1, &textureID);
}

void _oglTexBase::bind(const uint16_t pos) const noexcept
{
    CheckCurrent();
    DSA->ogluBindTextureUnit(pos, common::enum_cast(Type), textureID);
    //glBindMultiTextureEXT(GL_TEXTURE0 + pos, (GLenum)Type, textureID);
    //glActiveTexture(GL_TEXTURE0 + pos);
    //glBindTexture((GLenum)Type, textureID);
}

void _oglTexBase::unbind() const noexcept
{
    //glBindTexture((GLenum)Type, 0);
}

void _oglTexBase::CheckMipmapLevel(const uint8_t level) const
{
    if (level >= Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"set data of a level exceeds texture's mipmap range.");
}

std::pair<uint32_t, uint32_t> _oglTexBase::GetInternalSize2() const
{
    CheckCurrent();
    GLint w = 0, h = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_HEIGHT, &h);
    return { (uint32_t)w,(uint32_t)h };
}

std::tuple<uint32_t, uint32_t, uint32_t> _oglTexBase::GetInternalSize3() const
{
    CheckCurrent();
    GLint w = 0, h = 0, z = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_HEIGHT, &h);
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_DEPTH, &z);
    if (const auto e = oglUtil::GetError(); e.has_value())
    {
        oglLog().warning(u"GetInternalSize3 occurs error due to {}.\n", e.value());
    }
    return { (uint32_t)w,(uint32_t)h,(uint32_t)z };
}

static GLint ParseWrap(const TextureWrapVal val)
{
    switch (val)
    {
    case TextureWrapVal::Repeat:        return GL_REPEAT;
    case TextureWrapVal::ClampEdge:     return GL_CLAMP_TO_EDGE;
    case TextureWrapVal::ClampBorder:   return GL_CLAMP_TO_BORDER;
    };
    COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Unknown texture wrap value.");
}

void _oglTexBase::SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    CheckCurrent();
    const auto valS = ParseWrap(wrapS), valT = ParseWrap(wrapT);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_WRAP_S, valS);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_WRAP_T, valT);
}

void _oglTexBase::SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR)
{
    CheckCurrent();
    const auto valS = ParseWrap(wrapS), valT = ParseWrap(wrapT), valR = ParseWrap(wrapR);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_WRAP_S, valS);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_WRAP_T, valT);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_WRAP_R, valR);
}

static GLint ConvertFilterVal(const TextureFilterVal filter) noexcept
{
    switch (filter & TextureFilterVal::LAYER_MASK)
    {
    case TextureFilterVal::Linear:
        switch (filter & TextureFilterVal::MIPMAP_MASK)
        {
        case TextureFilterVal::LinearMM:    return GL_LINEAR_MIPMAP_LINEAR;
        case TextureFilterVal::NearestMM:   return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilterVal::NoneMM:      return GL_LINEAR;
        default:                            return 0;
        }
    case TextureFilterVal::Nearest:
        switch (filter & TextureFilterVal::MIPMAP_MASK)
        {
        case TextureFilterVal::LinearMM:    return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilterVal::NearestMM:   return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilterVal::NoneMM:      return GL_NEAREST;
        default:                            return 0;
        }
    default:                                return 0;
    }
}

void _oglTexBase::SetProperty(const TextureFilterVal magFilter, TextureFilterVal minFilter)
{
    CheckCurrent();
    //if (Mipmap <= 1)
    //    minFilter = REMOVE_MASK(minFilter, TextureFilterVal::MIPMAP_MASK);
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_MAG_FILTER, ConvertFilterVal(REMOVE_MASK(magFilter, TextureFilterVal::MIPMAP_MASK)));
    DSA->ogluTextureParameteri(textureID, common::enum_cast(Type), GL_TEXTURE_MIN_FILTER, ConvertFilterVal(minFilter));
}

void _oglTexBase::Clear(const TextureFormat format)
{
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    for (int32_t level = 0; level < Mipmap; ++level)
        glClearTexImage(textureID, level, datatype, comptype, nullptr);
}

bool _oglTexBase::IsCompressed() const
{
    CheckCurrent();
    GLint ret = GL_FALSE;
    DSA->ogluGetTextureLevelParameteriv(textureID, common::enum_cast(Type), 0, GL_TEXTURE_COMPRESSED, &ret);
    return ret != GL_FALSE;
}

void _oglTexture2D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        DSA->ogluTextureSubImage2D(textureID, GL_TEXTURE_2D, level, 0, 0, Width >> level, Height >> level, comptype, datatype, data);
    else
        DSA->ogluTextureImage2D(textureID, GL_TEXTURE_2D, level, (GLint)OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, 0, comptype, datatype, data);
}

void _oglTexture2D::SetCompressedData(const bool isSub, const void * data, const size_t size, const uint8_t level) noexcept
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        DSA->ogluCompressedTextureSubImage2D(textureID, GL_TEXTURE_2D, level, 0, 0, Width >> level, Height >> level, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage2D(textureID, GL_TEXTURE_2D, level, OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture2D::GetCompressedData(const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (!IsCompressed())
        return {};
    GLint size = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    DSA->ogluGetCompressedTextureImage(textureID, GL_TEXTURE_2D, level, size, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture2D::GetData(const TextureFormat format, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    const auto size = (w * h * xziar::img::TexFormatUtil::BitPerPixel(format) / 8) >> (level * 2);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, false);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_2D, level, comptype, datatype, size, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const ImageDataType format, const bool flipY, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    Image image(format);
    image.SetSize(w >> level, h >> level);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_2D, level, comptype, datatype, image.GetSize(), image.GetRawPtr());
    if (flipY)
        image.FlipVertical();
    return image;
}


_oglTexture2DView::_oglTexture2DView(const _oglTexture2DStatic& tex, const TextureFormat format) : _oglTexture2D(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = format;
    Name = tex.Name + u"-View";
    glTextureView(textureID, GL_TEXTURE_2D, tex.textureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, 0, 1);
}
_oglTexture2DView::_oglTexture2DView(const _oglTexture2DArray& tex, const TextureFormat format, const uint32_t layer) : _oglTexture2D(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = format;
    glTextureView(textureID, GL_TEXTURE_2D, tex.textureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, layer, 1);
}
static void CheckCompatible(const TextureFormat formatA, const TextureFormat formatB)
{
    if (formatA == formatB) return;
    const bool isCompressA = TexFormatUtil::IsCompressType(formatA), isCompressB = TexFormatUtil::IsCompressType(formatB);
    if (isCompressA != isCompressB)
        COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between compress/non-compress format", formatB, formatA);
    if(isCompressA)
    {
        if ((formatA & TextureFormat::MASK_COMPRESS_RAW) != (formatB & TextureFormat::MASK_COMPRESS_RAW))
            COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between different compressed format", formatB, formatA);
        else
            return;
    }
    else
    {
        if (TexFormatUtil::BitPerPixel(formatA) != TexFormatUtil::BitPerPixel(formatB))
            COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between different sized format", formatB, formatA);
        else
            return;
    }
}


_oglTexture2DStatic::_oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureFormat format, const uint8_t mipmap) : _oglTexture2D(true)
{
    if (width == 0 || height == 0 || mipmap == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = format, Mipmap = mipmap;
    DSA->ogluTextureStorage2D(textureID, GL_TEXTURE_2D, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), Width, Height);
}

void _oglTexture2DStatic::SetData(const TextureFormat format, const void *data, const uint8_t level)
{
    _oglTexture2D::SetData(true, format, data, level);
}

void _oglTexture2DStatic::SetData(const TextureFormat format, const oglPBO& buf, const uint8_t level)
{
    _oglTexture2D::SetData(true, format, buf, level);
}

void _oglTexture2DStatic::SetData(const Image & img, const bool normalized, const bool flipY, const uint8_t level)
{
    if (img.GetWidth() != (Width >> level) || img.GetHeight() != (Height >> level))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex2D(S)");
    _oglTexture2D::SetData(true, img, normalized, flipY, level);
}

void _oglTexture2DStatic::SetCompressedData(const void *data, const size_t size, const uint8_t level)
{
    _oglTexture2D::SetCompressedData(true, data, size, level);
}

void _oglTexture2DStatic::SetCompressedData(const oglPBO & buf, const size_t size, const uint8_t level)
{
    _oglTexture2D::SetCompressedData(true, buf, size, level);
}

oglTex2DV _oglTexture2DStatic::GetTextureView(const TextureFormat format) const
{
    CheckCurrent();
    CheckCompatible(InnerFormat, format);
    oglTex2DV tex(new _oglTexture2DView(*this, InnerFormat));
    tex->Name = Name + u"-View";
    return tex;
}


void _oglTexture2DDynamic::CheckAndSetMetadata(const TextureFormat format, const uint32_t w, const uint32_t h)
{
    CheckCurrent();
    if (w % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    InnerFormat = format, Width = w, Height = h;
}

void _oglTexture2DDynamic::GenerateMipmap()
{
    CheckCurrent();
    DSA->ogluGenerateTextureMipmap(textureID, GL_TEXTURE_2D);
}

void _oglTexture2DDynamic::SetData(const TextureFormat format, const uint32_t w, const uint32_t h, const void *data)
{
    CheckAndSetMetadata(format, w, h);
    _oglTexture2D::SetData(false, format, data, 0);
}

void _oglTexture2DDynamic::SetData(const TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf)
{
    CheckAndSetMetadata(format, w, h);
    _oglTexture2D::SetData(false, format, buf, 0);
}

void _oglTexture2DDynamic::SetData(const TextureFormat format, const xziar::img::Image& img, const bool normalized, const bool flipY)
{
    CheckAndSetMetadata(format, img.GetWidth(), img.GetHeight());
    _oglTexture2D::SetData(false, img, normalized, flipY, 0);
}

void _oglTexture2DDynamic::SetCompressedData(const TextureFormat format, const uint32_t w, const uint32_t h, const void *data, const size_t size)
{
    CheckAndSetMetadata(format, w, h);
    _oglTexture2D::SetCompressedData(false, data, size, 0);
}

void _oglTexture2DDynamic::SetCompressedData(const TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf, const size_t size)
{
    CheckAndSetMetadata(format, w, h);
    _oglTexture2D::SetCompressedData(false, buf, size, 0);
}


_oglTexture2DArray::_oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureFormat format, const uint8_t mipmap)
    : _oglTexBase(TextureType::Tex2DArray, true)
{
    if (width == 0 || height == 0 || layers == 0 || mipmap == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2DArray.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Layers = layers, InnerFormat = format, Mipmap = mipmap;
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_2D_ARRAY, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), width, height, layers);
}

_oglTexture2DArray::_oglTexture2DArray(const Wrapper<_oglTexture2DArray>& old, const uint32_t layerAdd) :
    _oglTexture2DArray(old->Width, old->Height, old->Layers + layerAdd, old->InnerFormat, old->Mipmap)
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
    CheckCurrent();
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
    if (tex->InnerFormat != InnerFormat)
        oglLog().warning(u"tex[{}][{}] has different innerFormat with texarr[{}][{}].\n", tex->textureID, (uint16_t)tex->InnerFormat, textureID, (uint16_t)InnerFormat);
    for (uint8_t i = 0; i < Mipmap; ++i)
    {
        glCopyImageSubData(tex->textureID, common::enum_cast(tex->Type), i, 0, 0, 0,
            textureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, layer,
            tex->Width >> i, tex->Height >> i, 1);
    }
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const Image& img, const bool flipY, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    if (img.GetWidth() % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"each line's should be aligned to 4 pixels");
    if (img.GetWidth() != (Width >> level) || img.GetHeight() != (Height >> level))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(img.GetDataType(), true);
    DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, img.GetWidth(), img.GetHeight(), 1, comptype, datatype, flipY ? img.FlipToVertical().GetRawPtr() : img.GetRawPtr());
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const TextureFormat format, const void *data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, Width >> level, Height >> level, 1, comptype, datatype, data);
}

void _oglTexture2DArray::SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    CheckCurrent();
    DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, Width >> level, Height >> level, 1, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
}

void _oglTexture2DArray::SetTextureLayers(const uint32_t destLayer, const oglTex2DArray& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    CheckCurrent();
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

oglTex2DV _oglTexture2DArray::ViewTextureLayer(const uint32_t layer, const TextureFormat format) const
{
    CheckCurrent();
    if (layer >= Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
    CheckCompatible(InnerFormat, format);
    oglTex2DV tex(new _oglTexture2DView(*this, InnerFormat, layer));
    const auto layerStr = std::to_string(layer);
    tex->Name = Name + u"-Layer" + u16string(layerStr.cbegin(), layerStr.cend());
    return tex;
}


void _oglTexture3D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_3D, level, 0, 0, 0, Width >> level, Height >> level, Depth >> level, comptype, datatype, data);
    else
        DSA->ogluTextureImage3D(textureID, GL_TEXTURE_3D, level, (GLint)OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, Depth >> level, 0, comptype, datatype, data);
}

void _oglTexture3D::SetCompressedData(const bool isSub, const void * data, const size_t size, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_3D, level, 0, 0, 0, Width >> level, Height >> level, Depth >> level, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage3D(textureID, GL_TEXTURE_3D, level, OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, Depth >> level, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture3D::GetCompressedData(const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (!IsCompressed())
        return {};
    GLint size = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, GL_TEXTURE_3D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    DSA->ogluGetCompressedTextureImage(textureID, GL_TEXTURE_3D, level, size, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture3D::GetData(const TextureFormat format, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h, d] = GetInternalSize3();
    const auto size = (w * h * d * xziar::img::TexFormatUtil::BitPerPixel(format) / 8) >> (level * 3);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, false);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_3D, level, comptype, datatype, size, output.data());
    return output;
}

_oglTexture3DView::_oglTexture3DView(const _oglTexture3DStatic& tex, const TextureFormat format) : _oglTexture3D(false)
{
    Width = tex.Width, Height = tex.Height, Depth = tex.Depth, Mipmap = tex.Mipmap; InnerFormat = format;
    Name = tex.Name + u"-View";
    glTextureView(textureID, GL_TEXTURE_3D, tex.textureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, 0, 1);
}

_oglTexture3DStatic::_oglTexture3DStatic(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, const uint8_t mipmap) : _oglTexture3D(true)
{
    CheckCurrent();
    if (width == 0 || height == 0 || depth == 0 || mipmap == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex3D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Depth = depth, InnerFormat = format, Mipmap = mipmap;
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_3D, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), Width, Height, Depth);
    if (const auto e = oglUtil::GetError(); e.has_value())
        oglLog().warning(u"oglTex3DS occurs error due to {}.\n", e.value());
}

void _oglTexture3DStatic::SetData(const TextureFormat format, const void *data, const uint8_t level)
{
    _oglTexture3D::SetData(true, format, data, level);
}

void _oglTexture3DStatic::SetData(const TextureFormat format, const oglPBO& buf, const uint8_t level)
{
    _oglTexture3D::SetData(true, format, buf, level);
}

void _oglTexture3DStatic::SetData(const Image & img, const bool normalized, const bool flipY, const uint8_t level)
{
    if (img.GetWidth() != Width || img.GetHeight() != Height * Depth)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex3D(S)");
    _oglTexture3D::SetData(true, img, normalized, flipY, level);
}

void _oglTexture3DStatic::SetCompressedData(const void *data, const size_t size, const uint8_t level)
{
    _oglTexture3D::SetCompressedData(true, data, size, level);
}

void _oglTexture3DStatic::SetCompressedData(const oglPBO & buf, const size_t size, const uint8_t level)
{
    _oglTexture3D::SetCompressedData(true, buf, size, level);
}

oglTex3DV _oglTexture3DStatic::GetTextureView(const TextureFormat format) const
{
    CheckCurrent();
    CheckCompatible(InnerFormat, format);
    oglTex3DV tex(new _oglTexture3DView(*this, InnerFormat));
    tex->Name = Name + u"-View";
    return tex;
}


_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf, true)
{
}

void _oglBufferTexture::SetBuffer(const TextureFormat format, const oglTBO& tbo)
{
    CheckCurrent();
    InnerBuf = tbo;
    InnerFormat = format;
    DSA->ogluTextureBuffer(textureID, GL_TEXTURE_BUFFER, OGLTexUtil::GetInnerFormat(format), tbo->BufferID);
}


struct TIManCtxConfig : public CtxResConfig<false, TexImgManager>
{
    TexImgManager Construct() const { return {}; }
};
static TIManCtxConfig TEXIMGMAN_CTXCFG;

TexImgManager& _oglImgBase::getImgMan() noexcept
{
    return oglContext::CurrentContext()->GetOrCreate<false>(TEXIMGMAN_CTXCFG);
}


_oglImgBase::_oglImgBase(const Wrapper<detail::_oglTexBase>& tex, const TexImgUsage usage, const bool isLayered)
    : InnerTex(tex), Usage(usage), IsLayered(isLayered)
{
    tex->CheckCurrent();
    if (!InnerTex)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Empty oglTex");
    const auto format = InnerTex->GetInnerFormat();
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLWrongFormatException, u"TexImg does not support compressed texture type", format);
    if ((format & TextureFormat::MASK_DTYPE_CAT) == TextureFormat::DTYPE_CAT_COMP &&
        common::MatchAny(format & TextureFormat::MASK_COMP_RAW, 
            TextureFormat::COMP_332 , TextureFormat::COMP_4444, TextureFormat::COMP_565 ,
            TextureFormat::COMP_5551))
        COMMON_THROW(OGLWrongFormatException, u"TexImg does not support some composite texture type", format);
}

GLuint _oglImgBase::GetTextureID() const noexcept { return InnerTex ? InnerTex->textureID : GL_INVALID_INDEX; }

void _oglImgBase::bind(const uint16_t pos) const noexcept
{
    CheckCurrent();
    GLenum usage = GL_INVALID_ENUM;
    switch (Usage)
    {
    case TexImgUsage::ReadOnly:     usage = GL_READ_ONLY;  break;
    case TexImgUsage::WriteOnly:    usage = GL_WRITE_ONLY; break;
    case TexImgUsage::ReadWrite:    usage = GL_READ_WRITE; break;
    // Assume won't have unexpected Usage
    }
    glBindImageTexture(pos, GetTextureID(), 0, IsLayered ? GL_TRUE : GL_FALSE, 0, usage, OGLTexUtil::GetInnerFormat(InnerTex->GetInnerFormat()));
}

void _oglImgBase::unbind() const noexcept
{
}

_oglImg2D::_oglImg2D(const Wrapper<detail::_oglTexture2D>& tex, const TexImgUsage usage) : _oglImgBase(tex, usage, false) {}

_oglImg3D::_oglImg3D(const Wrapper<detail::_oglTexture3D>& tex, const TexImgUsage usage) : _oglImgBase(tex, usage, true) {}



}


GLenum OGLTexUtil::GetInnerFormat(const TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::R8:            return GL_R8;
    case TextureFormat::RG8:           return GL_RG8;
    case TextureFormat::RGB8:          return GL_RGB8;
    case TextureFormat::SRGB8:         return GL_SRGB8;
    case TextureFormat::RGBA8:         return GL_RGBA8;
    case TextureFormat::SRGBA8:        return GL_SRGB8_ALPHA8;
    case TextureFormat::R16:           return GL_R16;
    case TextureFormat::RG16:          return GL_RG16;
    case TextureFormat::RGB16:         return GL_RGB16;
    case TextureFormat::RGBA16:        return GL_RGBA16;
    case TextureFormat::R8S:           return GL_R8_SNORM;
    case TextureFormat::RG8S:          return GL_RG8_SNORM;
    case TextureFormat::RGB8S:         return GL_RGB8_SNORM;
    case TextureFormat::RGBA8S:        return GL_RGBA8_SNORM;
    case TextureFormat::R16S:          return GL_R16_SNORM;
    case TextureFormat::RG16S:         return GL_RG16_SNORM;
    case TextureFormat::RGB16S:        return GL_RGB16_SNORM;
    case TextureFormat::RGBA16S:       return GL_RGBA16_SNORM;
    case TextureFormat::R8U:           return GL_R8UI;
    case TextureFormat::RG8U:          return GL_RG8UI;
    case TextureFormat::RGB8U:         return GL_RGB8UI;
    case TextureFormat::RGBA8U:        return GL_RGBA8UI;
    case TextureFormat::R16U:          return GL_R16UI;
    case TextureFormat::RG16U:         return GL_RG16UI;
    case TextureFormat::RGB16U:        return GL_RGB16UI;
    case TextureFormat::RGBA16U:       return GL_RGBA16UI;
    case TextureFormat::R32U:          return GL_R32UI;
    case TextureFormat::RG32U:         return GL_RG32UI;
    case TextureFormat::RGB32U:        return GL_RGB32UI;
    case TextureFormat::RGBA32U:       return GL_RGBA32UI;
    case TextureFormat::R8I:           return GL_R8I;
    case TextureFormat::RG8I:          return GL_RG8I;
    case TextureFormat::RGB8I:         return GL_RGB8I;
    case TextureFormat::RGBA8I:        return GL_RGBA8I;
    case TextureFormat::R16I:          return GL_R16I;
    case TextureFormat::RG16I:         return GL_RG16I;
    case TextureFormat::RGB16I:        return GL_RGB16I;
    case TextureFormat::RGBA16I:       return GL_RGBA16I;
    case TextureFormat::R32I:          return GL_R32I;
    case TextureFormat::RG32I:         return GL_RG32I;
    case TextureFormat::RGB32I:        return GL_RGB32I;
    case TextureFormat::RGBA32I:       return GL_RGBA32I;
    case TextureFormat::Rh:            return GL_R16F;
    case TextureFormat::RGh:           return GL_RG16F;
    case TextureFormat::RGBh:          return GL_RGB16F;
    case TextureFormat::RGBAh:         return GL_RGBA16F;
    case TextureFormat::Rf:            return GL_R32F;
    case TextureFormat::RGf:           return GL_RG32F;
    case TextureFormat::RGBf:          return GL_RGB32F;
    case TextureFormat::RGBAf:         return GL_RGBA32F;
    case TextureFormat::RG11B10:       return GL_R11F_G11F_B10F;
    case TextureFormat::RGB332:        return GL_R3_G3_B2;
    case TextureFormat::RGBA4444:      return GL_RGBA4;
    case TextureFormat::RGB5A1:        return GL_RGB5_A1;
    case TextureFormat::RGB565:        return GL_RGB565;
    case TextureFormat::RGB10A2:       return GL_RGB10_A2;
    case TextureFormat::RGB10A2U:      return GL_RGB10_A2UI;
    //case TextureFormat::RGBA12:      return GL_RGB12;
    case TextureFormat::BC1:           return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case TextureFormat::BC1A:          return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case TextureFormat::BC2:           return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case TextureFormat::BC3:           return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case TextureFormat::BC4:           return GL_COMPRESSED_RED_RGTC1;
    case TextureFormat::BC5:           return GL_COMPRESSED_RG_RGTC2;
    case TextureFormat::BC6H:          return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    case TextureFormat::BC7:           return GL_COMPRESSED_RGBA_BPTC_UNORM;
    case TextureFormat::BC1SRGB:       return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
    case TextureFormat::BC1ASRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case TextureFormat::BC2SRGB:       return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case TextureFormat::BC3SRGB:       return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case TextureFormat::BC7SRGB:       return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    default:                               return GL_INVALID_ENUM;
    }
}


bool OGLTexUtil::ParseFormat(const TextureFormat format, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept
{
    const auto category = format & TextureFormat::MASK_DTYPE_CAT;
    const auto dtype = format & TextureFormat::MASK_DTYPE_RAW;
    const bool normalized = TexFormatUtil::IsNormalized(format);
    switch (category)
    {
    case TextureFormat::DTYPE_CAT_COMPRESS:
        return REMOVE_MASK(dtype, TextureFormat::MASK_COMPRESS_SIGNESS) != TextureFormat::CPRS_BPTCf;
    case TextureFormat::DTYPE_CAT_COMP:
        switch (format & (TextureFormat::MASK_COMP_RAW | TextureFormat::MASK_CHANNEL_ORDER))
        {
        case TextureFormat::COMP_332:       datatype = GL_UNSIGNED_BYTE_3_3_2; break;
        case TextureFormat::COMP_233R:      datatype = GL_UNSIGNED_BYTE_2_3_3_REV; break;
        case TextureFormat::COMP_565:       datatype = GL_UNSIGNED_SHORT_5_6_5; break;
        case TextureFormat::COMP_565R:      datatype = GL_UNSIGNED_SHORT_5_6_5_REV; break;
        case TextureFormat::COMP_4444:      datatype = GL_UNSIGNED_SHORT_4_4_4_4; break;
        case TextureFormat::COMP_4444R:     datatype = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
        case TextureFormat::COMP_5551:      datatype = GL_UNSIGNED_SHORT_5_5_5_1; break;
        case TextureFormat::COMP_1555R:     datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
        case TextureFormat::COMP_10_2:      datatype = GL_UNSIGNED_INT_10_10_10_2; break;
        case TextureFormat::COMP_10_2R:     datatype = GL_UNSIGNED_INT_2_10_10_10_REV; break;
        case TextureFormat::COMP_11_10R:    datatype = GL_UNSIGNED_INT_10F_11F_11F_REV; break; // no non-rev
        case TextureFormat::COMP_999_5R:    datatype = GL_UNSIGNED_INT_5_9_9_9_REV; break; // no non-rev
        default:                                return false;
        }
        break;
    case TextureFormat::DTYPE_CAT_PLAIN:
        switch (format & TextureFormat::MASK_DTYPE_RAW)
        {
        case TextureFormat::DTYPE_U8:       datatype = GL_UNSIGNED_BYTE; break;
        case TextureFormat::DTYPE_I8:       datatype = GL_BYTE; break;
        case TextureFormat::DTYPE_U16:      datatype = GL_UNSIGNED_SHORT; break;
        case TextureFormat::DTYPE_I16:      datatype = GL_SHORT; break;
        case TextureFormat::DTYPE_U32:      datatype = GL_UNSIGNED_INT; break;
        case TextureFormat::DTYPE_I32:      datatype = GL_INT; break;
        case TextureFormat::DTYPE_HALF:     datatype = isUpload ? GL_FLOAT : GL_HALF_FLOAT; break;
        case TextureFormat::DTYPE_FLOAT:    datatype = GL_FLOAT; break;
        default:                                return false;
        }
        break;
    default:                                    return false;
    }
    switch (format & TextureFormat::MASK_CHANNEL)
    {
    case TextureFormat::CHANNEL_R:      comptype = normalized ? GL_RED   : GL_RED_INTEGER;   break;
    case TextureFormat::CHANNEL_G:      comptype = normalized ? GL_GREEN : GL_GREEN_INTEGER; break;
    case TextureFormat::CHANNEL_B:      comptype = normalized ? GL_BLUE  : GL_BLUE_INTEGER;  break;
    case TextureFormat::CHANNEL_A:      comptype = normalized ? GL_ALPHA : GL_ALPHA_INTEGER; break;
    case TextureFormat::CHANNEL_RG:     comptype = normalized ? GL_RG    : GL_RG_INTEGER;    break;
    case TextureFormat::CHANNEL_RGB:    comptype = normalized ? GL_RGB   : GL_RGB_INTEGER;   break;
    case TextureFormat::CHANNEL_BGR:    comptype = normalized ? GL_BGR   : GL_BGR_INTEGER;   break;
    case TextureFormat::CHANNEL_RGBA:   comptype = normalized ? GL_RGBA  : GL_RGBA_INTEGER;  break;
    case TextureFormat::CHANNEL_BGRA:   comptype = normalized ? GL_BGRA  : GL_BGRA_INTEGER;  break;
    default:                                return false;
    }
    return true;
}
void OGLTexUtil::ParseFormat(const ImageDataType format, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
{
    if (HAS_FIELD(format, ImageDataType::FLOAT_MASK))
        datatype = GL_FLOAT;
    else
        datatype = GL_UNSIGNED_BYTE;
    switch (REMOVE_MASK(format, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::GRAY:   comptype = normalized ? GL_RED  : GL_RED_INTEGER; break;
    case ImageDataType::RA:     comptype = normalized ? GL_RG   : GL_RG_INTEGER; break;
    case ImageDataType::RGB:    comptype = normalized ? GL_RGB  : GL_RGB_INTEGER; break;
    case ImageDataType::BGR:    comptype = normalized ? GL_BGR  : GL_BGR_INTEGER; break;
    case ImageDataType::RGBA:   comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case ImageDataType::BGRA:   comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    default:                    break;
    }
}

using namespace std::literals;

u16string_view OGLTexUtil::GetTypeName(const TextureType type) noexcept
{
    switch (type)
    {
    case TextureType::Tex2D:             return u"Tex2D"sv;
    case TextureType::Tex3D:             return u"Tex3D"sv;
    case TextureType::Tex2DArray:        return u"Tex2DArray"sv;
    case TextureType::TexBuf:            return u"TexBuffer"sv;
    default:                             return u"Wrong"sv;
    }
}


}