#include "oglPch.h"
#include "oglTexture.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "BindingManager.h"

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

COMMON_EXCEPTION_IMPL(OGLWrongFormatException)

MAKE_ENABLER_IMPL(oglTex2DView_)
MAKE_ENABLER_IMPL(oglTex2DStatic_)
MAKE_ENABLER_IMPL(oglTex2DDynamic_)
MAKE_ENABLER_IMPL(oglTex2DArray_)
MAKE_ENABLER_IMPL(oglTex3DView_)
MAKE_ENABLER_IMPL(oglTex3DStatic_)
MAKE_ENABLER_IMPL(oglBufferTexture_)
MAKE_ENABLER_IMPL(oglImg2D_)
MAKE_ENABLER_IMPL(oglImg3D_)


struct TexLogItem
{
    uint64_t ThreadId;
    GLuint TexId;
    TextureType TexType;
    TextureFormat InnerFormat;
    TexLogItem(const oglTexBase_& tex) : 
        ThreadId(common::ThreadObject::GetCurrentThreadId()), TexId(tex.TextureID), 
        TexType(tex.Type), InnerFormat(tex.InnerFormat) { }
    bool operator<(const TexLogItem& other) const { return TexId < other.TexId; }
};
struct TexLogMap
{
    std::map<GLuint, TexLogItem> TexMap;
    common::SpinLocker MapLock;
    TexLogMap() {}
    TexLogMap(TexLogMap&& other) noexcept
    {
        const auto lock1 = MapLock.LockScope();
        const auto lock2 = other.MapLock.LockScope();
        TexMap.swap(other.TexMap);
    }
    void Insert(TexLogItem&& item) 
    {
        const auto lock = MapLock.LockScope();
        TexMap.insert_or_assign(item.TexId, item);
    }
    void Remove(const GLuint texId) 
    {
        const auto lock = MapLock.LockScope();
        TexMap.erase(texId);
    }
};

struct TLogCtxConfig : public CtxResConfig<false, TexLogMap>
{
    TexLogMap Construct() const { return {}; }
};
static TLogCtxConfig TEXLOG_CTXCFG;

static void RegistTexture(const oglTexBase_& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] create texture [{}], type[{}].\n", item.ThreadId, item.TexId, OGLTexUtil::GetTypeName(item.TexType));
#endif
    oglContext_::CurrentContext()->GetOrCreate<true>(TEXLOG_CTXCFG).Insert(std::move(item));
}
static void UnregistTexture(const oglTexBase_& tex)
{
    TexLogItem item(tex);
#ifdef _DEBUG
    oglLog().verbose(u"here[{}] delete texture [{}][{}], type[{}], detail[{}].\n", item.ThreadId, item.TexId, tex.GetName(),
        OGLTexUtil::GetTypeName(item.TexType), TexFormatUtil::GetFormatName(item.InnerFormat));
#endif
    oglContext_::CurrentContext()->GetOrCreate<true>(TEXLOG_CTXCFG).Remove(item.TexId);
}


struct TManCtxConfig : public CtxResConfig<false, std::unique_ptr<detail::ResourceBinder<oglTexBase_>>>
{
    std::unique_ptr<detail::ResourceBinder<oglTexBase_>> Construct() const
    { 
        if (CtxFunc->SupportBindlessTexture)
            return std::make_unique<detail::BindlessTexManager>();
        else
            return std::make_unique<detail::CachedResManager<oglTexBase_>>(CtxFunc->MaxTextureUnits);
    }
};
static TManCtxConfig TEXMAN_CTXCFG;
detail::ResourceBinder<oglTexBase_>& oglTexBase_::GetTexMan() noexcept
{
    return *oglContext_::CurrentContext()->GetOrCreate<false>(TEXMAN_CTXCFG);
}

oglTexBase_::oglTexBase_(const TextureType type, const bool shouldBindType) noexcept :
    Type(type), TextureID(GL_INVALID_INDEX), InnerFormat(TextureFormat::ERROR), Mipmap(1)
{
    if (shouldBindType)
        CtxFunc->CreateTextures(common::enum_cast(Type), 1, &TextureID);
    else
        CtxFunc->GenTextures(1, &TextureID);
    if (const auto e = oglUtil::GetError(); e.has_value())
        oglLog().warning(u"oglTexBase occurs error due to {}.\n", e.value());
    RegistTexture(*this);
}

oglTexBase_::~oglTexBase_()
{
    if (!EnsureValid()) return;
    UnregistTexture(*this);
    //force unbind texture, since texID may be reused after releasaed
    GetTexMan().ReleaseRes(this);
    CtxFunc->DeleteTextures(1, &TextureID);
}

void oglTexBase_::BindToUnit(const uint16_t pos) const noexcept
{
    CheckCurrent();
    CtxFunc->BindTextureUnit(pos, TextureID, common::enum_cast(Type));
}

void oglTexBase_::PinToHandle() noexcept
{
    CheckCurrent();
    Handle = CtxFunc->GetTextureHandle(TextureID);
    CtxFunc->MakeTextureHandleResident(Handle.value());
}

void oglTexBase_::CheckMipmapLevel(const uint8_t level) const
{
    if (level >= Mipmap)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"set data of a level exceeds texture's mipmap range.");
}

std::pair<uint32_t, uint32_t> oglTexBase_::GetInternalSize2() const
{
    CheckCurrent();
    GLint w = 0, h = 0;
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_WIDTH, &w);
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_HEIGHT, &h);
    return { (uint32_t)w,(uint32_t)h };
}

std::tuple<uint32_t, uint32_t, uint32_t> oglTexBase_::GetInternalSize3() const
{
    CheckCurrent();
    GLint w = 0, h = 0, z = 0;
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_WIDTH, &w);
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_HEIGHT, &h);
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_DEPTH, &z);
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
    COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Unknown texture wrap value.");
}

void oglTexBase_::SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT)
{
    CheckCurrent();
    const auto valS = ParseWrap(wrapS), valT = ParseWrap(wrapT);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_WRAP_S, valS);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_WRAP_T, valT);
}

void oglTexBase_::SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR)
{
    CheckCurrent();
    const auto valS = ParseWrap(wrapS), valT = ParseWrap(wrapT), valR = ParseWrap(wrapR);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_WRAP_S, valS);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_WRAP_T, valT);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_WRAP_R, valR);
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

void oglTexBase_::SetProperty(const TextureFilterVal magFilter, TextureFilterVal minFilter)
{
    CheckCurrent();
    //if (Mipmap <= 1)
    //    minFilter = REMOVE_MASK(minFilter, TextureFilterVal::MIPMAP_MASK);
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_MAG_FILTER, ConvertFilterVal(REMOVE_MASK(magFilter, TextureFilterVal::MIPMAP_MASK)));
    CtxFunc->TextureParameteri(TextureID, common::enum_cast(Type), GL_TEXTURE_MIN_FILTER, ConvertFilterVal(minFilter));
}

void oglu::oglTexBase_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->SetObjectLabel(GL_TEXTURE, TextureID, Name);
}

void oglTexBase_::Clear(const TextureFormat format)
{
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    for (int32_t level = 0; level < Mipmap; ++level)
        CtxFunc->ClearTexImage(TextureID, level, datatype, comptype, nullptr);
}

bool oglTexBase_::IsCompressed() const
{
    CheckCurrent();
    GLint ret = GL_FALSE;
    CtxFunc->GetTextureLevelParameteriv(TextureID, common::enum_cast(Type), 0, GL_TEXTURE_COMPRESSED, &ret);
    return ret != GL_FALSE;
}


oglTex2D_::~oglTex2D_()
{ }

void oglTex2D_::SetProperty(const TextureWrapVal wraptype) 
{ 
    SetWrapProperty(wraptype, wraptype);
}

void oglTex2D_::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        CtxFunc->TextureSubImage2D(TextureID, GL_TEXTURE_2D, level, 0, 0, Width >> level, Height >> level, comptype, datatype, data);
    else
        CtxFunc->TextureImage2D(TextureID, GL_TEXTURE_2D, level, (GLint)OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, 0, comptype, datatype, data);
}

void oglTex2D_::SetCompressedData(const bool isSub, const void * data, const size_t size, const uint8_t level) noexcept
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        CtxFunc->CompressedTextureSubImage2D(TextureID, GL_TEXTURE_2D, level, 0, 0, Width >> level, Height >> level, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        CtxFunc->CompressedTextureImage2D(TextureID, GL_TEXTURE_2D, level, OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> oglTex2D_::GetCompressedData(const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (!IsCompressed())
        return {};
    GLint size = 0;
    CtxFunc->GetTextureLevelParameteriv(TextureID, GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    CtxFunc->GetCompressedTextureImage(TextureID, GL_TEXTURE_2D, level, size, (*output).data());
    return output;
}

vector<uint8_t> oglTex2D_::GetData(const TextureFormat format, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    const auto size = (w * h * xziar::img::TexFormatUtil::BitPerPixel(format) / 8) >> (level * 2);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, false);
    CtxFunc->GetTextureImage(TextureID, GL_TEXTURE_2D, level, comptype, datatype, size, output.data());
    return output;
}

Image oglTex2D_::GetImage(const ImageDataType format, const bool flipY, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h] = GetInternalSize2();
    Image image(format);
    image.SetSize(w >> level, h >> level);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    CtxFunc->GetTextureImage(TextureID, GL_TEXTURE_2D, level, comptype, datatype, image.GetSize(), image.GetRawPtr());
    if (flipY)
        image.FlipVertical();
    return image;
}


oglTex2DView_::oglTex2DView_(const oglTex2DStatic_& tex, const TextureFormat format) : oglTex2D_(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = format;
    Name = tex.Name + u"-View";
    CtxFunc->TextureView(TextureID, GL_TEXTURE_2D, tex.TextureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, 0, 1);
}
oglTex2DView_::oglTex2DView_(const oglTex2DArray_& tex, const TextureFormat format, const uint32_t layer) : oglTex2D_(false)
{
    Width = tex.Width, Height = tex.Height, Mipmap = tex.Mipmap; InnerFormat = format;
    CtxFunc->TextureView(TextureID, GL_TEXTURE_2D, tex.TextureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, layer, 1);
}
static void CheckCompatible(const TextureFormat formatA, const TextureFormat formatB)
{
    if (formatA == formatB) return;
    const bool isCompressA = TexFormatUtil::IsCompressType(formatA), isCompressB = TexFormatUtil::IsCompressType(formatB);
    std::u16string_view errmsg;
    if (isCompressA == isCompressB)
    {
        if (isCompressA)
        {
            if ((formatA & TextureFormat::MASK_COMPRESS_RAW) != (formatB & TextureFormat::MASK_COMPRESS_RAW))
                errmsg = u"texture aliases not compatible between different compressed format";
            else
                return;
        }
        else
        {
            if (TexFormatUtil::BitPerPixel(formatA) != TexFormatUtil::BitPerPixel(formatB))
                errmsg = u"texture aliases not compatible between different sized format";
            else
                return;
        }
    }
    else
        errmsg = u"texture aliases not compatible between compress/non-compress format";
    COMMON_THROWEX(OGLWrongFormatException, errmsg, formatB).Attach("formatA", formatA);
}


oglTex2DStatic_::oglTex2DStatic_(const uint32_t width, const uint32_t height, const TextureFormat format, const uint8_t mipmap) : oglTex2D_(true)
{
    if (width == 0 || height == 0 || mipmap == 0)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2D.");
    if (width % 4)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, InnerFormat = format, Mipmap = mipmap;
    CtxFunc->TextureStorage2D(TextureID, GL_TEXTURE_2D, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), Width, Height);
}
oglTex2DS oglTex2DStatic_::Create(const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const uint8_t mipmap)
{
    return MAKE_ENABLER_SHARED(oglTex2DStatic_, (width, height, format, mipmap));
}

void oglTex2DStatic_::SetData(const TextureFormat format, const common::span<const std::byte> space, const uint8_t level)
{
    oglTex2D_::SetData(true, format, space.data(), level);
}

void oglTex2DStatic_::SetData(const TextureFormat format, const oglPBO& buf, const uint8_t level)
{
    oglTex2D_::SetData(true, format, buf, level);
}

void oglTex2DStatic_::SetData(const Image & img, const bool normalized, const bool flipY, const uint8_t level)
{
    if (img.GetWidth() != (Width >> level) || img.GetHeight() != (Height >> level))
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex2D(S)");
    oglTex2D_::SetData(true, img, normalized, flipY, level);
}

void oglTex2DStatic_::SetCompressedData(const common::span<const std::byte> space, const uint8_t level)
{
    oglTex2D_::SetCompressedData(true, space.data(), space.size(), level);
}

void oglTex2DStatic_::SetCompressedData(const oglPBO & buf, const size_t size, const uint8_t level)
{
    oglTex2D_::SetCompressedData(true, buf, size, level);
}

oglTex2DV oglTex2DStatic_::GetTextureView(const TextureFormat format) const
{
    CheckCurrent();
    CheckCompatible(InnerFormat, format);
    oglTex2DV tex(new oglTex2DView_(*this, InnerFormat));
    tex->Name = Name + u"-View";
    return tex;
}


void oglTex2DDynamic_::CheckAndSetMetadata(const TextureFormat format, const uint32_t w, const uint32_t h)
{
    CheckCurrent();
    if (w % 4)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    InnerFormat = format, Width = w, Height = h;
}
oglTex2DD oglTex2DDynamic_::Create()
{
    return MAKE_ENABLER_SHARED(oglTex2DDynamic_, ());
}

void oglTex2DDynamic_::GenerateMipmap()
{
    CheckCurrent();
    CtxFunc->GenerateTextureMipmap(TextureID, GL_TEXTURE_2D);
}

void oglTex2DDynamic_::SetData(const TextureFormat format, const uint32_t w, const uint32_t h, const common::span<const std::byte> space)
{
    CheckAndSetMetadata(format, w, h);
    oglTex2D_::SetData(false, format, space.data(), 0);
}

void oglTex2DDynamic_::SetData(const TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf)
{
    CheckAndSetMetadata(format, w, h);
    oglTex2D_::SetData(false, format, buf, 0);
}

void oglTex2DDynamic_::SetData(const TextureFormat format, const xziar::img::Image& img, const bool normalized, const bool flipY)
{
    CheckAndSetMetadata(format, img.GetWidth(), img.GetHeight());
    oglTex2D_::SetData(false, img, normalized, flipY, 0);
}

void oglTex2DDynamic_::SetCompressedData(const TextureFormat format, const uint32_t w, const uint32_t h, const common::span<const std::byte> space)
{
    CheckAndSetMetadata(format, w, h);
    oglTex2D_::SetCompressedData(false, space.data(), space.size(), 0);
}

void oglTex2DDynamic_::SetCompressedData(const TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf, const size_t size)
{
    CheckAndSetMetadata(format, w, h);
    oglTex2D_::SetCompressedData(false, buf, size, 0);
}


oglTex2DArray_::~oglTex2DArray_()
{ }

void oglTex2DArray_::SetProperty(const TextureWrapVal wraptype) 
{ 
    SetWrapProperty(wraptype, wraptype);
}

oglTex2DArray_::oglTex2DArray_(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureFormat format, const uint8_t mipmap)
    : oglTexBase_(TextureType::Tex2DArray, true)
{
    if (width == 0 || height == 0 || layers == 0 || mipmap == 0)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex2DArray.");
    if (width % 4)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Layers = layers, InnerFormat = format, Mipmap = mipmap;
    CtxFunc->TextureStorage3D(TextureID, GL_TEXTURE_2D_ARRAY, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), width, height, layers);
}
oglTex2DArray_::oglTex2DArray_(const oglTex2DArray& old, const uint32_t layerAdd) :
    oglTex2DArray_(old->Width, old->Height, old->Layers + layerAdd, old->InnerFormat, old->Mipmap)
{
    SetTextureLayers(0, old, 0, old->Layers);
}
oglTex2DArray oglTex2DArray_::Create(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureFormat format, const uint8_t mipmap)
{
    return MAKE_ENABLER_SHARED(oglTex2DArray_, (width, height, layers, format, mipmap));
}
oglTex2DArray oglTex2DArray_::Create(const oglTex2DArray& old, const uint32_t layerAdd)
{
    return MAKE_ENABLER_SHARED(oglTex2DArray_, (old, layerAdd));
}

void oglTex2DArray_::CheckLayerRange(const uint32_t layer) const
{
    if (layer >= Layers)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
}

void oglTex2DArray_::SetTextureLayer(const uint32_t layer, const oglTex2D& tex)
{
    CheckLayerRange(layer);
    CheckCurrent();
    if (tex->Width != Width || tex->Height != Height)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (tex->Mipmap < Mipmap)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
    if (tex->InnerFormat != InnerFormat)
        oglLog().warning(u"tex[{}][{}] has different innerFormat with texarr[{}][{}].\n", tex->TextureID, (uint16_t)tex->InnerFormat, TextureID, (uint16_t)InnerFormat);
    for (uint8_t i = 0; i < Mipmap; ++i)
    {
        CtxFunc->CopyImageSubData(tex->TextureID, common::enum_cast(tex->Type), i, 0, 0, 0,
            TextureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, layer,
            tex->Width >> i, tex->Height >> i, 1);
    }
}

void oglTex2DArray_::SetTextureLayer(const uint32_t layer, const Image& img, const bool flipY, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    if (img.GetWidth() % 4)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"each line's should be aligned to 4 pixels");
    if (img.GetWidth() != (Width >> level) || img.GetHeight() != (Height >> level))
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(img.GetDataType(), true);
    CtxFunc->TextureSubImage3D(TextureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, img.GetWidth(), img.GetHeight(), 1, comptype, datatype, flipY ? img.FlipToVertical().GetRawPtr() : img.GetRawPtr());
}

void oglTex2DArray_::SetTextureLayer(const uint32_t layer, const TextureFormat format, const void *data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    CheckCurrent();
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
    CtxFunc->TextureSubImage3D(TextureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, Width >> level, Height >> level, 1, comptype, datatype, data);
}

void oglTex2DArray_::SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckLayerRange(layer);
    CheckCurrent();
    CtxFunc->CompressedTextureSubImage3D(TextureID, GL_TEXTURE_2D_ARRAY, level, 0, 0, layer, Width >> level, Height >> level, 1, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
}

void oglTex2DArray_::SetTextureLayers(const uint32_t destLayer, const oglTex2DArray& tex, const uint32_t srcLayer, const uint32_t layerCount)
{
    CheckCurrent();
    if (Width != tex->Width || Height != tex->Height)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture size mismatch");
    if (destLayer + layerCount > Layers || srcLayer + layerCount > tex->Layers)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
    if (tex->Mipmap < Mipmap)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"too few mipmap level");
    for (uint8_t i = 0; i < Mipmap; ++i)
    {
        CtxFunc->CopyImageSubData(tex->TextureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, srcLayer,
            TextureID, GL_TEXTURE_2D_ARRAY, i, 0, 0, destLayer,
            tex->Width, tex->Height, layerCount);
    }
}

oglTex2DV oglTex2DArray_::ViewTextureLayer(const uint32_t layer, const TextureFormat format) const
{
    CheckCurrent();
    if (layer >= Layers)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"layer range outflow");
    CheckCompatible(InnerFormat, format);
    oglTex2DV tex(new oglTex2DView_(*this, InnerFormat, layer));
    const auto layerStr = std::to_string(layer);
    tex->Name = Name + u"-Layer" + u16string(layerStr.cbegin(), layerStr.cend());
    return tex;
}


oglTex3D_::~oglTex3D_()
{ }

void oglTex3D_::SetProperty(const TextureWrapVal wraptype) 
{ 
    SetWrapProperty(wraptype, wraptype, wraptype);
}

void oglTex3D_::SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void * data, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        CtxFunc->TextureSubImage3D(TextureID, GL_TEXTURE_3D, level, 0, 0, 0, Width >> level, Height >> level, Depth >> level, comptype, datatype, data);
    else
        CtxFunc->TextureImage3D(TextureID, GL_TEXTURE_3D, level, (GLint)OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, Depth >> level, 0, comptype, datatype, data);
}

void oglTex3D_::SetCompressedData(const bool isSub, const void * data, const size_t size, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (isSub)
        CtxFunc->CompressedTextureSubImage3D(TextureID, GL_TEXTURE_3D, level, 0, 0, 0, Width >> level, Height >> level, Depth >> level, OGLTexUtil::GetInnerFormat(InnerFormat), (GLsizei)size, data);
    else
        CtxFunc->CompressedTextureImage3D(TextureID, GL_TEXTURE_3D, level, OGLTexUtil::GetInnerFormat(InnerFormat), Width >> level, Height >> level, Depth >> level, 0, (GLsizei)size, data);
}

optional<vector<uint8_t>> oglTex3D_::GetCompressedData(const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    if (!IsCompressed())
        return {};
    GLint size = 0;
    CtxFunc->GetTextureLevelParameteriv(TextureID, GL_TEXTURE_3D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    optional<vector<uint8_t>> output(std::in_place, size);
    CtxFunc->GetCompressedTextureImage(TextureID, GL_TEXTURE_3D, level, size, (*output).data());
    return output;
}

vector<uint8_t> oglTex3D_::GetData(const TextureFormat format, const uint8_t level)
{
    CheckMipmapLevel(level);
    CheckCurrent();
    const auto[w, h, d] = GetInternalSize3();
    const auto size = (w * h * d * xziar::img::TexFormatUtil::BitPerPixel(format) / 8) >> (level * 3);
    vector<uint8_t> output(size);
    const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, false);
    CtxFunc->GetTextureImage(TextureID, GL_TEXTURE_3D, level, comptype, datatype, size, output.data());
    return output;
}

oglTex3DView_::oglTex3DView_(const oglTex3DStatic_& tex, const TextureFormat format) : oglTex3D_(false)
{
    Width = tex.Width, Height = tex.Height, Depth = tex.Depth, Mipmap = tex.Mipmap; InnerFormat = format;
    Name = tex.Name + u"-View";
    CtxFunc->TextureView(TextureID, GL_TEXTURE_3D, tex.TextureID, OGLTexUtil::GetInnerFormat(InnerFormat), 0, Mipmap, 0, 1);
}

oglTex3DStatic_::oglTex3DStatic_(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, const uint8_t mipmap) : oglTex3D_(true)
{
    CheckCurrent();
    if (width == 0 || height == 0 || depth == 0 || mipmap == 0)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Set size of 0 to Tex3D.");
    if (width % 4)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture's size should be aligned to 4 pixels");
    Width = width, Height = height, Depth = depth, InnerFormat = format, Mipmap = mipmap;
    CtxFunc->TextureStorage3D(TextureID, GL_TEXTURE_3D, mipmap, OGLTexUtil::GetInnerFormat(InnerFormat), Width, Height, Depth);
    if (const auto e = oglUtil::GetError(); e.has_value())
        oglLog().warning(u"oglTex3DS occurs error due to {}.\n", e.value());
}
oglTex3DS oglTex3DStatic_::Create(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, const uint8_t mipmap)
{
    return MAKE_ENABLER_SHARED(oglTex3DStatic_, (width, height, depth, format, mipmap));
}


void oglTex3DStatic_::SetData(const TextureFormat format, const void *data, const uint8_t level)
{
    oglTex3D_::SetData(true, format, data, level);
}

void oglTex3DStatic_::SetData(const TextureFormat format, const oglPBO& buf, const uint8_t level)
{
    oglTex3D_::SetData(true, format, buf, level);
}

void oglTex3DStatic_::SetData(const Image & img, const bool normalized, const bool flipY, const uint8_t level)
{
    if (img.GetWidth() != Width || img.GetHeight() != Height * Depth)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"image's size msmatch with oglTex3D(S)");
    oglTex3D_::SetData(true, img, normalized, flipY, level);
}

void oglTex3DStatic_::SetCompressedData(const void *data, const size_t size, const uint8_t level)
{
    oglTex3D_::SetCompressedData(true, data, size, level);
}

void oglTex3DStatic_::SetCompressedData(const oglPBO & buf, const size_t size, const uint8_t level)
{
    oglTex3D_::SetCompressedData(true, buf, size, level);
}

oglTex3DV oglTex3DStatic_::GetTextureView(const TextureFormat format) const
{
    CheckCurrent();
    CheckCompatible(InnerFormat, format);
    oglTex3DV tex(new oglTex3DView_(*this, InnerFormat));
    tex->Name = Name + u"-View";
    return tex;
}


oglBufferTexture_::oglBufferTexture_() noexcept : oglTexBase_(TextureType::TexBuf, true)
{ }

oglBufferTexture_::~oglBufferTexture_()
{ }
//oglBufTex oglBufferTexture_::Create()
//{
//    return MAKE_ENABLER_SHARED(oglBufferTexture_)();
//}

void oglBufferTexture_::SetBuffer(const TextureFormat format, const oglTBO& tbo)
{
    CheckCurrent();
    InnerBuf = tbo;
    InnerFormat = format;
    CtxFunc->TextureBuffer(TextureID, GL_TEXTURE_BUFFER, OGLTexUtil::GetInnerFormat(format), tbo->BufferID);
}


struct TIManCtxConfig : public CtxResConfig<false, std::unique_ptr<detail::ResourceBinder<oglImgBase_>>>
{
    std::unique_ptr<detail::ResourceBinder<oglImgBase_>> Construct() const
    {
        if (CtxFunc->SupportBindlessTexture)
            return std::make_unique<detail::BindlessImgManager>();
        else
            return std::make_unique<detail::CachedResManager<oglImgBase_>>(CtxFunc->MaxImageUnits);
    }
};
static TIManCtxConfig TEXIMGMAN_CTXCFG;
detail::ResourceBinder<oglImgBase_>& oglImgBase_::GetImgMan() noexcept
{
    return *oglContext_::CurrentContext()->GetOrCreate<false>(TEXIMGMAN_CTXCFG);
}


oglImgBase_::oglImgBase_(const oglTexBase& tex, const TexImgUsage usage, const bool isLayered)
    : InnerTex(tex), Usage(usage), IsLayered(isLayered)
{
    tex->CheckCurrent();
    if (!CtxFunc->SupportImageLoadStore)
        oglLog().warning(u"Attempt to create oglImg on unsupported context\n");
    if (!InnerTex)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Empty oglTex");
    const auto format = InnerTex->GetInnerFormat();
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROWEX(OGLWrongFormatException, u"TexImg does not support compressed texture type", format);
    if ((format & TextureFormat::MASK_DTYPE_CAT) == TextureFormat::DTYPE_CAT_COMP &&
        common::MatchAny(format & TextureFormat::MASK_COMP_RAW, 
            TextureFormat::COMP_332 , TextureFormat::COMP_4444, TextureFormat::COMP_565 ,
            TextureFormat::COMP_5551))
        COMMON_THROWEX(OGLWrongFormatException, u"TexImg does not support some composite texture type", format);
}
oglu::oglImgBase_::~oglImgBase_()
{
    GetImgMan().ReleaseRes(this);
}

bool oglu::oglImgBase_::CheckSupport()
{
    return CtxFunc->SupportImageLoadStore;
}

GLuint oglImgBase_::GetTextureID() const noexcept { return InnerTex ? InnerTex->TextureID : GL_INVALID_INDEX; }

void oglImgBase_::BindToUnit(const uint16_t pos) const noexcept
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
    CtxFunc->BindImageTexture(pos, GetTextureID(), 0, IsLayered ? GL_TRUE : GL_FALSE, 0, usage, OGLTexUtil::GetInnerFormat(InnerTex->GetInnerFormat()));
}

void oglImgBase_::PinToHandle() noexcept
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
    Handle = CtxFunc->GetImageHandle(GetTextureID(), 0, IsLayered ? GL_TRUE : GL_FALSE, 0, OGLTexUtil::GetInnerFormat(InnerTex->GetInnerFormat()));
    CtxFunc->MakeImageHandleResident(Handle.value(), usage);
}


oglImg2D_::oglImg2D_(const oglTex2D& tex, const TexImgUsage usage) : oglImgBase_(tex, usage, false) {}
oglImg2D oglImg2D_::Create(const oglTex2D& tex, const TexImgUsage usage)
{
    return MAKE_ENABLER_SHARED(oglImg2D_, (tex, usage));
}

oglImg3D_::oglImg3D_(const oglTex3D& tex, const TexImgUsage usage) : oglImgBase_(tex, usage, true) {}
oglImg3D oglImg3D_::Create(const oglTex3D& tex, const TexImgUsage usage)
{
    return MAKE_ENABLER_SHARED(oglImg3D_, (tex, usage));
}



GLenum OGLTexUtil::GetInnerFormat(const TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::R8:         return GL_R8;
    case TextureFormat::RG8:        return GL_RG8;
    case TextureFormat::RGB8:       return GL_RGB8;
    case TextureFormat::SRGB8:      return GL_SRGB8;
    case TextureFormat::RGBA8:      return GL_RGBA8;
    case TextureFormat::SRGBA8:     return GL_SRGB8_ALPHA8;
    case TextureFormat::R16:        return GL_R16;
    case TextureFormat::RG16:       return GL_RG16;
    case TextureFormat::RGB16:      return GL_RGB16;
    case TextureFormat::RGBA16:     return GL_RGBA16;
    case TextureFormat::R8S:        return GL_R8_SNORM;
    case TextureFormat::RG8S:       return GL_RG8_SNORM;
    case TextureFormat::RGB8S:      return GL_RGB8_SNORM;
    case TextureFormat::RGBA8S:     return GL_RGBA8_SNORM;
    case TextureFormat::R16S:       return GL_R16_SNORM;
    case TextureFormat::RG16S:      return GL_RG16_SNORM;
    case TextureFormat::RGB16S:     return GL_RGB16_SNORM;
    case TextureFormat::RGBA16S:    return GL_RGBA16_SNORM;
    case TextureFormat::R8U:        return GL_R8UI;
    case TextureFormat::RG8U:       return GL_RG8UI;
    case TextureFormat::RGB8U:      return GL_RGB8UI;
    case TextureFormat::RGBA8U:     return GL_RGBA8UI;
    case TextureFormat::R16U:       return GL_R16UI;
    case TextureFormat::RG16U:      return GL_RG16UI;
    case TextureFormat::RGB16U:     return GL_RGB16UI;
    case TextureFormat::RGBA16U:    return GL_RGBA16UI;
    case TextureFormat::R32U:       return GL_R32UI;
    case TextureFormat::RG32U:      return GL_RG32UI;
    case TextureFormat::RGB32U:     return GL_RGB32UI;
    case TextureFormat::RGBA32U:    return GL_RGBA32UI;
    case TextureFormat::R8I:        return GL_R8I;
    case TextureFormat::RG8I:       return GL_RG8I;
    case TextureFormat::RGB8I:      return GL_RGB8I;
    case TextureFormat::RGBA8I:     return GL_RGBA8I;
    case TextureFormat::R16I:       return GL_R16I;
    case TextureFormat::RG16I:      return GL_RG16I;
    case TextureFormat::RGB16I:     return GL_RGB16I;
    case TextureFormat::RGBA16I:    return GL_RGBA16I;
    case TextureFormat::R32I:       return GL_R32I;
    case TextureFormat::RG32I:      return GL_RG32I;
    case TextureFormat::RGB32I:     return GL_RGB32I;
    case TextureFormat::RGBA32I:    return GL_RGBA32I;
    case TextureFormat::Rh:         return GL_R16F;
    case TextureFormat::RGh:        return GL_RG16F;
    case TextureFormat::RGBh:       return GL_RGB16F;
    case TextureFormat::RGBAh:      return GL_RGBA16F;
    case TextureFormat::Rf:         return GL_R32F;
    case TextureFormat::RGf:        return GL_RG32F;
    case TextureFormat::RGBf:       return GL_RGB32F;
    case TextureFormat::RGBAf:      return GL_RGBA32F;
    case TextureFormat::RG11B10:    return GL_R11F_G11F_B10F;
    case TextureFormat::RGB332:     return GL_R3_G3_B2;
    case TextureFormat::RGBA4444:   return GL_RGBA4;
    case TextureFormat::RGB5A1:     return GL_RGB5_A1;
    case TextureFormat::RGB565:     return GL_RGB565;
    case TextureFormat::RGB10A2:    return GL_RGB10_A2;
    case TextureFormat::RGB10A2U:   return GL_RGB10_A2UI;
    //case TextureFormat::RGBA12:   return GL_RGB12;
    case TextureFormat::BC1:        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case TextureFormat::BC1A:       return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case TextureFormat::BC2:        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case TextureFormat::BC3:        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case TextureFormat::BC4:        return GL_COMPRESSED_RED_RGTC1;
    case TextureFormat::BC5:        return GL_COMPRESSED_RG_RGTC2;
    case TextureFormat::BC6H:       return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    case TextureFormat::BC7:        return GL_COMPRESSED_RGBA_BPTC_UNORM;
    case TextureFormat::BC1SRGB:    return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
    case TextureFormat::BC1ASRGB:   return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case TextureFormat::BC2SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case TextureFormat::BC3SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case TextureFormat::BC7SRGB:    return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    default:                        return GL_INVALID_ENUM;
    }
}


bool OGLTexUtil::ParseFormat(const TextureFormat format, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept
{
    const auto category = format & TextureFormat::MASK_DTYPE_CAT;
    const bool normalized = TexFormatUtil::IsNormalized(format);
    switch (category)
    {
    case TextureFormat::DTYPE_CAT_COMPRESS:
                                            return false;
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
        default:                            return false;
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
        default:                            return false;
        }
        break;
    default:                                return false;
    }
    switch (format & TextureFormat::MASK_CHANNEL)
    {
    case TextureFormat::CHANNEL_R:      comptype = normalized ? GL_RED   : GL_RED_INTEGER;   break;
    case TextureFormat::CHANNEL_G:      comptype = normalized ? GL_GREEN : GL_GREEN_INTEGER; break;
    case TextureFormat::CHANNEL_B:      comptype = normalized ? GL_BLUE  : GL_BLUE_INTEGER;  break;
    case TextureFormat::CHANNEL_A:      comptype = normalized ? GL_ALPHA : GL_ALPHA_INTEGER_EXT; break;
    case TextureFormat::CHANNEL_RG:     comptype = normalized ? GL_RG    : GL_RG_INTEGER;    break;
    case TextureFormat::CHANNEL_RGB:    comptype = normalized ? GL_RGB   : GL_RGB_INTEGER;   break;
    case TextureFormat::CHANNEL_BGR:    comptype = normalized ? GL_BGR   : GL_BGR_INTEGER;   break;
    case TextureFormat::CHANNEL_RGBA:   comptype = normalized ? GL_RGBA  : GL_RGBA_INTEGER;  break;
    case TextureFormat::CHANNEL_BGRA:   comptype = normalized ? GL_BGRA  : GL_BGRA_INTEGER;  break;
    default:                            return false;
    }
    return true;
}
bool OGLTexUtil::ParseFormat(const ImageDataType format, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
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
    default:                    return false;
    }
    return true;
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