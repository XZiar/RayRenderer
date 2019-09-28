#pragma once
#include "oglRely.h"
#include "oglException.h"
#include "ImageUtil/TexFormat.h"


namespace oglu
{


//[  flags|formats|channel|   bits]
//[15...13|12....8|7.....5|4.....0]
enum class TextureInnerFormat : uint16_t
{
    EMPTY = 0x0000, ERROR = 0xffff,
    BITS_MASK = 0x001f, CHANNEL_MASK = 0x00e0, FORMAT_MASK = 0x1f00, FLAG_MASK = 0xe000, CAT_MASK = FORMAT_MASK | BITS_MASK,
    //Bits
    BITS_2 = 0, BITS_4 = 2, BITS_5 = 3, BITS_8 = 4, BITS_9 = 5, BITS_10 = 6, BITS_11 = 7, BITS_12 = 8, BITS_16 = 9, BITS_32 = 10,
    //Channels[842]
    CHANNEL_R = 0x00, CHANNEL_RG = 0x20, CHANNEL_RA = CHANNEL_RG, CHANNEL_RGB = 0x40, CHANNEL_RGBA = 0x60, CHANNEL_ALPHA_MASK = 0x20,
    //Formats
    FORMAT_UNORM = 0x0000, FORMAT_SNORM = 0x0100, FORMAT_UINT  = 0x0200, FORMAT_SINT   = 0x0300, FORMAT_FLOAT = 0x0400,
    FORMAT_DXT1  = 0x0500, FORMAT_DXT3  = 0x0600, FORMAT_DXT5  = 0x0700, FORMAT_RGTC   = 0x0800, FORMAT_BPTC  = 0x0900,
    FORMAT_ETC   = 0x0a00, FORMAT_ETC2  = 0x0b00, FORMAT_PVRTC = 0x0c00, FORMAT_PVRTC2 = 0x0d00, FORMAT_ASTC  = 0x0e00,
    FORMAT_COMPRESS_PT = FORMAT_DXT1,
    //Flags
    FLAG_SRGB = 0x8000, FLAG_COMP = 0x4000,
    //Categories
    CAT_UNORM8  = BITS_8  | FORMAT_UNORM, CAT_U8   = BITS_8  | FORMAT_UINT , CAT_SNORM8  = BITS_8  | FORMAT_SNORM, CAT_S8  = BITS_8  | FORMAT_SINT,
    CAT_UNORM16 = BITS_16 | FORMAT_UNORM, CAT_U16  = BITS_16 | FORMAT_UINT , CAT_SNORM16 = BITS_16 | FORMAT_SNORM, CAT_S16 = BITS_16 | FORMAT_SINT,
    CAT_UNORM32 = BITS_32 | FORMAT_UNORM, CAT_U32  = BITS_32 | FORMAT_UINT , CAT_SNORM32 = BITS_32 | FORMAT_SNORM, CAT_S32 = BITS_32 | FORMAT_SINT,
    CAT_FLOAT   = BITS_32 | FORMAT_FLOAT, CAT_HALF = BITS_16 | FORMAT_FLOAT,
    //Actual Types
    //normalized integer->as float[0,1]
    R8  = CAT_UNORM8  | CHANNEL_R, RG8  = CAT_UNORM8  | CHANNEL_RG, RGB8  = CAT_UNORM8  | CHANNEL_RGB, RGBA8  = CAT_UNORM8  | CHANNEL_RGBA,
    R16 = CAT_UNORM16 | CHANNEL_R, RG16 = CAT_UNORM16 | CHANNEL_RG, RGB16 = CAT_UNORM16 | CHANNEL_RGB, RGBA16 = CAT_UNORM16 | CHANNEL_RGBA,
    //normalized integer->as float[-1,1]
    R8S  = CAT_SNORM8  | CHANNEL_R, RG8S  = CAT_SNORM8  | CHANNEL_RG, RGB8S  = CAT_SNORM8  | CHANNEL_RGB, RGBA8S  = CAT_SNORM8  | CHANNEL_RGBA,
    R16S = CAT_SNORM16 | CHANNEL_R, RG16S = CAT_SNORM16 | CHANNEL_RG, RGB16S = CAT_SNORM16 | CHANNEL_RGB, RGBA16S = CAT_SNORM16 | CHANNEL_RGBA,
    //non-normalized integer(unsigned)
    R8U  = CAT_U8  | CHANNEL_R, RG8U  = CAT_U8  | CHANNEL_RG, RGB8U  = CAT_U8  | CHANNEL_RGB, RGBA8U  = CAT_U8  | CHANNEL_RGBA,
    R16U = CAT_U16 | CHANNEL_R, RG16U = CAT_U16 | CHANNEL_RG, RGB16U = CAT_U16 | CHANNEL_RGB, RGBA16U = CAT_U16 | CHANNEL_RGBA,
    R32U = CAT_U32 | CHANNEL_R, RG32U = CAT_U32 | CHANNEL_RG, RGB32U = CAT_U32 | CHANNEL_RGB, RGBA32U = CAT_U32 | CHANNEL_RGBA,
    //non-normalized integer(signed)
    R8I  = CAT_S8  | CHANNEL_R, RG8I  = CAT_S8  | CHANNEL_RG, RGB8I  = CAT_S8  | CHANNEL_RGB, RGBA8I  = CAT_S8  | CHANNEL_RGBA,
    R16I = CAT_S16 | CHANNEL_R, RG16I = CAT_S16 | CHANNEL_RG, RGB16I = CAT_S16 | CHANNEL_RGB, RGBA16I = CAT_S16 | CHANNEL_RGBA,
    R32I = CAT_S32 | CHANNEL_R, RG32I = CAT_S32 | CHANNEL_RG, RGB32I = CAT_S32 | CHANNEL_RGB, RGBA32I = CAT_S32 | CHANNEL_RGBA,
    //half-float(FP16)
    Rh = CAT_HALF  | CHANNEL_R, RGh = CAT_HALF  | CHANNEL_RG, RGBh = CAT_HALF  | CHANNEL_RGB, RGBAh = CAT_HALF  | CHANNEL_RGBA,
    //float(FP32)
    Rf = CAT_FLOAT | CHANNEL_R, RGf = CAT_FLOAT | CHANNEL_RG, RGBf = CAT_FLOAT | CHANNEL_RGB, RGBAf = CAT_FLOAT | CHANNEL_RGBA,
    //special
    RG11B10  = BITS_11 | FORMAT_FLOAT | FLAG_COMP | CHANNEL_RGB,  RGB95    = BITS_9  | FORMAT_FLOAT | FLAG_COMP | CHANNEL_RGB,
    RGBA4444 = BITS_4  | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA, RGBA12   = BITS_12 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB,
    RGB332   = BITS_2  | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB,  RGB5A1   = BITS_5  | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA,
    RGB565   = BITS_5  | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGB,
    RGB10A2  = BITS_10 | FORMAT_UNORM | FLAG_COMP | CHANNEL_RGBA, RGB10A2U = BITS_10 | FORMAT_UINT  | FLAG_COMP | CHANNEL_RGBA,
    //compressed(S3TC/DXT135,RGTC,BPTC)
    BC1 = BITS_4 | FORMAT_DXT1 | CHANNEL_RGB, BC1A = BITS_4 | FORMAT_DXT1 | CHANNEL_RGBA, BC2  = BITS_8 | FORMAT_DXT3 | CHANNEL_RGBA, BC3 = BITS_8 | FORMAT_DXT5 | CHANNEL_RGBA,
    BC4 = BITS_4 | FORMAT_DXT5 | CHANNEL_R,   BC5  = BITS_8 | FORMAT_DXT5 | CHANNEL_RG,   BC6H = BITS_8 | FORMAT_BPTC | CHANNEL_RGB,  BC7 = BITS_8 | FORMAT_BPTC | CHANNEL_RGBA,
    //SRGB Types
    SRGB8   = RGB8 | FLAG_SRGB, SRGBA8   = RGBA8 | FLAG_SRGB,
    BC1SRGB = BC1  | FLAG_SRGB, BC1ASRGB = BC1A  | FLAG_SRGB, BC2SRGB = BC2 | FLAG_SRGB, BC3SRGB = BC3 | FLAG_SRGB, BC7SRGB = BC7 | FLAG_SRGB,
};
MAKE_ENUM_BITFIELD(TextureInnerFormat)


class OGLWrongFormatException : public OGLException
{
public:
    EXCEPTION_CLONE_EX(OGLWrongFormatException);
    std::variant<xziar::img::TextureDataFormat, TextureInnerFormat> Format;
    OGLWrongFormatException(const std::u16string_view& msg, const xziar::img::TextureDataFormat format, const std::any& data_ = std::any())
        : OGLException(TYPENAME, GLComponent::OGLU, msg, data_), Format(format)
    { }
    OGLWrongFormatException(const std::u16string_view& msg, const TextureInnerFormat format, const std::any& data_ = std::any())
        : OGLException(TYPENAME, GLComponent::OGLU, msg, data_), Format(format)
    { }
    virtual ~OGLWrongFormatException() {}
};


enum class TextureType : GLenum 
{ 
    Tex2D       = 0x0DE1/*GL_TEXTURE_2D*/,
    Tex3D       = 0x806F/*GL_TEXTURE_3D*/,
    TexBuf      = 0x8C2A/*GL_TEXTURE_BUFFER*/,
    Tex2DArray  = 0x8C1A/*GL_TEXTURE_2D_ARRAY*/
};


struct OGLUAPI TexFormatUtil
{
    constexpr static bool IsMonoColor(const TextureInnerFormat format) noexcept
    {
        return common::MatchAny(format & TextureInnerFormat::CHANNEL_MASK, TextureInnerFormat::CHANNEL_R, TextureInnerFormat::CHANNEL_RA);
    }
    constexpr static bool HasAlpha(const TextureInnerFormat format) noexcept
    {
        return common::MatchAny(format & TextureInnerFormat::CHANNEL_MASK, TextureInnerFormat::CHANNEL_RA, TextureInnerFormat::CHANNEL_RGBA);
    }
    constexpr static bool IsCompressType(const TextureInnerFormat format) noexcept
    {
        return common::enum_cast(format & TextureInnerFormat::FORMAT_MASK) >= common::enum_cast(TextureInnerFormat::FORMAT_COMPRESS_PT);
    }
    constexpr static bool IsSRGBType(const TextureInnerFormat format) noexcept
    { 
        return HAS_FIELD(format, TextureInnerFormat::FLAG_SRGB);
    }
    constexpr static TextureInnerFormat FromImageDType(const xziar::img::ImageDataType dtype, const bool normalized) noexcept
    {
        using xziar::img::ImageDataType;
        TextureInnerFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ? 
            TextureInnerFormat::CAT_FLOAT :
            (normalized ? TextureInnerFormat::CAT_UNORM8 : TextureInnerFormat::CAT_U8) | TextureInnerFormat::FLAG_SRGB;
        switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
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
    constexpr static TextureInnerFormat FromTexDType(const xziar::img::TextureDataFormat dformat) noexcept
    {
        using xziar::img::TextureDataFormat;
        const bool isInteger = HAS_FIELD(dformat, TextureDataFormat::DTYPE_INTEGER_MASK);
        if (HAS_FIELD(dformat, TextureDataFormat::DTYPE_COMP_MASK))
        {
            switch (REMOVE_MASK(dformat, TextureDataFormat::CHANNEL_REVERSE_MASK))
            {
            case TextureDataFormat::COMP_10_2:      return isInteger ? TextureInnerFormat::RGB10A2U : TextureInnerFormat::RGB10A2;
            case TextureDataFormat::COMP_5551:      return isInteger ? TextureInnerFormat::ERROR    : TextureInnerFormat::RGB5A1;
            case TextureDataFormat::COMP_565:       return isInteger ? TextureInnerFormat::ERROR    : TextureInnerFormat::RGB565;
            case TextureDataFormat::COMP_4444:      return isInteger ? TextureInnerFormat::ERROR    : TextureInnerFormat::RGBA4444;
            case TextureDataFormat::COMP_332:       return isInteger ? TextureInnerFormat::ERROR    : TextureInnerFormat::RGB332;
            default:                                return TextureInnerFormat::ERROR;
            }
        }
        else
        {
            TextureInnerFormat format = TextureInnerFormat::EMPTY;
            switch (dformat & TextureDataFormat::DTYPE_RAW_MASK)
            {
            case TextureDataFormat::DTYPE_U8:       format |= isInteger ? TextureInnerFormat::CAT_U8  : TextureInnerFormat::CAT_UNORM8;  break;
            case TextureDataFormat::DTYPE_I8:       format |= isInteger ? TextureInnerFormat::CAT_S8  : TextureInnerFormat::CAT_SNORM8;  break;
            case TextureDataFormat::DTYPE_U16:      format |= isInteger ? TextureInnerFormat::CAT_U16 : TextureInnerFormat::CAT_UNORM16; break;
            case TextureDataFormat::DTYPE_I16:      format |= isInteger ? TextureInnerFormat::CAT_S16 : TextureInnerFormat::CAT_SNORM16; break;
            case TextureDataFormat::DTYPE_U32:      format |= isInteger ? TextureInnerFormat::CAT_U32 : TextureInnerFormat::CAT_UNORM32; break;
            case TextureDataFormat::DTYPE_I32:      format |= isInteger ? TextureInnerFormat::CAT_S32 : TextureInnerFormat::CAT_SNORM32; break;
            case TextureDataFormat::DTYPE_HALF:     format |= isInteger ? TextureInnerFormat::ERROR   : TextureInnerFormat::CAT_HALF;    break;
            case TextureDataFormat::DTYPE_FLOAT:    format |= isInteger ? TextureInnerFormat::ERROR   : TextureInnerFormat::CAT_FLOAT;   break;
            default:                                return TextureInnerFormat::ERROR;
            }
            switch (dformat & TextureDataFormat::CHANNEL_MASK)
            {
            case TextureDataFormat::CHANNEL_R:      format |= TextureInnerFormat::CHANNEL_R;    break;
            case TextureDataFormat::CHANNEL_G:      format |= TextureInnerFormat::CHANNEL_R;    break;
            case TextureDataFormat::CHANNEL_B:      format |= TextureInnerFormat::CHANNEL_R;    break;
            case TextureDataFormat::CHANNEL_A:      format |= TextureInnerFormat::CHANNEL_R;    break;
            case TextureDataFormat::CHANNEL_RG:     format |= TextureInnerFormat::CHANNEL_RA;   break;
            case TextureDataFormat::CHANNEL_RGB:    format |= TextureInnerFormat::CHANNEL_RGB;  break;
            case TextureDataFormat::CHANNEL_BGR:    format |= TextureInnerFormat::CHANNEL_RGB;  break;
            case TextureDataFormat::CHANNEL_RGBA:   format |= TextureInnerFormat::CHANNEL_RGBA; break;
            case TextureDataFormat::CHANNEL_BGRA:   format |= TextureInnerFormat::CHANNEL_RGBA; break;
            default:                                return TextureInnerFormat::ERROR;
            }
            return format;
        }
    }
    constexpr static uint8_t UnitSize(const TextureInnerFormat format) noexcept
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
            case TextureInnerFormat::RGBA4444:      return 2;
            case TextureInnerFormat::RGB95:         return 4;
            default:                                return 0;
            }
        }
        if (IsCompressType(format))
            return 0; // should not use this
        uint8_t size = 0;
        switch (format & TextureInnerFormat::BITS_MASK)
        {
        case TextureInnerFormat::BITS_8:        size = 1; break;
        case TextureInnerFormat::BITS_16:       size = 2; break;
        case TextureInnerFormat::BITS_32:       size = 4; break;
        default:                                return 0;
        }
        switch (format & TextureInnerFormat::CHANNEL_MASK)
        {
        case TextureInnerFormat::CHANNEL_R:     return size * 1;
        case TextureInnerFormat::CHANNEL_RG:    return size * 2;
        case TextureInnerFormat::CHANNEL_RGB:   return size * 3;
        case TextureInnerFormat::CHANNEL_RGBA:  return size * 4;
        default:                                return 0;
        }
    }
    constexpr static xziar::img::ImageDataType ToImageDType(const TextureInnerFormat format, const bool relaxConvert = false) noexcept
    {
        using xziar::img::ImageDataType;
        if (!HAS_FIELD(format, TextureInnerFormat::FLAG_COMP))
        {
            ImageDataType dtype = ImageDataType::UNKNOWN_RESERVE;
            switch (format & TextureInnerFormat::CHANNEL_MASK)
            {
            case TextureInnerFormat::CHANNEL_R:     dtype = ImageDataType::RED;  break;
            case TextureInnerFormat::CHANNEL_RG:    dtype = ImageDataType::RA;   break;
            case TextureInnerFormat::CHANNEL_RGB:   dtype = ImageDataType::RGB;  break;
            case TextureInnerFormat::CHANNEL_RGBA:  dtype = ImageDataType::RGBA; break;
            default:                                return ImageDataType::UNKNOWN_RESERVE;
            }
            switch (format & TextureInnerFormat::CAT_MASK)
            {
            case TextureInnerFormat::CAT_SNORM8:
            case TextureInnerFormat::CAT_U8:
            case TextureInnerFormat::CAT_S8:        
                if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE;
                //only pass through when relaxConvert
                //[[fallthrough]]
            case TextureInnerFormat::CAT_UNORM8:    return dtype;
            case TextureInnerFormat::CAT_FLOAT:     return dtype | ImageDataType::FLOAT_MASK;
            default:                                return ImageDataType::UNKNOWN_RESERVE;
            }
        }
        return ImageDataType::UNKNOWN_RESERVE;
    }


    static GLenum GetInnerFormat(const TextureInnerFormat format) noexcept;
    static void ParseFormat(const xziar::img::TextureDataFormat dformat, const bool isUpload, GLenum& datatype, GLenum& comptype) noexcept;
    static std::pair<GLenum, GLenum> ParseFormat(const xziar::img::TextureDataFormat dformat, const bool isUpload) noexcept
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
    static xziar::img::TextureDataFormat ToDType(const TextureInnerFormat format);
    
    static u16string_view GetTypeName(const TextureType type) noexcept;
    static u16string_view GetFormatName(const TextureInnerFormat format) noexcept;
    static string GetFormatDetail(const TextureInnerFormat format) noexcept;
};


}
