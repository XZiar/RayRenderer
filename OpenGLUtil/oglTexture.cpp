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


namespace detail
{

struct TexLogItem
{
    uint64_t ThreadId;
    GLuint TexId;
    TextureInnerFormat InnerFormat;
    TextureType TexType;
    TexLogItem(const _oglTexBase& tex) : ThreadId(common::ThreadObject::GetCurrentThreadId()), TexId(tex.textureID), InnerFormat(tex.InnerFormat), TexType(tex.Type) {}
    bool operator<(const TexLogItem& other) { return TexId < other.TexId; }
};
struct TexLogMap
{
    std::map<GLuint, TexLogItem> TexMap;
    std::atomic_flag MapLock = { 0 };
    TexLogMap() {}
    TexLogMap(TexLogMap&& other)
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
    oglLog().verbose(u"here[{}] create texture [{}], type[{}].\n", item.ThreadId, item.TexId, TexFormatUtil::GetTypeName(item.TexType));
#endif
    oglContext::CurrentContext()->GetOrCreate<true>(TEXLOG_CTXCFG).Insert(std::move(item));
}
static void UnregistTexture(const _oglTexBase& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] delete texture [{}][{}], type[{}], detail[{}].\n", item.ThreadId, item.TexId, tex.Name,
        TexFormatUtil::GetTypeName(item.TexType), TexFormatUtil::GetFormatName(item.InnerFormat));
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

_oglTexBase::_oglTexBase(const TextureType type, const bool shouldBindType) noexcept : Type(type)
{
    glGenTextures(1, &textureID);
    if (shouldBindType)
        glBindTexture((GLenum)Type, textureID);
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
    CheckCurrent();
    GLint w = 0, h = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    return { (uint32_t)w,(uint32_t)h };
}

std::tuple<uint32_t, uint32_t, uint32_t> _oglTexBase::GetInternalSize3() const
{
    CheckCurrent();
    GLint w = 0, h = 0, z = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_WIDTH, &w);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_HEIGHT, &h);
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_DEPTH, &z);
    return { (uint32_t)w,(uint32_t)h,(uint32_t)z };
}

void _oglTexBase::SetProperty(const TextureFilterVal magFilter, const TextureFilterVal minFilter, const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    CheckCurrent();
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_WRAP_S, (GLint)wrapS);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_WRAP_T, (GLint)wrapT);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
    DSA->ogluTextureParameteri(textureID, (GLenum)Type, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
}

bool _oglTexBase::IsCompressed() const
{
    CheckCurrent();
    GLint ret = GL_FALSE;
    DSA->ogluGetTextureLevelParameteriv(textureID, (GLenum)Type, 0, GL_TEXTURE_COMPRESSED, &ret);
    return ret != GL_FALSE;
}

void _oglTexture2D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    CheckCurrent();
    if (isSub)
        DSA->ogluTextureSubImage2D(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, comptype, datatype, data);
    else
        DSA->ogluTextureImage2D(textureID, GL_TEXTURE_2D, 0, (GLint)TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height, 0, comptype, datatype, data);
}

void _oglTexture2D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    CheckCurrent();
    if (isSub)
        DSA->ogluCompressedTextureSubImage2D(textureID, GL_TEXTURE_2D, 0, 0, 0, Width, Height, TexFormatUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage2D(textureID, GL_TEXTURE_2D, 0, TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture2D::GetCompressedData()
{
    CheckCurrent();
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
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    const auto size = w * h * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat, false);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_2D, 0, comptype, datatype, size, output.data());
    return output;
}

Image _oglTexture2D::GetImage(const ImageDataType format, const bool flipY)
{
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    Image image(format);
    image.SetSize(w, h);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(format, true);
    DSA->ogluGetTextureImage(textureID, (GLenum)Type, 0, comptype, datatype, image.GetSize(), image.GetRawPtr());
    if (flipY)
        image.FlipVertical();
    return image;
}


_oglTexture2DView::_oglTexture2DView(const _oglTexture2DStatic& tex, const TextureInnerFormat iformat) : _oglTexture2D(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = iformat;
    Name = tex.Name + u"-View";
    glTextureView(textureID, GL_TEXTURE_2D, tex.textureID, TexFormatUtil::GetInnerFormat(InnerFormat), 0, Mipmap, 0, 1);
}
_oglTexture2DView::_oglTexture2DView(const _oglTexture2DArray& tex, const TextureInnerFormat iformat, const uint32_t layer) : _oglTexture2D(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = iformat;
    glTextureView(textureID, GL_TEXTURE_2D, tex.textureID, TexFormatUtil::GetInnerFormat(InnerFormat), 0, Mipmap, layer, 1);
}
static void CheckCompatible(const TextureInnerFormat formatA, const TextureInnerFormat formatB)
{
    if (formatA == formatB) return;
    const bool isCompressA = TexFormatUtil::IsCompressType(formatA), isCompressB = TexFormatUtil::IsCompressType(formatB);
    if (isCompressA != isCompressB)
        COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between compress/non-compress format", formatB, formatA);
    if(isCompressA)
    {
        if ((formatA & TextureInnerFormat::FORMAT_MASK) != (formatB & TextureInnerFormat::FORMAT_MASK))
            COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between different compressed format", formatB, formatA);
        else
            return;
    }
    else
    {
        if (TexFormatUtil::ParseFormatSize(formatA) != TexFormatUtil::ParseFormatSize(formatB))
            COMMON_THROW(OGLWrongFormatException, u"texture aliases not compatible between different sized format", formatB, formatA);
        else
            return;
    }
}


_oglTexture2DStatic::_oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat, const uint8_t mipmap) : _oglTexture2D(true)
{
    if (width == 0 || height == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = iformat, Mipmap = mipmap;
    DSA->ogluTextureStorage2D(textureID, GL_TEXTURE_2D, mipmap, TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height);
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
    CheckCurrent();
    DSA->ogluGenerateTextureMipmap(textureID, GL_TEXTURE_2D);
}

oglTex2DV _oglTexture2DStatic::GetTextureView(const TextureInnerFormat format) const
{
    CheckCurrent();
    CheckCompatible(InnerFormat, format);
    oglTex2DV tex(new _oglTexture2DView(*this, InnerFormat));
    tex->Name = Name + u"-View";
    return tex;
}


void _oglTexture2DDynamic::CheckAndSetMetadata(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h)
{
    CheckCurrent();
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
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_2D_ARRAY, 1, TexFormatUtil::GetInnerFormat(InnerFormat), width, height, layers);
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
    CheckCurrent();
    CheckLayerRange(layer);
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (tex->Mipmap < Mipmap)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
    if (tex->InnerFormat != InnerFormat)
        oglLog().warning(u"tex[{}][{}] has different innerFormat with texarr[{}][{}].\n", tex->textureID, (uint16_t)tex->InnerFormat, textureID, (uint16_t)InnerFormat);
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
    CheckCurrent();
    CheckLayerRange(layer);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(img.GetDataType(), true);
    DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, comptype, datatype, flipY ? img.FlipToVertical().GetRawPtr() : img.GetRawPtr());
}

void _oglTexture2DArray::SetTextureLayer(const uint32_t layer, const TextureDataFormat dformat, const void *data)
{
    CheckCurrent();
    CheckLayerRange(layer);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat, true);
    DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, comptype, datatype, data);
}

void _oglTexture2DArray::SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size)
{
    CheckCurrent();
    CheckLayerRange(layer);
    DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, Width, Height, 1, TexFormatUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
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

oglTex2DV _oglTexture2DArray::ViewTextureLayer(const uint32_t layer, const TextureInnerFormat format) const
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


void _oglTexture3D::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data) noexcept
{
    CheckCurrent();
    if (isSub)
        DSA->ogluTextureSubImage3D(textureID, GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, comptype, datatype, data);
    else
        DSA->ogluTextureImage3D(textureID, GL_TEXTURE_3D, 0, (GLint)TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height, Depth, 0, comptype, datatype, data);
}

void _oglTexture3D::SetCompressedData(const bool isSub, const void * data, const size_t size) noexcept
{
    CheckCurrent();
    if (isSub)
        DSA->ogluCompressedTextureSubImage3D(textureID, GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, TexFormatUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        DSA->ogluCompressedTextureImage3D(textureID, GL_TEXTURE_3D, 0, TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height, Depth, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> _oglTexture3D::GetCompressedData()
{
    CheckCurrent();
    if (!IsCompressed())
        return {};
    GLint size = 0;
    DSA->ogluGetTextureLevelParameteriv(textureID, GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    DSA->ogluGetCompressedTextureImage(textureID, GL_TEXTURE_3D, 0, size, (*output).data());
    return output;
}

vector<uint8_t> _oglTexture3D::GetData(const TextureDataFormat dformat)
{
    CheckCurrent();
    const auto[w, h, d] = GetInternalSize3();
    const auto size = w * h * d * TexFormatUtil::ParseFormatSize(dformat);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat, false);
    DSA->ogluGetTextureImage(textureID, GL_TEXTURE_3D, 0, comptype, datatype, size, output.data());
    return output;
}

_oglTexture3DStatic::_oglTexture3DStatic(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureInnerFormat iformat, const uint8_t mipmap) : _oglTexture3D(false)
{
    CheckCurrent();
    if (width == 0 || height == 0 || depth == 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Depth = depth, InnerFormat = iformat, Mipmap = mipmap;
    DSA->ogluTextureStorage3D(textureID, GL_TEXTURE_3D, mipmap, TexFormatUtil::GetInnerFormat(InnerFormat), Width, Height, Depth);
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
    CheckCurrent();
    DSA->ogluGenerateTextureMipmap(textureID, GL_TEXTURE_3D);
}


_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf, true)
{
}

void _oglBufferTexture::SetBuffer(const TextureInnerFormat iformat, const oglTBO& tbo)
{
    CheckCurrent();
    InnerBuf = tbo;
    InnerFormat = iformat;
    DSA->ogluTextureBuffer(textureID, GL_TEXTURE_BUFFER, TexFormatUtil::GetInnerFormat(iformat), tbo->bufferID);
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
    if (HAS_FIELD(format, TextureInnerFormat::FLAG_COMP) &&
        MATCH_ANY(format & TextureInnerFormat::BITS_MASK, { TextureInnerFormat::BITS_2, TextureInnerFormat::BITS_4, TextureInnerFormat::BITS_5, TextureInnerFormat::BITS_12 }))
        COMMON_THROW(OGLWrongFormatException, u"TexImg does not support some composite texture type", format);
}

void _oglImgBase::bind(const uint16_t pos) const noexcept
{
    CheckCurrent();
    glBindImageTexture(pos, GetTextureID(), 0, IsLayered ? GL_TRUE : GL_FALSE, 0, (GLenum)Usage, TexFormatUtil::GetInnerFormat(InnerTex->GetInnerFormat()));
}

void _oglImgBase::unbind() const noexcept
{
}

_oglImg2D::_oglImg2D(const Wrapper<detail::_oglTexture2D>& tex, const TexImgUsage usage) : _oglImgBase(tex, usage, false) {}

_oglImg3D::_oglImg3D(const Wrapper<detail::_oglTexture3D>& tex, const TexImgUsage usage) : _oglImgBase(tex, usage, true) {}



}


GLenum TexFormatUtil::GetInnerFormat(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::R8:            return GL_R8;
    case TextureInnerFormat::RG8:           return GL_RG8;
    case TextureInnerFormat::RGB8:          return GL_RGB8;
    case TextureInnerFormat::SRGB8:         return GL_SRGB8;
    case TextureInnerFormat::RGBA8:         return GL_RGBA8;
    case TextureInnerFormat::SRGBA8:        return GL_SRGB8_ALPHA8;
    case TextureInnerFormat::R16:           return GL_R16;
    case TextureInnerFormat::RG16:          return GL_RG16;
    case TextureInnerFormat::RGB16:         return GL_RGB16;
    case TextureInnerFormat::RGBA16:        return GL_RGBA16;
    case TextureInnerFormat::R8S:           return GL_R8_SNORM;
    case TextureInnerFormat::RG8S:          return GL_RG8_SNORM;
    case TextureInnerFormat::RGB8S:         return GL_RGB8_SNORM;
    case TextureInnerFormat::RGBA8S:        return GL_RGBA8_SNORM;
    case TextureInnerFormat::R16S:          return GL_R16_SNORM;
    case TextureInnerFormat::RG16S:         return GL_RG16_SNORM;
    case TextureInnerFormat::RGB16S:        return GL_RGB16_SNORM;
    case TextureInnerFormat::RGBA16S:       return GL_RGBA16_SNORM;
    case TextureInnerFormat::R8U:           return GL_R8UI;
    case TextureInnerFormat::RG8U:          return GL_RG8UI;
    case TextureInnerFormat::RGB8U:         return GL_RGB8UI;
    case TextureInnerFormat::RGBA8U:        return GL_RGBA8UI;
    case TextureInnerFormat::R16U:          return GL_R16UI;
    case TextureInnerFormat::RG16U:         return GL_RG16UI;
    case TextureInnerFormat::RGB16U:        return GL_RGB16UI;
    case TextureInnerFormat::RGBA16U:       return GL_RGBA16UI;
    case TextureInnerFormat::R32U:          return GL_R32UI;
    case TextureInnerFormat::RG32U:         return GL_RG32UI;
    case TextureInnerFormat::RGB32U:        return GL_RGB32UI;
    case TextureInnerFormat::RGBA32U:       return GL_RGBA32UI;
    case TextureInnerFormat::R8I:           return GL_R8I;
    case TextureInnerFormat::RG8I:          return GL_RG8I;
    case TextureInnerFormat::RGB8I:         return GL_RGB8I;
    case TextureInnerFormat::RGBA8I:        return GL_RGBA8I;
    case TextureInnerFormat::R16I:          return GL_R16I;
    case TextureInnerFormat::RG16I:         return GL_RG16I;
    case TextureInnerFormat::RGB16I:        return GL_RGB16I;
    case TextureInnerFormat::RGBA16I:       return GL_RGBA16I;
    case TextureInnerFormat::R32I:          return GL_R32I;
    case TextureInnerFormat::RG32I:         return GL_RG32I;
    case TextureInnerFormat::RGB32I:        return GL_RGB32I;
    case TextureInnerFormat::RGBA32I:       return GL_RGBA32I;
    case TextureInnerFormat::Rh:            return GL_R16F;
    case TextureInnerFormat::RGh:           return GL_RG16F;
    case TextureInnerFormat::RGBh:          return GL_RGB16F;
    case TextureInnerFormat::RGBAh:         return GL_RGBA16F;
    case TextureInnerFormat::Rf:            return GL_R32F;
    case TextureInnerFormat::RGf:           return GL_RG32F;
    case TextureInnerFormat::RGBf:          return GL_RGB32F;
    case TextureInnerFormat::RGBAf:         return GL_RGBA32F;
    case TextureInnerFormat::RG11B10:       return GL_R11F_G11F_B10F;
    case TextureInnerFormat::RGB332:        return GL_R3_G3_B2;
    case TextureInnerFormat::RGBA4444:      return GL_RGBA4;
    case TextureInnerFormat::RGB5A1:        return GL_RGB5_A1;
    case TextureInnerFormat::RGB565:        return GL_RGB565;
    case TextureInnerFormat::RGB10A2:       return GL_RGB10_A2;
    case TextureInnerFormat::RGB10A2U:      return GL_RGB10_A2UI;
    case TextureInnerFormat::RGBA12:        return GL_RGB12;
    case TextureInnerFormat::BC1:           return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case TextureInnerFormat::BC1A:          return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case TextureInnerFormat::BC2:           return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case TextureInnerFormat::BC3:           return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case TextureInnerFormat::BC4:           return GL_COMPRESSED_RED_RGTC1;
    case TextureInnerFormat::BC5:           return GL_COMPRESSED_RG_RGTC2;
    case TextureInnerFormat::BC6H:          return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    case TextureInnerFormat::BC7:           return GL_COMPRESSED_RGBA_BPTC_UNORM;
    case TextureInnerFormat::BC1SRGB:       return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
    case TextureInnerFormat::BC1ASRGB:      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case TextureInnerFormat::BC2SRGB:       return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case TextureInnerFormat::BC3SRGB:       return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case TextureInnerFormat::BC7SRGB:       return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    default:                                return GL_INVALID_ENUM;
    }
}
TextureInnerFormat TexFormatUtil::ConvertFrom(const ImageDataType type, const bool normalized) noexcept
{
    TextureInnerFormat baseFormat = HAS_FIELD(type, ImageDataType::FLOAT_MASK) ? TextureInnerFormat::CAT_FLOAT :
        (normalized ? TextureInnerFormat::CAT_UNORM8 : TextureInnerFormat::CAT_U8) | TextureInnerFormat::FLAG_SRGB;
    switch (REMOVE_MASK(type, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::RGB:
    case ImageDataType::BGR:    return TextureInnerFormat::CHANNEL_RGB  | baseFormat;
    case ImageDataType::RGBA:
    case ImageDataType::BGRA:   return TextureInnerFormat::CHANNEL_RGBA | baseFormat;
    case ImageDataType::GRAY:   return TextureInnerFormat::CHANNEL_R    | baseFormat;
    case ImageDataType::GA:     return TextureInnerFormat::CHANNEL_RG   | baseFormat;
    default:                    return TextureInnerFormat::ERROR;
    }
}
TextureInnerFormat TexFormatUtil::ConvertFrom(const TextureDataFormat dformat) noexcept
{
    const bool isInteger = HAS_FIELD(dformat, TextureDataFormat::INTEGER_MASK);
    if (HAS_FIELD(dformat, TextureDataFormat::TYPE_COMP))
    {
        switch (REMOVE_MASK(dformat, TextureDataFormat::REVERSE_MASK))
        {
        case TextureDataFormat::TYPE_10_2:      return isInteger ? TextureInnerFormat::RGB10A2U : TextureInnerFormat::RGB10A2;
        case TextureDataFormat::TYPE_5551:      return TextureInnerFormat::RGB5A1;
        case TextureDataFormat::TYPE_565:       return TextureInnerFormat::RGB565;
        case TextureDataFormat::TYPE_4444:      return TextureInnerFormat::RGBA4444;
        case TextureDataFormat::TYPE_332:       return TextureInnerFormat::RGB332;
        default:                                return TextureInnerFormat::ERROR;
        }
    }
    TextureInnerFormat format = TextureInnerFormat::EMPTY_MASK;
    switch (dformat & TextureDataFormat::TYPE_RAW_MASK)
    {
    case TextureDataFormat::TYPE_U8:        format |= isInteger ? TextureInnerFormat::CAT_U8  : TextureInnerFormat::CAT_UNORM8; break;
    case TextureDataFormat::TYPE_I8:        format |= isInteger ? TextureInnerFormat::CAT_S8  : TextureInnerFormat::CAT_SNORM8; break;
    case TextureDataFormat::TYPE_U16:       format |= isInteger ? TextureInnerFormat::CAT_U16 : TextureInnerFormat::CAT_UNORM16; break;
    case TextureDataFormat::TYPE_I16:       format |= isInteger ? TextureInnerFormat::CAT_S16 : TextureInnerFormat::CAT_SNORM16; break;
    case TextureDataFormat::TYPE_U32:       format |= isInteger ? TextureInnerFormat::CAT_U32 : TextureInnerFormat::CAT_UNORM32; break;
    case TextureDataFormat::TYPE_I32:       format |= isInteger ? TextureInnerFormat::CAT_S32 : TextureInnerFormat::CAT_SNORM32; break;
    case TextureDataFormat::TYPE_HALF:      format |= TextureInnerFormat::CAT_HALF; break;
    case TextureDataFormat::TYPE_FLOAT:     format |= TextureInnerFormat::CAT_FLOAT; break;
    default:                                break;
    }
    switch (dformat & TextureDataFormat::FORMAT_MASK)
    {
    case TextureDataFormat::FORMAT_R:       format |= TextureInnerFormat::CHANNEL_R; break;
    case TextureDataFormat::FORMAT_G:       format |= TextureInnerFormat::CHANNEL_R; break;
    case TextureDataFormat::FORMAT_B:       format |= TextureInnerFormat::CHANNEL_R; break;
    case TextureDataFormat::FORMAT_A:       format |= TextureInnerFormat::CHANNEL_R; break;
    case TextureDataFormat::FORMAT_RG:      format |= TextureInnerFormat::CHANNEL_RA; break;
    case TextureDataFormat::FORMAT_RGB:     format |= TextureInnerFormat::CHANNEL_RGB; break;
    case TextureDataFormat::FORMAT_BGR:     format |= TextureInnerFormat::CHANNEL_RGB; break;
    case TextureDataFormat::FORMAT_RGBA:    format |= TextureInnerFormat::CHANNEL_RGBA; break;
    case TextureDataFormat::FORMAT_BGRA:    format |= TextureInnerFormat::CHANNEL_RGBA; break;
    default:                                break;
    }
    return format;
}
ImageDataType TexFormatUtil::ConvertToImgType(const TextureInnerFormat format, const bool relaxConvert) noexcept
{
    if (!HAS_FIELD(format, TextureInnerFormat::FLAG_COMP))
    {
        ImageDataType dtype;
        switch (format & TextureInnerFormat::CHANNEL_MASK)
        {
        case TextureInnerFormat::CHANNEL_R:     dtype = ImageDataType::RED; break;
        case TextureInnerFormat::CHANNEL_RG:    dtype = ImageDataType::RA; break;
        case TextureInnerFormat::CHANNEL_RGB:   dtype = ImageDataType::RGB; break;
        case TextureInnerFormat::CHANNEL_RGBA:  dtype = ImageDataType::RGBA; break;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
        switch (format & TextureInnerFormat::CAT_MASK)
        {
        case TextureInnerFormat::CAT_SNORM8:
        case TextureInnerFormat::CAT_U8:
        case TextureInnerFormat::CAT_S8:        if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE; //only pass through when relaxConvert
        case TextureInnerFormat::CAT_UNORM8:    return dtype;
        case TextureInnerFormat::CAT_FLOAT:     return dtype | ImageDataType::FLOAT_MASK;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
    }
    return ImageDataType::UNKNOWN_RESERVE;
}

void TexFormatUtil::ParseFormat(const TextureDataFormat dformat, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept
{
    switch (dformat & TextureDataFormat::TYPE_RAW_MASK)
    {
    case TextureDataFormat::TYPE_U8:        datatype = GL_UNSIGNED_BYTE; break;
    case TextureDataFormat::TYPE_I8:        datatype = GL_BYTE; break;
    case TextureDataFormat::TYPE_U16:       datatype = GL_UNSIGNED_SHORT; break;
    case TextureDataFormat::TYPE_I16:       datatype = GL_SHORT; break;
    case TextureDataFormat::TYPE_U32:       datatype = GL_UNSIGNED_INT; break;
    case TextureDataFormat::TYPE_I32:       datatype = GL_INT; break;
    case TextureDataFormat::TYPE_HALF:      datatype = isUpload ? GL_FLOAT : GL_HALF_FLOAT; break;
    case TextureDataFormat::TYPE_FLOAT:     datatype = GL_FLOAT; break;
    case TextureDataFormat::TYPE_332:       datatype = GL_UNSIGNED_BYTE_3_3_2; break;
    case TextureDataFormat::TYPE_233R:      datatype = GL_UNSIGNED_BYTE_2_3_3_REV; break;
    case TextureDataFormat::TYPE_565:       datatype = GL_UNSIGNED_SHORT_5_6_5; break;
    case TextureDataFormat::TYPE_565R:      datatype = GL_UNSIGNED_SHORT_5_6_5_REV; break;
    case TextureDataFormat::TYPE_4444:      datatype = GL_UNSIGNED_SHORT_4_4_4_4; break;
    case TextureDataFormat::TYPE_4444R:     datatype = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
    case TextureDataFormat::TYPE_5551:      datatype = GL_UNSIGNED_SHORT_5_5_5_1; break;
    case TextureDataFormat::TYPE_1555R:     datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
    case TextureDataFormat::TYPE_8888:      datatype = GL_UNSIGNED_INT_8_8_8_8; break;
    case TextureDataFormat::TYPE_8888R:     datatype = GL_UNSIGNED_INT_8_8_8_8_REV; break;
    case TextureDataFormat::TYPE_10_2:      datatype = GL_UNSIGNED_INT_10_10_10_2; break;
    case TextureDataFormat::TYPE_10_2R:     datatype = GL_UNSIGNED_INT_2_10_10_10_REV; break;
    default:                                break;
    }
    const bool normalized = !HAS_FIELD(dformat, TextureDataFormat::INTEGER_MASK);
    switch (dformat & TextureDataFormat::FORMAT_MASK)
    {
    case TextureDataFormat::FORMAT_R:       comptype = normalized ? GL_RED   : GL_RED_INTEGER; break;
    case TextureDataFormat::FORMAT_G:       comptype = normalized ? GL_GREEN : GL_GREEN_INTEGER; break;
    case TextureDataFormat::FORMAT_B:       comptype = normalized ? GL_BLUE  : GL_BLUE_INTEGER; break;
    case TextureDataFormat::FORMAT_A:       comptype = normalized ? GL_ALPHA : GL_ALPHA_INTEGER; break;
    case TextureDataFormat::FORMAT_RG:      comptype = normalized ? GL_RG    : GL_RG_INTEGER; break;
    case TextureDataFormat::FORMAT_RGB:     comptype = normalized ? GL_RGB   : GL_RGB_INTEGER; break;
    case TextureDataFormat::FORMAT_BGR:     comptype = normalized ? GL_BGR   : GL_BGR_INTEGER; break;
    case TextureDataFormat::FORMAT_RGBA:    comptype = normalized ? GL_RGBA  : GL_RGBA_INTEGER; break;
    case TextureDataFormat::FORMAT_BGRA:    comptype = normalized ? GL_BGRA  : GL_BGRA_INTEGER; break;
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
    case ImageDataType::GRAY:   comptype = normalized ? GL_RED  : GL_RED_INTEGER; break;
    case ImageDataType::RA:     comptype = normalized ? GL_RG   : GL_RG_INTEGER; break;
    case ImageDataType::RGB:    comptype = normalized ? GL_RGB  : GL_RGB_INTEGER; break;
    case ImageDataType::BGR:    comptype = normalized ? GL_BGR  : GL_BGR_INTEGER; break;
    case ImageDataType::RGBA:   comptype = normalized ? GL_RGBA : GL_RGBA_INTEGER; break;
    case ImageDataType::BGRA:   comptype = normalized ? GL_BGRA : GL_BGRA_INTEGER; break;
    default:                    break;
    }
}

oglu::TextureDataFormat TexFormatUtil::ConvertDtypeFrom(const oglu::TextureInnerFormat format)
{
    using oglu::TextureInnerFormat;
    using oglu::TextureDataFormat;
    if (oglu::TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLWrongFormatException, u"compressed texture format cannot be convert to common data format", format);
    if (HAS_FIELD(format, TextureInnerFormat::FLAG_COMP))
    {
        switch (format)
        {
        case TextureInnerFormat::RGB565:    return TextureDataFormat::TYPE_565    | TextureDataFormat::FORMAT_RGB;
        case TextureInnerFormat::RGB5A1:    return TextureDataFormat::TYPE_5551   | TextureDataFormat::FORMAT_RGBA;
        case TextureInnerFormat::RGB10A2:   return TextureDataFormat::TYPE_10_2   | TextureDataFormat::FORMAT_RGBA;
        case TextureInnerFormat::RGB10A2U:  return TextureDataFormat::TYPE_10_2   | TextureDataFormat::FORMAT_RGBA | TextureDataFormat::INTEGER_MASK;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported composite glTex format", format);
        }
    }
    TextureDataFormat dformat = TextureDataFormat::EMPTY_MASK;
    switch (format & TextureInnerFormat::FORMAT_MASK)
    {
    case TextureInnerFormat::FORMAT_UINT:
        dformat |= TextureDataFormat::INTEGER_MASK;
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_8:    dformat = TextureDataFormat::TYPE_U8; break;
        case TextureInnerFormat::BITS_16:   dformat = TextureDataFormat::TYPE_U16; break;
        case TextureInnerFormat::BITS_32:   dformat = TextureDataFormat::TYPE_U32; break;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported UINT glTex format", format);
        }
        break;
    case TextureInnerFormat::FORMAT_SINT:
        dformat |= TextureDataFormat::INTEGER_MASK;
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_8:    dformat = TextureDataFormat::TYPE_I8; break;
        case TextureInnerFormat::BITS_16:   dformat = TextureDataFormat::TYPE_I16; break;
        case TextureInnerFormat::BITS_32:   dformat = TextureDataFormat::TYPE_I32; break;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported INT glTex format", format);
        }
        break;
    case TextureInnerFormat::FORMAT_UNORM:
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_8:    dformat = TextureDataFormat::TYPE_U8; break;
        case TextureInnerFormat::BITS_16:   dformat = TextureDataFormat::TYPE_U16; break;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported UNORM glTex format", format);
        }
        break;
    case TextureInnerFormat::FORMAT_SNORM:
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_8:    dformat = TextureDataFormat::TYPE_I8; break;
        case TextureInnerFormat::BITS_16:   dformat = TextureDataFormat::TYPE_I16; break;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported SNORM glTex format", format);
        }
        break;
    case TextureInnerFormat::FORMAT_FLOAT:
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_16:   dformat = TextureDataFormat::TYPE_HALF; break;
        case TextureInnerFormat::BITS_32:   dformat = TextureDataFormat::TYPE_FLOAT; break;
        default:                            COMMON_THROW(OGLWrongFormatException, u"unsupported FLOAT glTex format", format);
        }
        break;
    default:
        COMMON_THROW(OGLWrongFormatException, u"unsupported glTex datatype format", format);
    }
    switch (format & TextureInnerFormat::CHANNEL_MASK)
    {
    case TextureInnerFormat::CHANNEL_R:     return dformat | TextureDataFormat::FORMAT_R;
    case TextureInnerFormat::CHANNEL_RG:    return dformat | TextureDataFormat::FORMAT_RG;
    case TextureInnerFormat::CHANNEL_RGB:   return dformat | TextureDataFormat::FORMAT_RGB;
    case TextureInnerFormat::CHANNEL_RGBA:  return dformat | TextureDataFormat::FORMAT_RGBA;
    default:                                COMMON_THROW(OGLWrongFormatException, u"unsupported glTex channel", format);
    }
}

TextureDataFormat TexFormatUtil::ConvertDtypeFrom(const ImageDataType dtype, const bool normalized) noexcept
{
    TextureDataFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ? TextureDataFormat::TYPE_FLOAT : TextureDataFormat::TYPE_U8;
    if (!normalized)
        baseFormat |= TextureDataFormat::INTEGER_MASK;
    switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::RGB:    return TextureDataFormat::FORMAT_RGB  | baseFormat;
    case ImageDataType::BGR:    return TextureDataFormat::FORMAT_BGR  | baseFormat;
    case ImageDataType::RGBA:   return TextureDataFormat::FORMAT_RGBA | baseFormat;
    case ImageDataType::BGRA:   return TextureDataFormat::FORMAT_BGRA | baseFormat;
    case ImageDataType::GRAY:   return TextureDataFormat::FORMAT_R    | baseFormat;
    case ImageDataType::GA:     return TextureDataFormat::FORMAT_RG   | baseFormat;
    default:                    return baseFormat;
    }
}
ImageDataType TexFormatUtil::ConvertToImgType(const TextureDataFormat format, const bool relaxConvert) noexcept
{
    ImageDataType isFloat;
    switch (format & TextureDataFormat::TYPE_MASK)
    {
    case TextureDataFormat::TYPE_I8 | TextureDataFormat::INTEGER_MASK:
    case TextureDataFormat::TYPE_U8 | TextureDataFormat::INTEGER_MASK:
    case TextureDataFormat::TYPE_I8:        if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE; //only pass through when relaxConvert
    case TextureDataFormat::TYPE_U8:        isFloat = ImageDataType::EMPTY_MASK; break;
    case TextureDataFormat::TYPE_FLOAT:     isFloat = ImageDataType::FLOAT_MASK; break;
    default:                                return ImageDataType::UNKNOWN_RESERVE;
    }
    switch (format & TextureDataFormat::FORMAT_MASK)
    {
    case TextureDataFormat::FORMAT_R:
    case TextureDataFormat::FORMAT_G:
    case TextureDataFormat::FORMAT_B:       return ImageDataType::RED  | isFloat;
    case TextureDataFormat::FORMAT_RG:      return ImageDataType::RA   | isFloat;
    case TextureDataFormat::FORMAT_RGB:     return ImageDataType::RGB  | isFloat;
    case TextureDataFormat::FORMAT_RGBA:    return ImageDataType::RGBA | isFloat;
    case TextureDataFormat::FORMAT_BGR:     return ImageDataType::BGR  | isFloat;
    case TextureDataFormat::FORMAT_BGRA:    return ImageDataType::BGRA | isFloat;
    default:                                return ImageDataType::UNKNOWN_RESERVE;
    }
}

size_t TexFormatUtil::ParseFormatSize(const TextureInnerFormat format) noexcept
{
    if (HAS_FIELD(format, TextureInnerFormat::FLAG_COMP))
    {
        switch (format)
        {
        case TextureInnerFormat::RG11B10:       return 4;
        case TextureInnerFormat::RGB332:        return 1;
        case TextureInnerFormat::RGB5A1:        return 2;
        case TextureInnerFormat::RGB565:        return 2;
        case TextureInnerFormat::RGB10A2:       return 4;
        case TextureInnerFormat::RGB10A2U:      return 4;
        case TextureInnerFormat::RGBA12:        return 6;
        default:                                return 0;
        }
    }
    if (IsCompressType(format))
        return 0; // should not use this
    size_t size = 0;
    switch (format & TextureInnerFormat::BITS_MASK)
    {
    case TextureInnerFormat::BITS_8:        size = 8; break;
    case TextureInnerFormat::BITS_16:       size = 8; break;
    case TextureInnerFormat::BITS_32:       size = 8; break;
    default:                                return 0;
    }
    switch (format & TextureInnerFormat::CHANNEL_MASK)
    {
    case TextureInnerFormat::CHANNEL_R:     size *= 1; break;
    case TextureInnerFormat::CHANNEL_RG:    size *= 2; break;
    case TextureInnerFormat::CHANNEL_RGB:   size *= 3; break;
    case TextureInnerFormat::CHANNEL_RGBA:  size *= 4; break;
    default:                                return 0;
    }
    return size / 8;
}
size_t TexFormatUtil::ParseFormatSize(const TextureDataFormat dformat) noexcept
{
    size_t size = 0;
    switch (dformat & TextureDataFormat::TYPE_RAW_MASK)
    {
    case TextureDataFormat::TYPE_U8:
    case TextureDataFormat::TYPE_I8:
    case TextureDataFormat::TYPE_332:
    case TextureDataFormat::TYPE_233R:
        size = 8; break;
    case TextureDataFormat::TYPE_U16:
    case TextureDataFormat::TYPE_I16:
    case TextureDataFormat::TYPE_HALF:
    case TextureDataFormat::TYPE_4444:
    case TextureDataFormat::TYPE_4444R:
    case TextureDataFormat::TYPE_565:
    case TextureDataFormat::TYPE_565R:
    case TextureDataFormat::TYPE_5551:
    case TextureDataFormat::TYPE_1555R:
        size = 16; break;
    case TextureDataFormat::TYPE_U32:
    case TextureDataFormat::TYPE_I32:
    case TextureDataFormat::TYPE_FLOAT:
    case TextureDataFormat::TYPE_8888:
    case TextureDataFormat::TYPE_8888R:
    case TextureDataFormat::TYPE_10_2:
    case TextureDataFormat::TYPE_10_2R:
        size = 32; break;
    default:
        return 0;
    }
    if (!HAS_FIELD(dformat, TextureDataFormat::COMP_MASK))
    {
        switch (dformat & TextureDataFormat::FORMAT_MASK)
        {
        case TextureDataFormat::FORMAT_R:
        case TextureDataFormat::FORMAT_G:
        case TextureDataFormat::FORMAT_B:
        case TextureDataFormat::FORMAT_A:
            size *= 1; break;
        case TextureDataFormat::FORMAT_RG:
            size *= 2; break;
        case TextureDataFormat::FORMAT_RGB:
        case TextureDataFormat::FORMAT_BGR:
            size *= 3; break;
        case TextureDataFormat::FORMAT_RGBA:
        case TextureDataFormat::FORMAT_BGRA:
            size *= 4; break;
        default:
            return 0;
        }
    }
    return size / 8;
}

using namespace std::literals;

u16string_view TexFormatUtil::GetTypeName(const TextureType type) noexcept
{
    switch (type)
    {
    case TextureType::Tex2D:             return u"Tex2D"sv;
    case TextureType::Tex2DArray:        return u"Tex2DArray"sv;
    case TextureType::TexBuf:            return u"TexBuffer"sv;
    default:                             return u"Wrong"sv;
    }
}

u16string_view TexFormatUtil::GetFormatName(const TextureInnerFormat format) noexcept
{
    switch (format)
    {
    case TextureInnerFormat::R8:            return u"R8"sv;
    case TextureInnerFormat::RG8:           return u"RG8"sv;
    case TextureInnerFormat::RGB8:          return u"RGB8"sv;
    case TextureInnerFormat::SRGB8:         return u"SRGB8"sv;
    case TextureInnerFormat::RGBA8:         return u"RGBA8"sv;
    case TextureInnerFormat::SRGBA8:        return u"SRGBA8"sv;
    case TextureInnerFormat::R16:           return u"R16"sv;
    case TextureInnerFormat::RG16:          return u"RG16"sv;
    case TextureInnerFormat::RGB16:         return u"RGB16"sv;
    case TextureInnerFormat::RGBA16:        return u"RGBA16"sv;
    case TextureInnerFormat::R8S:           return u"R8S"sv;
    case TextureInnerFormat::RG8S:          return u"RG8S"sv;
    case TextureInnerFormat::RGB8S:         return u"RGB8S"sv;
    case TextureInnerFormat::RGBA8S:        return u"RGBA8S"sv;
    case TextureInnerFormat::R16S:          return u"R16S"sv;
    case TextureInnerFormat::RG16S:         return u"RG16S"sv;
    case TextureInnerFormat::RGB16S:        return u"RGB16S"sv;
    case TextureInnerFormat::RGBA16S:       return u"RGBA16S"sv;
    case TextureInnerFormat::R8U:           return u"R8U"sv;
    case TextureInnerFormat::RG8U:          return u"RG8U"sv;
    case TextureInnerFormat::RGB8U:         return u"RGB8U"sv;
    case TextureInnerFormat::RGBA8U:        return u"RGBA8U"sv;
    case TextureInnerFormat::R16U:          return u"R16U"sv;
    case TextureInnerFormat::RG16U:         return u"RG16U"sv;
    case TextureInnerFormat::RGB16U:        return u"RGB16U"sv;
    case TextureInnerFormat::RGBA16U:       return u"RGBA16U"sv;
    case TextureInnerFormat::R32U:          return u"R32U"sv;
    case TextureInnerFormat::RG32U:         return u"RG32U"sv;
    case TextureInnerFormat::RGB32U:        return u"RGB32U"sv;
    case TextureInnerFormat::RGBA32U:       return u"RGBA32U"sv;
    case TextureInnerFormat::R8I:           return u"R8I"sv;
    case TextureInnerFormat::RG8I:          return u"RG8I"sv;
    case TextureInnerFormat::RGB8I:         return u"RGB8I"sv;
    case TextureInnerFormat::RGBA8I:        return u"RGBA8I"sv;
    case TextureInnerFormat::R16I:          return u"R16I"sv;
    case TextureInnerFormat::RG16I:         return u"RG16I"sv;
    case TextureInnerFormat::RGB16I:        return u"RGB16I"sv;
    case TextureInnerFormat::RGBA16I:       return u"RGBA16I"sv;
    case TextureInnerFormat::R32I:          return u"R32I"sv;
    case TextureInnerFormat::RG32I:         return u"RG32I"sv;
    case TextureInnerFormat::RGB32I:        return u"RGB32I"sv;
    case TextureInnerFormat::RGBA32I:       return u"RGBA32I"sv;
    case TextureInnerFormat::Rh:            return u"Rh"sv;
    case TextureInnerFormat::RGh:           return u"RGh"sv;
    case TextureInnerFormat::RGBh:          return u"RGBh"sv;
    case TextureInnerFormat::RGBAh:         return u"RGBAh"sv;
    case TextureInnerFormat::Rf:            return u"Rf"sv;
    case TextureInnerFormat::RGf:           return u"RGf"sv;
    case TextureInnerFormat::RGBf:          return u"RGBf"sv;
    case TextureInnerFormat::RGBAf:         return u"RGBAf"sv;
    case TextureInnerFormat::RG11B10:       return u"RG11B10"sv;
    case TextureInnerFormat::RGB332:        return u"RGB332"sv;
    case TextureInnerFormat::RGBA4444:      return u"RGBA4444"sv;
    case TextureInnerFormat::RGB5A1:        return u"RGB5A1"sv;
    case TextureInnerFormat::RGB565:        return u"RGB565"sv;
    case TextureInnerFormat::RGB10A2:       return u"RGB10A2"sv;
    case TextureInnerFormat::RGB10A2U:      return u"RGB10A2U"sv;
    case TextureInnerFormat::RGBA12:        return u"RGBA12"sv;
    case TextureInnerFormat::BC1:           return u"BC1"sv;
    case TextureInnerFormat::BC1A:          return u"BC1A"sv;
    case TextureInnerFormat::BC2:           return u"BC2"sv;
    case TextureInnerFormat::BC3:           return u"BC3"sv;
    case TextureInnerFormat::BC4:           return u"BC4"sv;
    case TextureInnerFormat::BC5:           return u"BC5"sv;
    case TextureInnerFormat::BC6H:          return u"BC6H"sv;
    case TextureInnerFormat::BC7:           return u"BC7"sv;
    case TextureInnerFormat::BC1SRGB:       return u"BC1SRGB"sv;
    case TextureInnerFormat::BC1ASRGB:      return u"BC1ASRGB"sv;
    case TextureInnerFormat::BC2SRGB:       return u"BC2SRGB"sv;
    case TextureInnerFormat::BC3SRGB:       return u"BC3SRGB"sv;
    case TextureInnerFormat::BC7SRGB:       return u"BC7SRGB"sv;
    default:                                return u"Other"sv;
    }
}

string TexFormatUtil::GetFormatDetail(const TextureInnerFormat format) noexcept
{
    //[  flags|formats|channel|   bits]
    //[15...13|12....8|7.....5|4.....0]
    uint8_t bits = 0;
    switch (format & TextureInnerFormat::BITS_MASK)
    {
    case TextureInnerFormat::BITS_2:    bits = 2; break;
    case TextureInnerFormat::BITS_4:    bits = 4; break;
    case TextureInnerFormat::BITS_5:    bits = 5; break;
    case TextureInnerFormat::BITS_8:    bits = 8; break;
    case TextureInnerFormat::BITS_10:   bits = 10; break;
    case TextureInnerFormat::BITS_11:   bits = 11; break;
    case TextureInnerFormat::BITS_12:   bits = 12; break;
    case TextureInnerFormat::BITS_16:   bits = 16; break;
    case TextureInnerFormat::BITS_32:   bits = 32; break;
    default:                            break;
    }
    string_view channel;
    switch (format & TextureInnerFormat::CHANNEL_MASK)
    {
    case TextureInnerFormat::CHANNEL_R:     channel = "Red"sv; break;
    case TextureInnerFormat::CHANNEL_RG:    channel = "RG"sv; break;
    case TextureInnerFormat::CHANNEL_RGB:   channel = "RGB"sv; break;
    case TextureInnerFormat::CHANNEL_RGBA:  channel = "RGBA"sv; break;
    default:                                channel = "UNKNOWN"sv; break;
    }
    string_view dtype;
    switch (format & TextureInnerFormat::FORMAT_MASK)
    {
    case TextureInnerFormat::FORMAT_UNORM:      dtype = "UNORM"sv; break;
    case TextureInnerFormat::FORMAT_SNORM:      dtype = "SNORM"sv; break;
    case TextureInnerFormat::FORMAT_UINT:       dtype = "UINT"sv; break;
    case TextureInnerFormat::FORMAT_SINT:       dtype = "INT"sv; break;
    case TextureInnerFormat::FORMAT_FLOAT:      dtype = "FLOAT"sv; break;
    case TextureInnerFormat::FORMAT_DXT1:       dtype = "DXT1"sv; break;
    case TextureInnerFormat::FORMAT_DXT3:       dtype = "DXT3"sv; break;
    case TextureInnerFormat::FORMAT_DXT5:       dtype = "DXT5"sv; break;
    case TextureInnerFormat::FORMAT_RGTC:       dtype = "RGTC"sv; break;
    case TextureInnerFormat::FORMAT_BPTC:       dtype = "BPTC"sv; break;
    case TextureInnerFormat::FORMAT_ETC:        dtype = "ETC"sv; break;
    case TextureInnerFormat::FORMAT_ETC2:       dtype = "ETC2"sv; break;
    case TextureInnerFormat::FORMAT_PVRTC:      dtype = "PVRTC"sv; break;
    case TextureInnerFormat::FORMAT_PVRTC2:     dtype = "PVRTC2"sv; break;
    case TextureInnerFormat::FORMAT_ASTC:       dtype = "ASTC"sv; break;
    default:                                    dtype = "UNKNOWN"sv; break;
    }
    string flag = "|";
    if (HAS_FIELD(format, TextureInnerFormat::FLAG_SRGB))
        flag += "sRGB|";
    if (HAS_FIELD(format, TextureInnerFormat::FLAG_COMP))
        flag += "Comp|";
    flag.pop_back();
    return fmt::format("dtype[{:^7}]channel[{:^7}]bits[{:2}]flags[{}]", dtype, channel, bits, flag);
}
string TexFormatUtil::GetFormatDetail(const TextureDataFormat format) noexcept
{
    string_view dtype, dflag;
    const auto rawType = format & TextureDataFormat::TYPE_RAW_MASK;
    if (static_cast<uint16_t>(rawType) >= static_cast<uint16_t>(TextureDataFormat::TYPE_COMP))
    {
        dflag = HAS_FIELD(rawType, TextureDataFormat::TYPE_REV_MASK) ? "REV"sv : ""sv;
        switch (REMOVE_MASK(rawType, TextureDataFormat::TYPE_REV_MASK))
        {
        case TextureDataFormat::TYPE_332:       dtype = "UNORM332"sv; break;
        case TextureDataFormat::TYPE_565:       dtype = "UNORM565"sv; break;
        case TextureDataFormat::TYPE_5551:      dtype = "UNORM5551"sv; break;
        case TextureDataFormat::TYPE_4444:      dtype = "UNORM4444"sv; break;
        case TextureDataFormat::TYPE_10_2:      dtype = "UNORM10_2"sv; break;
        case TextureDataFormat::TYPE_8888:      dtype = "UNORM8888"sv; break;
        default:                                dtype = "UNKNOWN"sv; break;
        }
    }
    else
    {
        dflag = HAS_FIELD(format, TextureDataFormat::INTEGER_MASK) ? "INTEGER"sv : "NORMAL"sv;
        switch (rawType)
        {
        case TextureDataFormat::TYPE_U8:    dtype = "U8"sv; break;
        case TextureDataFormat::TYPE_I8:    dtype = "I8"sv; break;
        case TextureDataFormat::TYPE_U16:   dtype = "U16"sv; break;
        case TextureDataFormat::TYPE_I16:   dtype = "I16"sv; break;
        case TextureDataFormat::TYPE_U32:   dtype = "U32"sv; break;
        case TextureDataFormat::TYPE_I32:   dtype = "I32"sv; break;
        case TextureDataFormat::TYPE_HALF:  dtype = "Half"sv; break;
        case TextureDataFormat::TYPE_FLOAT: dtype = "Float"sv; break;
        default:                            dtype = "UNKNOWN"sv; break;
        }
    }
    string_view channel;
    switch (format & TextureDataFormat::FORMAT_MASK)
    {
    case TextureDataFormat::FORMAT_R:       channel = "R"sv; break;
    case TextureDataFormat::FORMAT_G:       channel = "G"sv; break;
    case TextureDataFormat::FORMAT_B:       channel = "B"sv; break;
    case TextureDataFormat::FORMAT_A:       channel = "A"sv; break;
    case TextureDataFormat::FORMAT_RG:      channel = "RG"sv; break;
    case TextureDataFormat::FORMAT_RGB:     channel = "RGB"sv; break;
    case TextureDataFormat::FORMAT_BGR:     channel = "BGR"sv; break;
    case TextureDataFormat::FORMAT_RGBA:    channel = "RGBA"sv; break;
    case TextureDataFormat::FORMAT_BGRA:    channel = "BGRA"sv; break;
    default:                                channel = "UNKNOWN"sv; break;
    }
    return fmt::format("dtype[{:^7}]({:7})channel[{:^7}]", dtype, dflag, channel);
}

}