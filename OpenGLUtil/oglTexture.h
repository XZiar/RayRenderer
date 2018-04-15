#pragma once
#include "oglRely.h"
#include "oglBuffer.h"

namespace oglu
{


enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, TexBuf = GL_TEXTURE_BUFFER, Tex2DArray = GL_TEXTURE_2D_ARRAY };

//upper for format, lower for type
enum class TextureDataFormat : uint8_t
{
    NORMAL_MASK = 0x80, TYPE_MASK = 0x0f, FORMAT_MASK = 0xf0, RAW_FORMAT_MASK = 0x70,
    R_FORMAT = 0x00, RG_FORMAT = 0x10, RGB_FORMAT = 0x20, BGR_FORMAT = 0x30, RGBA_FORMAT = 0x40, BGRA_FORMAT = 0x50,
    U8_TYPE = 0x0, I8_TYPE = 0x1, U16_TYPE = 0x2, I16_TYPE = 0x3, U32_TYPE = 0x4, I32_TYPE = 0x5, HALF_TYPE = 0x6, FLOAT_TYPE = 0x7,
    //non-normalized integer(uint8)
    R8U = R_FORMAT | U8_TYPE, RG8U = RG_FORMAT | U8_TYPE, RGB8U = RGB_FORMAT | U8_TYPE, BGR8U = BGR_FORMAT | U8_TYPE, RGBA8U = RGBA_FORMAT | U8_TYPE, BGRA8U = BGRA_FORMAT | U8_TYPE,
    //normalized integer(uint8)
    R8 = R8U | NORMAL_MASK, RG8 = RG8U | NORMAL_MASK, RGB8 = RGB8U | NORMAL_MASK, BGR8 = BGR8U | NORMAL_MASK, RGBA8 = RGBA8U | NORMAL_MASK, BGRA8 = BGRA8U | NORMAL_MASK,
    //float(FP32)
    Rf = R_FORMAT | FLOAT_TYPE | NORMAL_MASK, RGf = RG_FORMAT | FLOAT_TYPE | NORMAL_MASK, RGBf = RGB_FORMAT | FLOAT_TYPE | NORMAL_MASK, BGRf = BGR_FORMAT | FLOAT_TYPE | NORMAL_MASK, RGBAf = RGBA_FORMAT | FLOAT_TYPE | NORMAL_MASK, BGRAf = BGRA_FORMAT | FLOAT_TYPE | NORMAL_MASK,
};
MAKE_ENUM_BITFIELD(TextureDataFormat)
enum class TextureInnerFormat : GLint
{
    //normalized integer->as float[0,1]
    R8 = GL_R8, RG8 = GL_RG8, RGB8 = GL_RGB8, RGBA8 = GL_RGBA8,
    //non-normalized integer(uint8)
    R8U = GL_R8UI, RG8U = GL_RG8UI, RGB8U = GL_RGB8UI, RGBA8U = GL_RGBA8UI,
    //float(FP32)
    Rf = GL_R32F, RGf = GL_RG32F, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F,
    //compressed(S3TC/DXT1,S3TC/DXT3,S3TC/DXT5,RGTC,BPTC
    BC1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT, BC1A = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BC2 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
    BC3 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BC4 = GL_COMPRESSED_RED_RGTC1, BC5 = GL_COMPRESSED_RG_RGTC2,
    BC6H = GL_COMPRESSED_RGBA_BPTC_UNORM, BC7 = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
};

enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };

enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, Clamp = GL_CLAMP, };


namespace detail
{
using xziar::img::ImageDataType;
using xziar::img::Image;


class OGLUAPI _oglTexBase : public NonCopyable, public NonMovable
{
    friend class TextureManager;
    friend class _oglProgram;
    friend class ProgState;
    friend class ProgDraw;
    friend class ::oclu::detail::_oclGLBuffer;
    friend struct TexLogItem;
protected:
    const TextureType Type;
    TextureInnerFormat InnerFormat;
    GLuint textureID = GL_INVALID_INDEX;
    static TextureManager& getTexMan() noexcept;
    explicit _oglTexBase(const TextureType type) noexcept;
    void bind(const uint16_t pos) const noexcept;
    void unbind() const noexcept;
    std::pair<uint32_t, uint32_t> GetInternalSize2() const;
    std::tuple<uint32_t, uint32_t, uint32_t> GetInternalSize3() const;
public:
    ~_oglTexBase() noexcept;
    void SetProperty(const TextureFilterVal magFilter, const TextureFilterVal minFilter, const TextureWrapVal wrapS, const TextureWrapVal wrapT);
    void SetProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype) { SetProperty(filtertype, filtertype, wraptype, wraptype); }
    bool IsCompressed() const;

    static void ParseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept;
    static std::pair<GLenum, GLenum> ParseFormat(const TextureDataFormat dformat) noexcept
    {
        GLenum datatype, comptype;
        ParseFormat(dformat, datatype, comptype);
        return { datatype,comptype };
    }
    static void ParseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept;
    static std::pair<GLenum, GLenum> ParseFormat(const ImageDataType dformat, const bool normalized) noexcept
    {
        GLenum datatype, comptype;
        ParseFormat(dformat, normalized, datatype, comptype);
        return { datatype,comptype };
    }
    static ImageDataType ConvertFormat(const TextureDataFormat dformat) noexcept;
    static size_t ParseFormatSize(const TextureDataFormat dformat) noexcept;
    static bool IsCompressType(const TextureInnerFormat format) noexcept
    {
        if ((GLenum)format >= (GLenum)TextureInnerFormat::BC1 && (GLenum)format <= (GLenum)TextureInnerFormat::BC3)
            return true;
        if ((GLenum)format >= (GLenum)TextureInnerFormat::BC4 && (GLenum)format <= (GLenum)TextureInnerFormat::BC7)
            return true;
        return false;
    }
    static const char16_t* GetTypeName(const TextureType type);
};

class _oglTexture2DView;

class OGLUAPI _oglTexture2D : public _oglTexBase
{
    friend class _oglTexture2DArray;
    friend class ::oclu::detail::_oclGLBuffer;
protected:
    uint32_t Width, Height;

    explicit _oglTexture2D() noexcept : _oglTexBase(TextureType::Tex2D), Width(0), Height(0) {}

    void SetData(const bool isSub, const GLenum datatype, const GLenum comptype, const void *data) noexcept;
    void SetData(const bool isSub, const TextureDataFormat dformat, const void *data) noexcept;
    void SetData(const bool isSub, const TextureDataFormat dformat, const oglPBO& buf) noexcept;
    void SetData(const bool isSub, const Image& img, const bool normalized) noexcept;

    void SetCompressedData(const bool isSub, const void *data, const size_t size) noexcept;
    void SetCompressedData(const bool isSub, const oglPBO& buf, const size_t size) noexcept;
public:
    std::pair<uint32_t, uint32_t> GetSize() const { return { Width, Height }; }
    optional<vector<uint8_t>> GetCompressedData();
    vector<uint8_t> GetData(const TextureDataFormat dformat);
    Image GetImage(const TextureDataFormat dformat);
};


///<summary>Texture2D View, readonly</summary>  
class OGLUAPI _oglTexture2DView : public _oglTexture2D
{
    friend class _oglTexture2DArray;
    friend class _oglTexture2DStatic;
private:
    _oglTexture2DView(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat)
    {
        Width = width, Height = height; InnerFormat = iformat;
    }
};


class OGLUAPI _oglTexture2DStatic : public _oglTexture2D
{
public:
    _oglTexture2DStatic(const uint32_t width, const uint32_t height, const TextureInnerFormat iformat);

    void SetData(const TextureDataFormat dformat, const void *data);
    void SetData(const TextureDataFormat dformat, const oglPBO& buf);
    void SetData(const Image& img, const bool normalized = true);
    template<class T, class A>
    void SetData(const TextureDataFormat dformat, const vector<T, A>& data) 
    { 
        SetData(iformat, dformat, data.data());
    }
    
    void SetCompressedData(const void *data, const size_t size);
    void SetCompressedData(const oglPBO& buf, const size_t size);
    template<class T, class A>
    void SetCompressedData(const vector<T, A>& data) 
    { 
        SetCompressedData(data.data(), data.size() * sizeof(T));
    }

    Wrapper<_oglTexture2DView> GetTextureView() const;
};


class OGLUAPI _oglTexture2DDynamic : public _oglTexture2D
{
protected:
    void CheckAndSetMetadata(const TextureInnerFormat iformat, const uint32_t w, const uint32_t h);
public:
    _oglTexture2DDynamic() noexcept {}

    void SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const void *data);
    void SetData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const uint32_t w, const uint32_t h, const oglPBO& buf);
    void SetData(const TextureInnerFormat iformat, const Image& img, const bool normalized = true);
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
    uint32_t Width, Height, Layers;
public:
    _oglTexture2DArray(const uint32_t width, const uint32_t height, const uint32_t layers, const TextureInnerFormat iformat);
    _oglTexture2DArray(const Wrapper<_oglTexture2DArray>& old, const uint32_t layerAdd);
    
    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Layers }; }
    
    void SetTextureLayer(const uint32_t layer, const Wrapper<_oglTexture2D>& tex);
    void SetTextureLayer(const uint32_t layer, const Image& img);
    void SetTextureLayers(const uint32_t destLayer, const Wrapper<_oglTexture2DArray>& tex, const uint32_t srcLayer, const uint32_t layerCount);
    
    Wrapper<_oglTexture2DView> ViewTextureLayer(const uint32_t layer) const;
};


class OGLUAPI _oglBufferTexture : public _oglTexBase
{
protected:
    oglTBO InnerBuf;
public:
    _oglBufferTexture() noexcept;
    void SetBuffer(const TextureInnerFormat iformat, const oglTBO& tbo);
};


}

using oglTexBase = Wrapper<detail::_oglTexBase>;
using oglTex2D = Wrapper<detail::_oglTexture2D>;
using oglTex2DS = Wrapper<detail::_oglTexture2DStatic>;
using oglTex2DD = Wrapper<detail::_oglTexture2DDynamic>;
using oglTex2DV = Wrapper<detail::_oglTexture2DView>;
using oglTex2DArray = Wrapper<detail::_oglTexture2DArray>;
using oglBufTex = Wrapper<detail::_oglBufferTexture>;


}