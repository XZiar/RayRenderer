#pragma once
#include "oglRely.h"
#include "oglBuffer.h"
#include "oglException.h"
#include "oglTexFormat.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{


enum class TextureFilterVal : uint8_t
{
    LAYER_MASK  = 0x0f, Linear   = 0x01, Nearest   = 0x02,
    MIPMAP_MASK = 0xf0, LinearMM = 0x10, NearestMM = 0x20, NoneMM = 0x00,
    BothLinear  = Linear | LinearMM, BothNearest   = Nearest | NearestMM,
};
MAKE_ENUM_BITFIELD(TextureFilterVal)

enum class TextureWrapVal : uint8_t { Repeat, ClampEdge, ClampBorder };

enum class TexImgUsage : uint8_t { ReadOnly, WriteOnly, ReadWrite };


namespace detail
{
using xziar::img::ImageDataType;
using xziar::img::Image;


class OGLUAPI _oglTexBase : public NonMovable, public oglCtxObject<true>
{
    friend class TextureManager;
    friend class _oglImgBase;
    friend class _oglFrameBuffer;
    friend class _oglProgram;
    friend class ProgState;
    friend class ProgDraw;
    friend struct TexLogItem;
    friend class ::oclu::detail::GLInterOP;
protected:
    const TextureType Type;
    TextureInnerFormat InnerFormat;
    GLuint textureID;
    uint8_t Mipmap;
    static TextureManager& getTexMan() noexcept;
    explicit _oglTexBase(const TextureType type, const bool shouldBindType) noexcept;
    void bind(const uint16_t pos) const noexcept;
    void unbind() const noexcept;
    void CheckMipmapLevel(const uint8_t level) const;
    std::pair<uint32_t, uint32_t> GetInternalSize2() const;
    std::tuple<uint32_t, uint32_t, uint32_t> GetInternalSize3() const;
    void SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT);
    void SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR);
    void Clear(const xziar::img::TextureDataFormat dformat);
public:
    u16string Name;
    virtual ~_oglTexBase() noexcept;
    void SetProperty(const TextureFilterVal magFilter, TextureFilterVal minFilter);
    void SetProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype) 
    {
        SetProperty(filtertype, filtertype); 
        SetProperty(wraptype);
    }
    void SetProperty(const TextureFilterVal filtertype) { SetProperty(filtertype, filtertype); }
    virtual void SetProperty(const TextureWrapVal wraptype) = 0;
    bool IsCompressed() const;
    uint8_t GetMipmapCount() const noexcept { return Mipmap; }
    TextureInnerFormat GetInnerFormat() const noexcept { return InnerFormat; }
};

template<typename Base>
struct _oglTexCommon
{
    forceinline void SetData(const bool isSub, const xziar::img::TextureDataFormat dformat, const void *data, const uint8_t level)
    {
        const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat, true);
        ((Base&)(*this)).SetData(isSub, datatype, comptype, data, level);
    }
    forceinline void SetData(const bool isSub, const xziar::img::TextureDataFormat dformat, const oglPBO& buf, const uint8_t level)
    {
        buf->bind();
        SetData(isSub, dformat, nullptr, level);
        buf->unbind();
    }
    forceinline void SetData(const bool isSub, const Image& img, const bool normalized, const bool flipY, const uint8_t level)
    {
        const auto[datatype, comptype] = TexFormatUtil::ParseFormat(img.GetDataType(), normalized);
        if (flipY)
        {
            const auto theimg = img.FlipToVertical();
            ((Base&)(*this)).SetData(isSub, datatype, comptype, theimg.GetRawPtr(), level);
        }
        else
        {
            ((Base&)(*this)).SetData(isSub, datatype, comptype, img.GetRawPtr(), level);
        }
    }
    forceinline void SetCompressedData(const bool isSub, const oglPBO& buf, const size_t size, const uint8_t level) noexcept
    {
        buf->bind();
        ((Base&)(*this)).SetCompressedData(isSub, nullptr, size, level);
        buf->unbind();
    }
};

class _oglTexture2DView;

class OGLUAPI _oglTexture2D : public _oglTexBase, protected _oglTexCommon<_oglTexture2D>
{
    friend _oglTexCommon<_oglTexture2D>;
    friend class _oglTexture2DArray;
    friend class _oglFrameBuffer;
protected:
    uint32_t Width, Height;

    explicit _oglTexture2D(const bool shouldBindType) noexcept 
        : _oglTexBase(TextureType::Tex2D, shouldBindType), Width(0), Height(0) {}

    void SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void *data, const uint8_t level);
    void SetCompressedData(const bool isSub, const void *data, const size_t size, const uint8_t level) noexcept;
    using _oglTexCommon<_oglTexture2D>::SetData;
    using _oglTexCommon<_oglTexture2D>::SetCompressedData;
public:
    std::pair<uint32_t, uint32_t> GetSize() const noexcept { return { Width, Height }; }
    virtual void SetProperty(const TextureWrapVal wraptype) override { SetWrapProperty(wraptype, wraptype); };
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT) { SetWrapProperty(wrapS, wrapT); }
    using _oglTexBase::SetProperty;
    optional<vector<uint8_t>> GetCompressedData(const uint8_t level = 0);
    vector<uint8_t> GetData(const xziar::img::TextureDataFormat dformat, const uint8_t level = 0);
    Image GetImage(const ImageDataType format, const bool flipY = true, const uint8_t level = 0);
};

class _oglTexture2DArray;
class _oglTexture2DStatic;
///<summary>Texture2D View, readonly</summary>  
class OGLUAPI _oglTexture2DView : public _oglTexture2D
{
    friend class _oglTexture2DArray;
    friend class _oglTexture2DStatic;
private:
    _oglTexture2DView(const _oglTexture2DStatic& tex, const TextureInnerFormat iformat);
    _oglTexture2DView(const _oglTexture2DArray& tex, const TextureInnerFormat iformat, const uint32_t layer);
};


class OGLUAPI _oglTexture2DStatic : public _oglTexture2D
{
    friend class _oglTexture2DView;
private:
public:
    _oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat, const uint8_t mipmap = 1);

    void SetData(const xziar::img::TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetData(const xziar::img::TextureDataFormat dformat, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<class T, class A>
    void SetData(const xziar::img::TextureDataFormat dformat, const vector<T, A>& data, const uint8_t level = 0)
    { 
        SetData(dformat, data.data(), level);
    }
    
    void SetCompressedData(const void *data, const size_t size, const uint8_t level = 0);
    void SetCompressedData(const oglPBO& buf, const size_t size, const uint8_t level = 0);
    template<class T, class A>
    void SetCompressedData(const vector<T, A>& data, const uint8_t level = 0)
    { 
        SetCompressedData(data.data(), data.size() * sizeof(T), level);
    }

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ToDType(InnerFormat)); }

    Wrapper<_oglTexture2DView> GetTextureView(const TextureInnerFormat format) const;
    Wrapper<_oglTexture2DView> GetTextureView() const { return GetTextureView(InnerFormat); }
};


class OGLUAPI _oglTexture2DDynamic : public _oglTexture2D
{
protected:
    void CheckAndSetMetadata(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h);
public:
    _oglTexture2DDynamic() noexcept : _oglTexture2D(true) {}

    using _oglTexBase::Clear;
    void GenerateMipmap();
    
    void SetData(const TextureInnerFormat iformat, const xziar::img::TextureDataFormat dformat, const uint32_t w, const uint32_t h, const void *data);
    void SetData(const TextureInnerFormat iformat, const xziar::img::TextureDataFormat dformat, const uint32_t w, const uint32_t h, const oglPBO& buf);
    void SetData(const TextureInnerFormat iformat, const Image& img, const bool normalized = true, const bool flipY = true);
    template<class T, class A>
    void SetData(const TextureInnerFormat iformat, const xziar::img::TextureDataFormat dformat, const uint32_t w, const uint32_t h, const vector<T, A>& data)
    { 
        SetData(iformat, dformat, w, h, data.data()); 
    }
    
    void SetCompressedData(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h, const void *data, const size_t size);
    void SetCompressedData(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h, const oglPBO& buf, const size_t size);
    template<class T, class A>
    void SetCompressedData(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h, const vector<T, A>& data)
    { 
        SetCompressedData(iformat, w, h, data.data(), data.size() * sizeof(T));
    }
};


///<summary>Texture2D Array, immutable only</summary>  
class OGLUAPI _oglTexture2DArray : public _oglTexBase
{
    friend class _oglFrameBuffer;
    friend class _oglTexture2DView;
private:
    uint32_t Width, Height, Layers;
    void CheckLayerRange(const uint32_t layer) const;
public:
    _oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureInnerFormat iformat, const uint8_t mipmap = 1);
    _oglTexture2DArray(const Wrapper<_oglTexture2DArray>& old, const uint32_t layerAdd);
    
    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Layers }; }

    virtual void SetProperty(const TextureWrapVal wraptype) override { SetWrapProperty(wraptype, wraptype); };
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT) { SetWrapProperty(wrapS, wrapT); }
    using _oglTexBase::SetProperty;
    void SetTextureLayer(const uint32_t layer, const Wrapper<_oglTexture2D>& tex);
    void SetTextureLayer(const uint32_t layer, const Image& img, const bool flipY = true, const uint8_t level = 0);
    void SetTextureLayer(const uint32_t layer, const xziar::img::TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size, const uint8_t level = 0);
    void SetTextureLayers(const uint32_t destLayer, const Wrapper<_oglTexture2DArray>& tex, const uint32_t srcLayer, const uint32_t layerCount);

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ToDType(InnerFormat)); }

    Wrapper<_oglTexture2DView> ViewTextureLayer(const uint32_t layer, const TextureInnerFormat format) const;
    Wrapper<_oglTexture2DView> ViewTextureLayer(const uint32_t layer) const { return ViewTextureLayer(layer, InnerFormat); }
};


class OGLUAPI _oglTexture3D : public _oglTexBase, protected _oglTexCommon<_oglTexture3D>
{
    friend _oglTexCommon<_oglTexture3D>;
    friend class ::oclu::detail::_oclGLBuffer;
protected:
    uint32_t Width, Height, Depth;

    explicit _oglTexture3D(const bool shouldBindType) noexcept 
        : _oglTexBase(TextureType::Tex3D, shouldBindType), Width(0), Height(0), Depth(0) {}

    void SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void *data, const uint8_t level);
    void SetCompressedData(const bool isSub, const void *data, const size_t size, const uint8_t level);
    using _oglTexCommon<_oglTexture3D>::SetData;
    using _oglTexCommon<_oglTexture3D>::SetCompressedData;
public:
    virtual void SetProperty(const TextureWrapVal wraptype) override { SetWrapProperty(wraptype, wraptype, wraptype); };
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR) { SetWrapProperty(wrapS, wrapT, wrapR); }
    using _oglTexBase::SetProperty;
    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const noexcept { return { Width, Height, Depth }; }
    optional<vector<uint8_t>> GetCompressedData(const uint8_t level = 0);
    vector<uint8_t> GetData(const xziar::img::TextureDataFormat dformat, const uint8_t level = 0);
};

class _oglTexture3DStatic;
///<summary>Texture3D View, readonly</summary>  
class OGLUAPI _oglTexture3DView : public _oglTexture3D
{
    friend class _oglTexture3DStatic;
private:
    _oglTexture3DView(const _oglTexture3DStatic& tex, const TextureInnerFormat iformat);
};

class OGLUAPI _oglTexture3DStatic : public _oglTexture3D
{
    friend class _oglTexture3DView;
private:
public:
    _oglTexture3DStatic(const uint32_t width, const uint32_t height, const uint32_t depth, const TextureInnerFormat iformat, const uint8_t mipmap = 1);

    void SetData(const xziar::img::TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetData(const xziar::img::TextureDataFormat dformat, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<class T, class A>
    void SetData(const xziar::img::TextureDataFormat dformat, const vector<T, A>& data, const uint8_t level = 0)
    {
        SetData(dformat, data.data(), level);
    }

    void SetCompressedData(const void *data, const size_t size, const uint8_t level = 0);
    void SetCompressedData(const oglPBO& buf, const size_t size, const uint8_t level = 0);
    template<class T, class A>
    void SetCompressedData(const vector<T, A>& data, const uint8_t level = 0)
    {
        SetCompressedData(data.data(), data.size() * sizeof(T), level);
    }

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ToDType(InnerFormat)); }

    Wrapper<_oglTexture3DView> GetTextureView(const TextureInnerFormat format) const;
    Wrapper<_oglTexture3DView> GetTextureView() const { return GetTextureView(InnerFormat); }
};


class OGLUAPI _oglBufferTexture : public _oglTexBase
{
protected:
    oglTBO InnerBuf;
public:
    _oglBufferTexture() noexcept;
    virtual ~_oglBufferTexture() noexcept {}
    void SetBuffer(const TextureInnerFormat iformat, const oglTBO& tbo);
};


class OGLUAPI _oglImgBase : public NonMovable, public oglCtxObject<true>
{
    friend class TexImgManager;
    friend class _oglProgram;
    friend class ProgState;
    friend class ProgDraw;
protected:
    Wrapper<_oglTexBase> InnerTex;
    TexImgUsage Usage;
    const bool IsLayered;
    static TexImgManager& getImgMan() noexcept;
    GLuint GetTextureID() const noexcept { return InnerTex ? InnerTex->textureID : GL_INVALID_INDEX; }
    void bind(const uint16_t pos) const noexcept;
    void unbind() const noexcept;
    _oglImgBase(const Wrapper<_oglTexBase>& tex, const TexImgUsage usage, const bool isLayered);
public:
    TextureType GetType() const { return InnerTex->Type; }
};

class OGLUAPI _oglImg2D : public _oglImgBase
{
public:
    _oglImg2D(const Wrapper<detail::_oglTexture2D>& tex, const TexImgUsage usage);
};

class OGLUAPI _oglImg3D : public _oglImgBase
{
public:
    _oglImg3D(const Wrapper<detail::_oglTexture3D>& tex, const TexImgUsage usage);
};

}

using oglTexBase    = Wrapper<detail::_oglTexBase>;
using oglTex2D      = Wrapper<detail::_oglTexture2D>;
using oglTex2DS     = Wrapper<detail::_oglTexture2DStatic>;
using oglTex2DD     = Wrapper<detail::_oglTexture2DDynamic>;
using oglTex2DV     = Wrapper<detail::_oglTexture2DView>;
using oglTex2DArray = Wrapper<detail::_oglTexture2DArray>;
using oglTex3D      = Wrapper<detail::_oglTexture3D>;
using oglTex3DS     = Wrapper<detail::_oglTexture3DStatic>;
using oglTex3DV     = Wrapper<detail::_oglTexture3DView>;
using oglBufTex     = Wrapper<detail::_oglBufferTexture>;
using oglImgBase    = Wrapper<detail::_oglImgBase>;
using oglImg2D      = Wrapper<detail::_oglImg2D>;
using oglImg3D      = Wrapper<detail::_oglImg3D>;


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif