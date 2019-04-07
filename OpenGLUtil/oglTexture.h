#pragma once
#include "oglRely.h"
#include "oglBuffer.h"
#include "oglException.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu
{


enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, Tex3D = GL_TEXTURE_3D, TexBuf = GL_TEXTURE_BUFFER, Tex2DArray = GL_TEXTURE_2D_ARRAY };

//[  flags|formats|channel|   bits]
//[15...13|12....8|7.....5|4.....0]
enum class TextureInnerFormat : uint16_t
{
    BITS_MASK = 0x001f, CHANNEL_MASK = 0x00e0, FORMAT_MASK = 0x1f00, FLAG_MASK = 0xe000, CAT_MASK = FORMAT_MASK | BITS_MASK, EMPTY_MASK = 0x0000, ERROR = 0xffff,
    //Bits
    BITS_2 = 0, BITS_4 = 2, BITS_5 = 3, BITS_8 = 4, BITS_10 = 5, BITS_11 = 6, BITS_12 = 7, BITS_16 = 8, BITS_32 = 9,
    //Channels[842]
    CHANNEL_R = 0x00, CHANNEL_RG = 0x20, CHANNEL_RA = CHANNEL_RG, CHANNEL_RGB = 0x40, CHANNEL_RGBA = 0x60, CHANNEL_ALPHA_MASK = 0x20,
    //Formats
    FORMAT_UNORM = 0x0000, FORMAT_SNORM = 0x0100, FORMAT_UINT = 0x0200, FORMAT_SINT = 0x0300, FORMAT_FLOAT = 0x0400,
    FORMAT_INT_SIGNED = 0x0100, FORMAT_INT_INTEGER = 0x0200,
    FORMAT_DXT1 = 0x0500, FORMAT_DXT3 = 0x0600, FORMAT_DXT5 = 0x0700, FORMAT_RGTC = 0x0800, FORMAT_BPTC = 0x0900,
    FORMAT_ETC = 0x0a00, FORMAT_ETC2 = 0x0b00, FORMAT_PVRTC = 0x0c00, FORMAT_PVRTC2 = 0x0d00, FORMAT_ASTC = 0x0e00,
    //Flags
    FLAG_SRGB = 0x8000, FLAG_COMP = 0x4000,
    //Categories
    CAT_UNORM8 = BITS_8 | FORMAT_UNORM, CAT_U8 = BITS_8 | FORMAT_UINT, CAT_SNORM8 = BITS_8 | FORMAT_SNORM, CAT_S8 = BITS_8 | FORMAT_SINT,
    CAT_UNORM16 = BITS_16 | FORMAT_UNORM, CAT_U16 = BITS_16 | FORMAT_UINT, CAT_SNORM16 = BITS_16 | FORMAT_SNORM, CAT_S16 = BITS_16 | FORMAT_SINT,
    CAT_UNORM32 = BITS_32 | FORMAT_UNORM, CAT_U32 = BITS_32 | FORMAT_UINT, CAT_SNORM32 = BITS_32 | FORMAT_SNORM, CAT_S32 = BITS_32 | FORMAT_SINT,
    CAT_FLOAT = BITS_32 | FORMAT_FLOAT, CAT_HALF = BITS_16 | FORMAT_FLOAT,
    //Actual Types
    //normalized integer->as float[0,1]
    R8 = CAT_UNORM8 | CHANNEL_R, RG8 = CAT_UNORM8 | CHANNEL_RG, RGB8 = CAT_UNORM8 | CHANNEL_RGB, RGBA8 = CAT_UNORM8 | CHANNEL_RGBA,
    SRGB8 = RGB8 | FLAG_SRGB, SRGBA8 = RGBA8 | FLAG_SRGB,
    R16 = CAT_UNORM16 | CHANNEL_R, RG16 = CAT_UNORM16 | CHANNEL_RG, RGB16 = CAT_UNORM16 | CHANNEL_RGB, RGBA16 = CAT_UNORM16 | CHANNEL_RGBA,
    //normalized integer->as float[-1,1]
    R8S = CAT_SNORM8 | CHANNEL_R, RG8S = CAT_SNORM8 | CHANNEL_RG, RGB8S = CAT_SNORM8 | CHANNEL_RGB, RGBA8S = CAT_SNORM8 | CHANNEL_RGBA,
    R16S = CAT_SNORM16 | CHANNEL_R, RG16S = CAT_SNORM16 | CHANNEL_RG, RGB16S = CAT_SNORM16 | CHANNEL_RGB, RGBA16S = CAT_SNORM16 | CHANNEL_RGBA,
    //non-normalized integer(unsigned)
    R8U = CAT_U8 | CHANNEL_R, RG8U = CAT_U8 | CHANNEL_RG, RGB8U = CAT_U8 | CHANNEL_RGB, RGBA8U = CAT_U8 | CHANNEL_RGBA,
    R16U = CAT_U16 | CHANNEL_R, RG16U = CAT_U16 | CHANNEL_RG, RGB16U = CAT_U16 | CHANNEL_RGB, RGBA16U = CAT_U16 | CHANNEL_RGBA,
    R32U = CAT_U32 | CHANNEL_R, RG32U = CAT_U32 | CHANNEL_RG, RGB32U = CAT_U32 | CHANNEL_RGB, RGBA32U = CAT_U32 | CHANNEL_RGBA,
    //non-normalized integer(signed)
    R8I = CAT_S8 | CHANNEL_R, RG8I = CAT_S8 | CHANNEL_RG, RGB8I = CAT_S8 | CHANNEL_RGB, RGBA8I = CAT_S8 | CHANNEL_RGBA,
    R16I = CAT_S16 | CHANNEL_R, RG16I = CAT_S16 | CHANNEL_RG, RGB16I = CAT_S16 | CHANNEL_RGB, RGBA16I = CAT_S16 | CHANNEL_RGBA,
    R32I = CAT_S32 | CHANNEL_R, RG32I = CAT_S32 | CHANNEL_RG, RGB32I = CAT_S32 | CHANNEL_RGB, RGBA32I = CAT_S32 | CHANNEL_RGBA,
    //half-float(FP16)
    Rh = CAT_HALF | CHANNEL_R, RGh = CAT_HALF | CHANNEL_RG, RGBh = CAT_HALF | CHANNEL_RGB, RGBAh = CAT_HALF | CHANNEL_RGBA,
    //float(FP32)
    Rf = CAT_FLOAT | CHANNEL_R, RGf = CAT_FLOAT | CHANNEL_RG, RGBf = CAT_FLOAT | CHANNEL_RGB, RGBAf = CAT_FLOAT | CHANNEL_RGBA,
    //special
    RG11B10 = BITS_11 | FORMAT_FLOAT | FLAG_COMP | CHANNEL_RGB, 
    RGBA4444 = BITS_4 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA, RGBA12 = BITS_12 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB,
    RGB332 = BITS_2 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB, RGB5A1 = BITS_5 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA, RGB565 = BITS_5 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB,
    RGB10A2 = BITS_10 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA, RGB10A2U = BITS_10 | FORMAT_UINT | FLAG_COMP | CHANNEL_RGBA,
    //compressed(S3TC/DXT135,RGTC,BPTC)
    BC1 = BITS_4 | FORMAT_DXT1 | CHANNEL_RGB, BC1A = BITS_4 | FORMAT_DXT1 | CHANNEL_RGBA, BC2 = BITS_8 | FORMAT_DXT3 | CHANNEL_RGBA, BC3 = BITS_8 | FORMAT_DXT5 | CHANNEL_RGBA,
    BC4 = BITS_4 | FORMAT_DXT5 | CHANNEL_R, BC5 = BITS_8 | FORMAT_DXT5 | CHANNEL_RG, BC6H = BITS_8 | FORMAT_BPTC | CHANNEL_RGB, BC7 = BITS_8 | FORMAT_BPTC | CHANNEL_RGBA,
    BC1SRGB = BC1 | FLAG_SRGB, BC1ASRGB = BC1A | FLAG_SRGB, BC2SRGB = BC2 | FLAG_SRGB, BC3SRGB = BC3 | FLAG_SRGB, BC7SRGB = BC7 | FLAG_SRGB,
};
MAKE_ENUM_BITFIELD(TextureInnerFormat)

//[reverse|component|normal|data type]
//[     15|14......8|     7|6.......0]
enum class TextureDataFormat : uint16_t
{
    TYPE_MASK = 0x00ff, INTEGER_MASK = 0x0080, TYPE_RAW_MASK = 0x007f, COMP_MASK = 0x0010, FORMAT_MASK = 0xff00, REVERSE_MASK = 0x8000, EMPTY_MASK = 0x0000,
    //format(channel)
    FORMAT_R = 0x0100, FORMAT_G = 0x0200, FORMAT_B = 0x0400, FORMAT_A = 0x0800, 
    FORMAT_RG = 0x0900, FORMAT_RGB = 0x0700, FORMAT_BGR = 0x8700, FORMAT_RGBA = 0x0f00, FORMAT_BGRA = 0x8f00,
    //data type
    TYPE_U8 = 0x0, TYPE_I8 = 0x1, TYPE_U16 = 0x2, TYPE_I16 = 0x3, TYPE_U32 = 0x4, TYPE_I32 = 0x5, TYPE_HALF = 0x6, TYPE_FLOAT = 0x7,
    TYPE_COMP = 0x10, TYPE_REV_MASK = 0x01,
    TYPE_332 = 0x10, TYPE_233R = 0x11, TYPE_565 = 0x12, TYPE_565R = 0x13, TYPE_4444 = 0x14, TYPE_4444R = 0x15, TYPE_5551 = 0x16, TYPE_1555R = 0x17,
    TYPE_8888 = 0x18, TYPE_8888R = 0x19, TYPE_10_2 = 0x1a, TYPE_10_2R = 0x1b, TYPE_11_10 = 0x1c, TYPE_11_10R = 0x1d,
    //normalized integer[0,1]
    R8 = FORMAT_R | TYPE_U8, RG8 = FORMAT_RG | TYPE_U8, RGB8 = FORMAT_RGB | TYPE_U8, BGR8 = FORMAT_BGR | TYPE_U8, RGBA8 = FORMAT_RGBA | TYPE_U8, BGRA8 = FORMAT_BGRA | TYPE_U8,
    R16 = FORMAT_R | TYPE_U16, RG16 = FORMAT_RG | TYPE_U16, RGB16 = FORMAT_RGB | TYPE_U16, BGR16 = FORMAT_BGR | TYPE_U16, RGBA16 = FORMAT_RGBA | TYPE_U16, BGRA16 = FORMAT_BGRA | TYPE_U16,
    //normalized integer[-1,1]
    R8S = FORMAT_R | TYPE_U8, RG8S = FORMAT_RG | TYPE_U8, RGB8S = FORMAT_RGB | TYPE_U8, BGR8S = FORMAT_BGR | TYPE_U8, RGBA8S = FORMAT_RGBA | TYPE_U8, BGRA8S = FORMAT_BGRA | TYPE_U8,
    R16S = FORMAT_R | TYPE_U16, RG16S = FORMAT_RG | TYPE_U16, RGB16S = FORMAT_RGB | TYPE_U16, BGR16S = FORMAT_BGR | TYPE_U16, RGBA16S = FORMAT_RGBA | TYPE_U16, BGRA16S = FORMAT_BGRA | TYPE_U16,
    //non-normalized integer(unsigned)
    R8U = R8 | INTEGER_MASK, RG8U = RG8 | INTEGER_MASK, RGB8U = RGB8 | INTEGER_MASK, BGR8U = BGR8 | INTEGER_MASK, RGBA8U = RGBA8 | INTEGER_MASK, BGRA8U = BGRA8 | INTEGER_MASK,
    R16U = R16 | INTEGER_MASK, RG16U = RG16 | INTEGER_MASK, RGB16U = RGB16 | INTEGER_MASK, BGR16U = BGR16 | INTEGER_MASK, RGBA16U = RGBA16 | INTEGER_MASK, BGRA16U = BGRA16 | INTEGER_MASK,
    R32U = FORMAT_R | TYPE_U32 | INTEGER_MASK, RG32U = FORMAT_RG | TYPE_U32 | INTEGER_MASK, RGB32U = FORMAT_RGB | TYPE_U32 | INTEGER_MASK, 
    BGR32U = FORMAT_BGR | TYPE_U32 | INTEGER_MASK, RGBA32U = FORMAT_RGBA | TYPE_U32 | INTEGER_MASK, BGRA32U = FORMAT_BGRA | TYPE_U32 | INTEGER_MASK,
    //non-normalized integer(signed)
    R8I = R8S | INTEGER_MASK, RG8I = RG8S | INTEGER_MASK, RGB8I = RGB8S | INTEGER_MASK, BGR8I = BGR8S | INTEGER_MASK, RGBA8I = RGBA8S | INTEGER_MASK, BGRA8I = BGRA8S | INTEGER_MASK,
    R16I = R16S | INTEGER_MASK, RG16I = RG16S | INTEGER_MASK, RGB16I = RGB16S | INTEGER_MASK, BGR16I = BGR16S | INTEGER_MASK, RGBA16I = RGBA16S | INTEGER_MASK, BGRA16I = BGRA16S | INTEGER_MASK,
    R32I = FORMAT_R | TYPE_I32 | INTEGER_MASK, RG32I = FORMAT_RG | TYPE_I32 | INTEGER_MASK, RGB32I = FORMAT_RGB | TYPE_I32 | INTEGER_MASK,
    BGR32I = FORMAT_BGR | TYPE_I32 | INTEGER_MASK, RGBA32I = FORMAT_RGBA | TYPE_I32 | INTEGER_MASK, BGRA32I = FORMAT_BGRA | TYPE_I32 | INTEGER_MASK,
    //half-float(FP16)
    Rh = FORMAT_R | TYPE_HALF, RGh = FORMAT_RG | TYPE_HALF, RGBh = FORMAT_RGB | TYPE_HALF, BGRh = FORMAT_BGR | TYPE_HALF, RGBAh = FORMAT_RGBA | TYPE_HALF, BGRAh = FORMAT_BGRA | TYPE_HALF,
    //float(FP32)
    Rf = FORMAT_R | TYPE_FLOAT, RGf = FORMAT_RG | TYPE_FLOAT, RGBf = FORMAT_RGB | TYPE_FLOAT, BGRf = FORMAT_BGR | TYPE_FLOAT, RGBAf = FORMAT_RGBA | TYPE_FLOAT, BGRAf = FORMAT_BGRA | TYPE_FLOAT,
    //composite
    RGB10A2 = FORMAT_RGBA | TYPE_10_2, RGB10A2U = RGB10A2 | INTEGER_MASK,
};
MAKE_ENUM_BITFIELD(TextureDataFormat)

class OGLWrongFormatException : public OGLException
{
public:
    EXCEPTION_CLONE_EX(OGLWrongFormatException);
    std::variant<TextureDataFormat, TextureInnerFormat> Format;
    OGLWrongFormatException(const std::u16string_view& msg, const TextureDataFormat format, const std::any& data_ = std::any())
        : OGLException(TYPENAME, GLComponent::OGLU, msg, data_), Format(format)
    { }
    OGLWrongFormatException(const std::u16string_view& msg, const TextureInnerFormat format, const std::any& data_ = std::any())
        : OGLException(TYPENAME, GLComponent::OGLU, msg, data_), Format(format)
    { }
    virtual ~OGLWrongFormatException() {}
};

//enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };
enum class TextureFilterVal : uint8_t
{
    LAYER_MASK = 0x0f, Linear = 0x01, Nearest = 0x02,
    MIPMAP_MASK = 0xf0, NoneMM = 0x00, LinearMM = 0x10, NearestMM = 0x20,
    BothLinear = Linear | LinearMM, BothNearest = Nearest | NearestMM,
};
MAKE_ENUM_BITFIELD(TextureFilterVal)

enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, ClampEdge = GL_CLAMP_TO_EDGE, ClampBorder = GL_CLAMP_TO_BORDER };

enum class TexImgUsage : GLenum { ReadOnly = GL_READ_ONLY, WriteOnly = GL_WRITE_ONLY, ReadWrite = GL_READ_WRITE };

struct OGLUAPI TexFormatUtil
{
    static GLenum GetInnerFormat(const TextureInnerFormat format) noexcept;
    static bool IsCompressType(const TextureInnerFormat format) noexcept 
    { 
        return static_cast<uint16_t>(format & TextureInnerFormat::FORMAT_MASK) > static_cast<uint16_t>(TextureInnerFormat::FORMAT_FLOAT);
    }
    static bool IsGrayType(const TextureInnerFormat format) noexcept
    {
        const auto channel = format & TextureInnerFormat::CHANNEL_MASK;
        return channel == TextureInnerFormat::CHANNEL_R || channel == TextureInnerFormat::CHANNEL_RA;
    }
    static bool IsAlphaType(const TextureInnerFormat format) noexcept
    {
        const auto channel = format & TextureInnerFormat::CHANNEL_MASK;
        return channel == TextureInnerFormat::CHANNEL_RA || channel == TextureInnerFormat::CHANNEL_RGBA;
    }
    static bool IsSRGBType(const TextureInnerFormat format) noexcept { return HAS_FIELD(format, TextureInnerFormat::FLAG_SRGB); }
    static TextureInnerFormat ConvertFrom(const xziar::img::ImageDataType dtype, const bool normalized) noexcept;
    static TextureInnerFormat ConvertFrom(const TextureDataFormat dformat) noexcept;

    static void ParseFormat(const TextureDataFormat dformat, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept;
    static std::pair<GLenum, GLenum> ParseFormat(const TextureDataFormat dformat, const bool isUpload) noexcept
    {
        GLenum datatype, comptype;
        ParseFormat(dformat, isUpload, datatype, comptype);
        return { datatype,comptype };
    }
    static void ParseFormat(const xziar::img::ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept;
    static std::pair<GLenum, GLenum> ParseFormat(const xziar::img::ImageDataType dformat, const bool normalized) noexcept
    {
        GLenum datatype, comptype;
        ParseFormat(dformat, normalized, datatype, comptype);
        return { datatype,comptype };
    }
    static size_t ParseFormatSize(const TextureInnerFormat dformat) noexcept;
    static size_t ParseFormatSize(const TextureDataFormat dformat) noexcept;
    static TextureDataFormat ConvertDtypeFrom(const TextureInnerFormat format);
    static TextureDataFormat ConvertDtypeFrom(const xziar::img::ImageDataType dtype, const bool normalized) noexcept;
    static xziar::img::ImageDataType ConvertToImgType(const TextureInnerFormat format, const bool relaxConvert = false) noexcept;
    static xziar::img::ImageDataType ConvertToImgType(const TextureDataFormat format, const bool relaxConvert = false) noexcept;
    static u16string_view GetTypeName(const TextureType type) noexcept;
    static u16string_view GetFormatName(const TextureInnerFormat format) noexcept;
    static string GetFormatDetail(const TextureInnerFormat format) noexcept;
    static string GetFormatDetail(const TextureDataFormat format) noexcept;
};


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
    GLuint textureID = GL_INVALID_INDEX;
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
    void Clear(const TextureDataFormat dformat);
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
    forceinline void SetData(const bool isSub, const TextureDataFormat dformat, const void *data, const uint8_t level)
    {
        const auto[datatype, comptype] = TexFormatUtil::ParseFormat(dformat, true);
        ((Base&)(*this)).SetData(isSub, datatype, comptype, data, level);
    }
    forceinline void SetData(const bool isSub, const TextureDataFormat dformat, const oglPBO& buf, const uint8_t level)
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
    vector<uint8_t> GetData(const TextureDataFormat dformat, const uint8_t level = 0);
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

    void SetData(const TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetData(const TextureDataFormat dformat, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<class T, class A>
    void SetData(const TextureDataFormat dformat, const vector<T, A>& data, const uint8_t level = 0)
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

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ConvertDtypeFrom(InnerFormat)); }

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
    
    void SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const void *data);
    void SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const oglPBO& buf);
    void SetData(const TextureInnerFormat iformat, const Image& img, const bool normalized = true, const bool flipY = true);
    template<class T, class A>
    void SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const vector<T, A>& data)
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
    void SetTextureLayer(const uint32_t layer, const TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetCompressedTextureLayer(const uint32_t layer, const void *data, const size_t size, const uint8_t level = 0);
    void SetTextureLayers(const uint32_t destLayer, const Wrapper<_oglTexture2DArray>& tex, const uint32_t srcLayer, const uint32_t layerCount);

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ConvertDtypeFrom(InnerFormat)); }

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
    vector<uint8_t> GetData(const TextureDataFormat dformat, const uint8_t level = 0);
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

    void SetData(const TextureDataFormat dformat, const void *data, const uint8_t level = 0);
    void SetData(const TextureDataFormat dformat, const oglPBO& buf, const uint8_t level = 0);
    void SetData(const Image& img, const bool normalized = true, const bool flipY = true, const uint8_t level = 0);
    template<class T, class A>
    void SetData(const TextureDataFormat dformat, const vector<T, A>& data, const uint8_t level = 0)
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

    void Clear() { _oglTexBase::Clear(TexFormatUtil::ConvertDtypeFrom(InnerFormat)); }

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