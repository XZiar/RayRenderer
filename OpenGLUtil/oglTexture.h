#pragma once
#include "oglRely.h"
#include "oglBuffer.h"
#include "oglException.h"
#include "ImageUtil/TexFormat.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{
class oglTexBase_;
using oglTexBase    = std::shared_ptr<oglTexBase_>;
class oglTex2D_;
using oglTex2D      = std::shared_ptr<oglTex2D_>;
class oglTex2DStatic_;
using oglTex2DS     = std::shared_ptr<oglTex2DStatic_>;
class oglTex2DDynamic_;
using oglTex2DD     = std::shared_ptr<oglTex2DDynamic_>;
class oglTex2DView_;
using oglTex2DV     = std::shared_ptr<oglTex2DView_>;
class oglTex2DArray_;
using oglTex2DArray = std::shared_ptr<oglTex2DArray_>;
class oglTex3D_;
using oglTex3D      = std::shared_ptr<oglTex3D_>;
class oglTex3DStatic_;
using oglTex3DS     = std::shared_ptr<oglTex3DStatic_>;
class oglTex3DView_;
using oglTex3DV     = std::shared_ptr<oglTex3DView_>;
class oglBufferTexture_;
using oglBufTex     = std::shared_ptr<oglBufferTexture_>;
class oglImgBase_;
using oglImgBase    = std::shared_ptr<oglImgBase_>;
class oglImg2D_;
using oglImg2D      = std::shared_ptr<oglImg2D_>;
class oglImg3D_;
using oglImg3D      = std::shared_ptr<oglImg3D_>;


enum class TextureType : GLenum
{
    Tex2D      = 0x0DE1/*GL_TEXTURE_2D*/,
    Tex3D      = 0x806F/*GL_TEXTURE_3D*/,
    TexBuf     = 0x8C2A/*GL_TEXTURE_BUFFER*/,
    Tex2DArray = 0x8C1A/*GL_TEXTURE_2D_ARRAY*/
};


class OGLWrongFormatException : public OGLException
{
public:
    EXCEPTION_CLONE_EX(OGLWrongFormatException);
    xziar::img::TextureFormat Format;
    OGLWrongFormatException(const std::u16string_view& msg, const xziar::img::TextureFormat format, const std::any& data_ = std::any())
        : OGLException(TYPENAME, GLComponent::OGLU, msg, data_), Format(format)
    { }
    virtual ~OGLWrongFormatException() {}
};

struct OGLUAPI OGLTexUtil
{
    [[nodiscard]] static GLenum GetInnerFormat(const xziar::img::TextureFormat format) noexcept;
    [[nodiscard]] static bool ParseFormat(const xziar::img::TextureFormat dformat, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept;
    [[nodiscard]] static std::pair<GLenum, GLenum> ParseFormat(const xziar::img::TextureFormat dformat, const bool isUpload) noexcept
    {
        GLenum datatype, comptype;
        if (ParseFormat(dformat, isUpload, datatype, comptype))
            return { datatype,comptype };
        else
            return { GLInvalidEnum, GLInvalidEnum };
    }
    [[nodiscard]] static bool ParseFormat(const xziar::img::ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept;
    [[nodiscard]] static std::pair<GLenum, GLenum> ParseFormat(const xziar::img::ImageDataType dformat, const bool normalized) noexcept
    {
        GLenum datatype, comptype;
        if (ParseFormat(dformat, normalized, datatype, comptype))
            return { datatype,comptype };
        else
            return { GLInvalidEnum, GLInvalidEnum };
    }

    [[nodiscard]] static std::u16string_view GetTypeName(const TextureType type) noexcept;
};


enum class TextureFilterVal : uint8_t
{
    LAYER_MASK  = 0x0f, Linear   = 0x01, Nearest   = 0x02,
    MIPMAP_MASK = 0xf0, LinearMM = 0x10, NearestMM = 0x20, NoneMM = 0x00,
    BothLinear  = Linear | LinearMM, BothNearest   = Nearest | NearestMM,
};
MAKE_ENUM_BITFIELD(TextureFilterVal)

enum class TextureWrapVal : uint8_t { Repeat, ClampEdge, ClampBorder };

enum class TexImgUsage : uint8_t { ReadOnly, WriteOnly, ReadWrite };



class OGLUAPI oglTexBase_ : public common::NonMovable, public detail::oglCtxObject<true>
{
    friend class detail::TextureManager;
    friend class oglImgBase_;
    friend class oglCustomFrameBuffer_;
    friend class oglProgram_;
    friend class ProgState;
    friend class ProgDraw;
    friend struct TexLogItem;
    friend class ::oclu::GLInterop;
protected:
    std::u16string Name;
    const TextureType Type;
    xziar::img::TextureFormat InnerFormat;
    GLuint TextureID;
    uint8_t Mipmap;
    static detail::TextureManager& getTexMan() noexcept;
    explicit oglTexBase_(const TextureType type, const bool shouldBindType) noexcept;
    void bind(const uint16_t pos) const noexcept;
    void unbind() const noexcept;
    void CheckMipmapLevel(const uint8_t level) const;
    std::pair<uint32_t, uint32_t> GetInternalSize2() const;
    std::tuple<uint32_t, uint32_t, uint32_t> GetInternalSize3() const;
    void SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT);
    void SetWrapProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR);
    void Clear(const xziar::img::TextureFormat format);
public:
    virtual ~oglTexBase_();
    void SetProperty(const TextureFilterVal magFilter, TextureFilterVal minFilter);
    void SetProperty(const TextureFilterVal filtertype) { SetProperty(filtertype, filtertype); }
    virtual void SetProperty(const TextureWrapVal wraptype) = 0;
    void SetProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype)
    {
        SetProperty(filtertype, filtertype);
        SetProperty(wraptype);
    }
    void SetName(std::u16string name) noexcept;

    [[nodiscard]] bool IsCompressed() const;
    [[nodiscard]] uint8_t GetMipmapCount() const noexcept { return Mipmap; }
    [[nodiscard]] xziar::img::TextureFormat GetInnerFormat() const noexcept { return InnerFormat; }
    [[nodiscard]] std::u16string_view GetName() const noexcept { return Name; }
};


template<typename Base>
struct oglTexCommon_
{
    forceinline void SetData(const bool isSub, const xziar::img::TextureFormat format, const void *data, const uint8_t level)
    {
        const auto[datatype, comptype] = OGLTexUtil::ParseFormat(format, true);
        ((Base&)(*this)).SetData(isSub, datatype, comptype, data, level);
    }
    forceinline void SetData(const bool isSub, const xziar::img::TextureFormat format, const oglPBO& buf, const uint8_t level)
    {
        buf->bind();
        SetData(isSub, format, nullptr, level);
        buf->unbind();
    }
    forceinline void SetData(const bool isSub, const xziar::img::Image& img, const bool normalized, const bool flipY, const uint8_t level)
    {
        const auto[datatype, comptype] = OGLTexUtil::ParseFormat(img.GetDataType(), normalized);
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


class OGLUAPI oglTex2D_ : public oglTexBase_, protected oglTexCommon_<oglTex2D_>
{
    friend oglTexCommon_<oglTex2D_>;
    friend class oglTex2DArray_;
    friend class oglCustomFrameBuffer_;
protected:
    uint32_t Width, Height;

    explicit oglTex2D_(const bool shouldBindType) noexcept 
        : oglTexBase_(TextureType::Tex2D, shouldBindType), Width(0), Height(0) {}

    void SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void *data, const uint8_t level);
    void SetCompressedData(const bool isSub, const void *data, const size_t size, const uint8_t level) noexcept;
    using oglTexCommon_<oglTex2D_>::SetData;
    using oglTexCommon_<oglTex2D_>::SetCompressedData;
public:
    ~oglTex2D_() override;
    void SetProperty(const TextureWrapVal wraptype) override;

    [[nodiscard]] std::pair<uint32_t, uint32_t> GetSize() const noexcept { return { Width, Height }; }
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT) { SetWrapProperty(wrapS, wrapT); }
    using oglTexBase_::SetProperty;
    [[nodiscard]] std::optional<std::vector<uint8_t>> GetCompressedData(const uint8_t level = 0);
    [[nodiscard]] std::vector<uint8_t> GetData(const xziar::img::TextureFormat format, const uint8_t level = 0);
    [[nodiscard]] xziar::img::Image GetImage(const xziar::img::ImageDataType format, const bool flipY = true, const uint8_t level = 0);
};


class oglTex2DArray_;
class oglTex2DStatic_;
///<summary>Texture2D View, readonly</summary>  
class OGLUAPI oglTex2DView_: public oglTex2D_
{
    friend class oglTex2DArray_;
    friend class oglTex2DStatic_;
private:
    MAKE_ENABLER();
    oglTex2DView_(const oglTex2DStatic_& tex, const xziar::img::TextureFormat format);
    oglTex2DView_(const oglTex2DArray_& tex, const xziar::img::TextureFormat format, const uint32_t layer);
};


class OGLUAPI oglTex2DStatic_ : public oglTex2D_
{
    friend class oglTex2DView_;
private:
    MAKE_ENABLER();
    oglTex2DStatic_(const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
public:
    void SetData(const xziar::img::TextureFormat format, const common::span<const std::byte> space, const uint8_t level = 0);
    void SetData(const xziar::img::TextureFormat format, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const xziar::img::Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<typename T>
    void SetData(const xziar::img::TextureFormat format, const common::span<T> space, const uint8_t level = 0)
    {
        SetData(format, common::as_bytes(space), level);
    }
    template<typename T>
    void SetData(const xziar::img::TextureFormat format, const T& cont, const uint8_t level = 0)
    { 
        SetData(format, common::as_bytes(common::to_span(cont)), level);
    }
    
    void SetCompressedData(const common::span<const std::byte> space, const uint8_t level = 0);
    void SetCompressedData(const oglPBO& buf, const size_t size, const uint8_t level = 0);
    template<typename T>
    void SetCompressedData(const common::span<T> space, const uint8_t level = 0)
    {
        SetCompressedData(common::as_bytes(space), level);
    }
    template<typename T>
    void SetCompressedData(const T& cont, const uint8_t level = 0)
    {
        SetCompressedData(common::as_bytes(common::to_span(cont)), level);
    }

    void Clear() { oglTexBase_::Clear(InnerFormat); }

    [[nodiscard]] oglTex2DV GetTextureView(const xziar::img::TextureFormat format) const;
    [[nodiscard]] oglTex2DV GetTextureView() const { return GetTextureView(InnerFormat); }

    [[nodiscard]] static oglTex2DS Create(const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
};


class OGLUAPI oglTex2DDynamic_ : public oglTex2D_
{
protected:
    MAKE_ENABLER();
    oglTex2DDynamic_() noexcept : oglTex2D_(true) {}
    void CheckAndSetMetadata(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h);
public:

    using oglTexBase_::Clear;
    void GenerateMipmap();
    
    void SetData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const common::span<const std::byte> space);
    void SetData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf);
    void SetData(const xziar::img::TextureFormat format, const xziar::img::Image& img, const bool normalized = true, const bool flipY = true);
    template<typename T>
    void SetData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const common::span<T> space)
    { 
        SetData(format, w, h, common::as_bytes(space));
    }
    template<typename T>
    void SetData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const T& cont)
    {
        SetData(format, w, h, common::as_bytes(common::to_span(cont)));
    }
    
    void SetCompressedData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const common::span<const std::byte> space);
    void SetCompressedData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const oglPBO& buf, const size_t size);
    template<typename T>
    void SetCompressedData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const common::span<T> space)
    { 
        SetCompressedData(format, w, h, common::as_bytes(space));
    }
    template<typename T>
    void SetCompressedData(const xziar::img::TextureFormat format, const uint32_t w, const uint32_t h, const T& cont)
    {
        SetCompressedData(format, w, h, common::as_bytes(common::to_span(cont)));
    }

    [[nodiscard]] static oglTex2DD Create();
};


///<summary>Texture2D Array, immutable only</summary>  
class OGLUAPI oglTex2DArray_ : public oglTexBase_
{
    friend class oglCustomFrameBuffer_;
    friend class oglFrameBuffer2D_;
    friend class oglLayeredFrameBuffer_;
    friend class oglTex2DView_;
private:
    MAKE_ENABLER();
    uint32_t Width, Height, Layers;

    oglTex2DArray_(const uint32_t width, const uint32_t height, const uint32_t layers, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
    oglTex2DArray_(const oglTex2DArray& old, const uint32_t layerAdd);
    void CheckLayerRange(const uint32_t layer) const;
public:
    ~oglTex2DArray_() override;
    void SetProperty(const TextureWrapVal wraptype) override;

    [[nodiscard]] std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Layers }; }
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT) { SetWrapProperty(wrapS, wrapT); }
    using oglTexBase_::SetProperty;
    void SetTextureLayer(const uint32_t layer, const oglTex2D& tex);
    void SetTextureLayer(const uint32_t layer, const xziar::img::Image& img, const bool flipY = true, const uint8_t level = 0);
    void SetTextureLayer(const uint32_t layer, const xziar::img::TextureFormat format, const void *data, const uint8_t level = 0);
    void SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size, const uint8_t level = 0);
    void SetTextureLayers(const uint32_t destLayer, const oglTex2DArray& tex, const uint32_t srcLayer, const uint32_t layerCount);

    void Clear() { oglTexBase_::Clear(InnerFormat); }

    [[nodiscard]] oglTex2DV ViewTextureLayer(const uint32_t layer, const xziar::img::TextureFormat format) const;
    [[nodiscard]] oglTex2DV ViewTextureLayer(const uint32_t layer) const { return ViewTextureLayer(layer, InnerFormat); }

    [[nodiscard]] static oglTex2DArray Create(const uint32_t width, const uint32_t height, const uint32_t layers, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
    [[nodiscard]] static oglTex2DArray Create(const oglTex2DArray& old, const uint32_t layerAdd);
};


class OGLUAPI oglTex3D_ : public oglTexBase_, protected oglTexCommon_<oglTex3D_>
{
    friend oglTexCommon_<oglTex3D_>;
    friend class ::oclu::oclGLBuffer_;
    friend class oglCustomFrameBuffer_;
    friend class oglLayeredFrameBuffer_;
protected:
    uint32_t Width, Height, Depth;

    explicit oglTex3D_(const bool shouldBindType) noexcept 
        : oglTexBase_(TextureType::Tex3D, shouldBindType), Width(0), Height(0), Depth(0) {}

    void SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void *data, const uint8_t level);
    void SetCompressedData(const bool isSub, const void *data, const size_t size, const uint8_t level);
    using oglTexCommon_<oglTex3D_>::SetData;
    using oglTexCommon_<oglTex3D_>::SetCompressedData;
public:
    ~oglTex3D_() override;
    void SetProperty(const TextureWrapVal wraptype) override;
    
    void SetProperty(const TextureWrapVal wrapS, const TextureWrapVal wrapT, const TextureWrapVal wrapR) { SetWrapProperty(wrapS, wrapT, wrapR); }
    using oglTexBase_::SetProperty;
    [[nodiscard]] std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const noexcept { return { Width, Height, Depth }; }
    [[nodiscard]] std::optional<std::vector<uint8_t>> GetCompressedData(const uint8_t level = 0);
    [[nodiscard]] std::vector<uint8_t> GetData(const xziar::img::TextureFormat format, const uint8_t level = 0);
};

class oglTex3DStatic_;
///<summary>Texture3D View, readonly</summary>  
class OGLUAPI oglTex3DView_ : public oglTex3D_
{
    friend class oglTex3DStatic_;
private:
    MAKE_ENABLER();
    oglTex3DView_(const oglTex3DStatic_& tex, const xziar::img::TextureFormat format);
};

class OGLUAPI oglTex3DStatic_ : public oglTex3D_
{
    friend class oglTex3DView_;
private:
    MAKE_ENABLER();
    oglTex3DStatic_(const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
public:
    void SetData(const xziar::img::TextureFormat format, const void *data, const uint8_t level = 0);
    void SetData(const xziar::img::TextureFormat format, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const xziar::img::Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<class T, class A>
    void SetData(const xziar::img::TextureFormat format, const std::vector<T, A>& data, const uint8_t level = 0)
    {
        SetData(format, data.data(), level);
    }

    void SetCompressedData(const void *data, const size_t size, const uint8_t level = 0);
    void SetCompressedData(const oglPBO& buf, const size_t size, const uint8_t level = 0);
    template<class T, class A>
    void SetCompressedData(const std::vector<T, A>& data, const uint8_t level = 0)
    {
        SetCompressedData(data.data(), data.size() * sizeof(T), level);
    }

    void Clear() { oglTexBase_::Clear(InnerFormat); }

    [[nodiscard]] oglTex3DV GetTextureView(const xziar::img::TextureFormat format) const;
    [[nodiscard]] oglTex3DV GetTextureView() const { return GetTextureView(InnerFormat); }

    [[nodiscard]] static oglTex3DS Create(const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const uint8_t mipmap = 1);
};


class OGLUAPI oglBufferTexture_ : public oglTexBase_
{
private:
    MAKE_ENABLER();
    oglBufferTexture_() noexcept;
protected:
    oglTBO InnerBuf;
public:
    //static oglBufTex Create();
    ~oglBufferTexture_() override;
    void SetBuffer(const xziar::img::TextureFormat format, const oglTBO& tbo);
};


class OGLUAPI oglImgBase_ : public common::NonMovable, public detail::oglCtxObject<true>
{
    friend class detail::TexImgManager;
    friend class oglProgram_;
    friend class ProgState;
    friend class ProgDraw;
protected:
    oglTexBase InnerTex;
    TexImgUsage Usage;
    const bool IsLayered;
    static detail::TexImgManager& getImgMan() noexcept;
    GLuint GetTextureID() const noexcept;
    void bind(const uint16_t pos) const noexcept;
    void unbind() const noexcept;
    oglImgBase_(const oglTexBase& tex, const TexImgUsage usage, const bool isLayered);
public:
    TextureType GetType() const { return InnerTex->Type; }

    [[nodiscard]] static bool CheckSupport();
};

class OGLUAPI oglImg2D_ : public oglImgBase_
{
private:
    MAKE_ENABLER();
    oglImg2D_(const oglTex2D& tex, const TexImgUsage usage);
public:
    [[nodiscard]] static oglImg2D Create(const oglTex2D& tex, const TexImgUsage usage);
};

class OGLUAPI oglImg3D_ : public oglImgBase_
{
private:
    MAKE_ENABLER();
    oglImg3D_(const oglTex3D& tex, const TexImgUsage usage);
public:
    [[nodiscard]] static oglImg3D Create(const oglTex3D& tex, const TexImgUsage usage);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif