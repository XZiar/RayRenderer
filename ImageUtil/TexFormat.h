#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"


namespace xziar::img
{


//Tex data type collection --- focusing on uncompressed image type, mainly on gpu side.
//[unused|sRGB|channel-order|channels|type categroy|data type]
//[------|*14*|13.........12|11.....8|7...........6|5.......0]
enum class TextureFormat : uint16_t
{
    EMPTY = 0x0000, ERROR = 0xffff,
    
    //format(channel)
    MASK_CHANNEL = 0x3f00, MASK_CHANNEL_ORDER = 0x3000, MASK_CHANNEL_RAW = 0x0f00,
    CHANNEL_ORDER_NORMAL = 0x0000, CHANNEL_ORDER_REV_RGB = 0x1000, CHANNEL_ORDER_REV_ENDIAN = 0x2000,
    
    CHANNEL_R    = 0x0100, CHANNEL_G    = 0x0200, CHANNEL_B    = 0x0400, CHANNEL_A    = 0x0800,
    CHANNEL_RG   = 0x0900, CHANNEL_RA   = 0x0900, CHANNEL_RGB  = 0x0700, CHANNEL_RGBA = 0x0f00,
    CHANNEL_BGR  = CHANNEL_RGB | CHANNEL_ORDER_REV_RGB, CHANNEL_BGRA = CHANNEL_RGBA | CHANNEL_ORDER_REV_RGB, CHANNEL_ABGR = CHANNEL_RGBA | CHANNEL_ORDER_REV_ENDIAN,
    
    //data type
    MASK_DTYPE = 0x00ff, MASK_DTYPE_CAT = 0x00c0, MASK_DTYPE_RAW = 0x003f,
    
    //plain type category
    DTYPE_CAT_PLAIN = 0x0000, MASK_PLAIN_NORMALIZE = 0x0020, MASK_PLAIN_RAW = 0x001f, DTYPE_PLAIN_NORMALIZE = 0x0000, DTYPE_PLAIN_RAW = 0x0020,

    DTYPE_U8 = 0x0, DTYPE_I8 = 0x1, DTYPE_U16 = 0x2, DTYPE_I16 = 0x3, DTYPE_U32 = 0x4, DTYPE_I32 = 0x5, 
    DTYPE_HALF = DTYPE_PLAIN_RAW | 0x6, DTYPE_FLOAT = DTYPE_PLAIN_RAW | 0x7,

    //composite type category
    DTYPE_CAT_COMP = 0x0040, MASK_COMP_NORMALIZE = 0x0020, MASK_COMP_RAW = 0x001f, DTYPE_COMP_NORMALIZE = 0x0000, DTYPE_COMP_RAW = 0x0020,

    COMP_332   = DTYPE_CAT_COMP | 0x0, COMP_233R   = CHANNEL_ORDER_REV_RGB | COMP_332  ,
    COMP_565   = DTYPE_CAT_COMP | 0x1, COMP_565R   = CHANNEL_ORDER_REV_RGB | COMP_565  ,
    COMP_4444  = DTYPE_CAT_COMP | 0x2, COMP_4444R  = CHANNEL_ORDER_REV_RGB | COMP_4444 ,
    COMP_5551  = DTYPE_CAT_COMP | 0x3, COMP_1555R  = CHANNEL_ORDER_REV_RGB | COMP_5551 ,
    COMP_10_2  = DTYPE_CAT_COMP | 0x4, COMP_10_2R  = CHANNEL_ORDER_REV_RGB | COMP_10_2 ,
    COMP_11_10 = DTYPE_CAT_COMP | 0x5, COMP_11_10R = CHANNEL_ORDER_REV_RGB | COMP_11_10,
    COMP_999_5 = DTYPE_CAT_COMP | 0x6, COMP_999_5R = CHANNEL_ORDER_REV_RGB | COMP_999_5,

    //compressed type category
    DTYPE_CAT_COMPRESS = 0x0080, MASK_COMPRESS_SIGNESS = 0x0020, MASK_COMPRESS_RAW = 0x001f, DTYPE_COMPRESS_UNSIGNED = 0x0000, DTYPE_COMPRESS_SIGNED = 0x0020,
    
    CPRS_DXT1   = DTYPE_CAT_COMPRESS | 0x0, CPRS_DXT3 = DTYPE_CAT_COMPRESS | 0x1, CPRS_DXT5  = DTYPE_CAT_COMPRESS | 0x2,
    CPRS_RGTC   = DTYPE_CAT_COMPRESS | 0x3, CPRS_BPTC = DTYPE_CAT_COMPRESS | 0x4, CPRS_BPTCf = DTYPE_CAT_COMPRESS | 0x5,
    CPRS_ETC    = DTYPE_CAT_COMPRESS | 0x6, CPRS_ETC2 = DTYPE_CAT_COMPRESS | 0x7, CPRS_PVRTC = DTYPE_CAT_COMPRESS | 0x8, 
    CPRS_PVRTC2 = DTYPE_CAT_COMPRESS | 0x9, CPRS_ASTC = DTYPE_CAT_COMPRESS | 0xa,

    //normalized integer[0,1]
    R8   = CHANNEL_R | DTYPE_U8 , RG8   = CHANNEL_RG | DTYPE_U8 , RGB8   = CHANNEL_RGB | DTYPE_U8 , BGR8   = CHANNEL_BGR | DTYPE_U8 , RGBA8   = CHANNEL_RGBA | DTYPE_U8 , BGRA8   = CHANNEL_BGRA | DTYPE_U8 ,
    R16  = CHANNEL_R | DTYPE_U16, RG16  = CHANNEL_RG | DTYPE_U16, RGB16  = CHANNEL_RGB | DTYPE_U16, BGR16  = CHANNEL_BGR | DTYPE_U16, RGBA16  = CHANNEL_RGBA | DTYPE_U16, BGRA16  = CHANNEL_BGRA | DTYPE_U16,
    R32  = CHANNEL_R | DTYPE_U32, RG32  = CHANNEL_RG | DTYPE_U32, RGB32  = CHANNEL_RGB | DTYPE_U32, BGR32  = CHANNEL_BGR | DTYPE_U32, RGBA32  = CHANNEL_RGBA | DTYPE_U32, BGRA32  = CHANNEL_BGRA | DTYPE_U32,

    //normalized integer[-1,1]
    R8S  = CHANNEL_R | DTYPE_I8 , RG8S  = CHANNEL_RG | DTYPE_I8 , RGB8S  = CHANNEL_RGB | DTYPE_I8 , BGR8S  = CHANNEL_BGR | DTYPE_I8 , RGBA8S  = CHANNEL_RGBA | DTYPE_I8 , BGRA8S  = CHANNEL_BGRA | DTYPE_I8,
    R16S = CHANNEL_R | DTYPE_I16, RG16S = CHANNEL_RG | DTYPE_I16, RGB16S = CHANNEL_RGB | DTYPE_I16, BGR16S = CHANNEL_BGR | DTYPE_I16, RGBA16S = CHANNEL_RGBA | DTYPE_I16, BGRA16S = CHANNEL_BGRA | DTYPE_I16,
    R32S = CHANNEL_R | DTYPE_I32, RG32S = CHANNEL_RG | DTYPE_I32, RGB32S = CHANNEL_RGB | DTYPE_I32, BGR32S = CHANNEL_BGR | DTYPE_I32, RGBA32S = CHANNEL_RGBA | DTYPE_I32, BGRA32S = CHANNEL_BGRA | DTYPE_I32,

    //non-normalized integer(unsigned)
    R8U  = R8  | DTYPE_PLAIN_RAW, RG8U  = RG8  | DTYPE_PLAIN_RAW, RGB8U  = RGB8  | DTYPE_PLAIN_RAW, BGR8U  = BGR8  | DTYPE_PLAIN_RAW, RGBA8U  = RGBA8  | DTYPE_PLAIN_RAW, BGRA8U  = BGRA8  | DTYPE_PLAIN_RAW,
    R16U = R16 | DTYPE_PLAIN_RAW, RG16U = RG16 | DTYPE_PLAIN_RAW, RGB16U = RGB16 | DTYPE_PLAIN_RAW, BGR16U = BGR16 | DTYPE_PLAIN_RAW, RGBA16U = RGBA16 | DTYPE_PLAIN_RAW, BGRA16U = BGRA16 | DTYPE_PLAIN_RAW,
    R32U = R32 | DTYPE_PLAIN_RAW, RG32U = RG32 | DTYPE_PLAIN_RAW, RGB32U = RGB32 | DTYPE_PLAIN_RAW, BGR32U = BGR32 | DTYPE_PLAIN_RAW, RGBA32U = RGBA32 | DTYPE_PLAIN_RAW, BGRA32U = BGRA32 | DTYPE_PLAIN_RAW,

    //non-normalized integer(signed)
    R8I  = R8S  | DTYPE_PLAIN_RAW, RG8I  = RG8S  | DTYPE_PLAIN_RAW, RGB8I  = RGB8S  | DTYPE_PLAIN_RAW, BGR8I  = BGR8S  | DTYPE_PLAIN_RAW, RGBA8I  = RGBA8S  | DTYPE_PLAIN_RAW, BGRA8I  = BGRA8S  | DTYPE_PLAIN_RAW,
    R16I = R16S | DTYPE_PLAIN_RAW, RG16I = RG16S | DTYPE_PLAIN_RAW, RGB16I = RGB16S | DTYPE_PLAIN_RAW, BGR16I = BGR16S | DTYPE_PLAIN_RAW, RGBA16I = RGBA16S | DTYPE_PLAIN_RAW, BGRA16I = BGRA16S | DTYPE_PLAIN_RAW,
    R32I = R32S | DTYPE_PLAIN_RAW, RG32I = RG32S | DTYPE_PLAIN_RAW, RGB32I = RGB32S | DTYPE_PLAIN_RAW, BGR32I = BGR32S | DTYPE_PLAIN_RAW, RGBA32I = RGBA32S | DTYPE_PLAIN_RAW, BGRA32I = BGRA32S | DTYPE_PLAIN_RAW,

    //half-float(FP16)
    Rh = CHANNEL_R | DTYPE_HALF , RGh = CHANNEL_RG | DTYPE_HALF , RGBh = CHANNEL_RGB | DTYPE_HALF , BGRh = CHANNEL_BGR | DTYPE_HALF , RGBAh = CHANNEL_RGBA | DTYPE_HALF , BGRAh = CHANNEL_BGRA | DTYPE_HALF ,

    //float(FP32)
    Rf = CHANNEL_R | DTYPE_FLOAT, RGf = CHANNEL_RG | DTYPE_FLOAT, RGBf = CHANNEL_RGB | DTYPE_FLOAT, BGRf = CHANNEL_BGR | DTYPE_FLOAT, RGBAf = CHANNEL_RGBA | DTYPE_FLOAT, BGRAf = CHANNEL_BGRA | DTYPE_FLOAT,

    //composite
    RGB332  = COMP_332   | CHANNEL_RGB , RGB565   = COMP_565    | CHANNEL_RGB , RGBA4444 = COMP_4444   | CHANNEL_RGBA,
    RGB5A1  = COMP_5551  | CHANNEL_RGBA, RGB95    = COMP_999_5  | CHANNEL_RGB , RG11B10  = COMP_11_10  | CHANNEL_RGB ,
    RGB10A2 = COMP_10_2  | CHANNEL_RGBA, RGB10A2U = RGB10A2     | DTYPE_COMP_RAW,
    BGR233  = COMP_233R  | CHANNEL_BGR , BGR565   = COMP_565R   | CHANNEL_BGR, BGRA4444  = COMP_4444R  | CHANNEL_BGRA,
    BGR5A1  = COMP_1555R | CHANNEL_BGRA, BGR95    = COMP_999_5R | CHANNEL_BGR, BG11R10   = COMP_11_10R | CHANNEL_BGR,
    BGR10A2 = COMP_10_2R | CHANNEL_BGRA, BGR10A2U = BGR10A2     | DTYPE_COMP_RAW,

    //compressed(S3TC/DXT135,RGTC,BPTC)
    BC1  = CPRS_DXT1  | CHANNEL_RGB , BC1A = CPRS_DXT1 | CHANNEL_RGBA, 
    BC2  = CPRS_DXT3  | CHANNEL_RGBA, BC3  = CPRS_DXT5 | CHANNEL_RGBA,
    BC4  = CPRS_DXT5  | CHANNEL_R   , BC5  = CPRS_DXT5 | CHANNEL_RG  , 
    BC6H = CPRS_BPTCf | CHANNEL_RGB , BC7  = CPRS_BPTC | CHANNEL_RGBA,
    BC6Hs = BC6H | DTYPE_COMPRESS_SIGNED,

    //sRGB
    MASK_SRGB = 0x4000,
    SRGB8   = RGB8 | MASK_SRGB, SRGBA8   = RGBA8 | MASK_SRGB,
    BC1SRGB = BC1  | MASK_SRGB, BC1ASRGB = BC1A  | MASK_SRGB, BC2SRGB = BC2 | MASK_SRGB, 
    BC3SRGB = BC3  | MASK_SRGB, BC7SRGB  = BC7   | MASK_SRGB,

    //dummy
    DUMMY_I8INT = DTYPE_I8 | DTYPE_PLAIN_RAW,
};
MAKE_ENUM_BITFIELD(TextureFormat)


struct TexFormatUtil
{
    [[nodiscard]] constexpr static bool IsMonoColor(const TextureFormat format) noexcept
    {
        return HAS_FIELD(format, TextureFormat::CHANNEL_A);
    }
    [[nodiscard]] constexpr static bool IsPlainType(const TextureFormat format) noexcept
    {
        return (format & TextureFormat::MASK_DTYPE_CAT) == TextureFormat::DTYPE_CAT_PLAIN;
    }
    [[nodiscard]] constexpr static bool IsCompositeType(const TextureFormat format) noexcept
    {
        return (format & TextureFormat::MASK_DTYPE_CAT) == TextureFormat::DTYPE_CAT_COMP;
    }
    [[nodiscard]] constexpr static bool IsCompressType(const TextureFormat format) noexcept
    {
        return (format & TextureFormat::MASK_DTYPE_CAT) == TextureFormat::DTYPE_CAT_COMPRESS;
    }
    [[nodiscard]] constexpr static bool IsSRGBType(const TextureFormat format) noexcept
    {
        return HAS_FIELD(format, TextureFormat::MASK_SRGB);
    }
    [[nodiscard]] constexpr static bool IsNormalized(const TextureFormat format) noexcept
    {
        const auto category = format & TextureFormat::MASK_DTYPE_CAT;
        const auto dtype = format & TextureFormat::MASK_DTYPE_RAW;
        switch (category)
        {
        case TextureFormat::DTYPE_CAT_COMPRESS:
            return REMOVE_MASK(dtype, TextureFormat::MASK_COMPRESS_SIGNESS) != TextureFormat::CPRS_BPTCf;
        case TextureFormat::DTYPE_CAT_COMP:
            return (dtype & TextureFormat::MASK_COMP_NORMALIZE) == TextureFormat::DTYPE_COMP_NORMALIZE;
        case TextureFormat::DTYPE_CAT_PLAIN:
            return (dtype & TextureFormat::MASK_PLAIN_NORMALIZE) == TextureFormat::DTYPE_PLAIN_NORMALIZE;
        default:
            return true;
        }
    }
    [[nodiscard]] constexpr static bool HasAlpha(const TextureFormat format) noexcept
    {
        return HAS_FIELD(format, TextureFormat::CHANNEL_A);
    }
    [[nodiscard]] constexpr static TextureFormat FromImageDType(const ImageDataType dtype, const bool normalized = true) noexcept
    {
        TextureFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ?
            TextureFormat::DTYPE_FLOAT :
            TextureFormat::DTYPE_U8 |
                (normalized ? TextureFormat::DTYPE_PLAIN_NORMALIZE : TextureFormat::DTYPE_PLAIN_RAW);
        baseFormat |= TextureFormat::DTYPE_CAT_PLAIN | TextureFormat::MASK_SRGB;
        switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
        {
        case ImageDataType::RGB:    return baseFormat | TextureFormat::CHANNEL_RGB;
        case ImageDataType::BGR:    return baseFormat | TextureFormat::CHANNEL_BGR;
        case ImageDataType::RGBA:   return baseFormat | TextureFormat::CHANNEL_RGBA;
        case ImageDataType::BGRA:   return baseFormat | TextureFormat::CHANNEL_BGRA;
        case ImageDataType::GRAY:   return baseFormat | TextureFormat::CHANNEL_R;
        case ImageDataType::GA:     return baseFormat | TextureFormat::CHANNEL_RG;
        default:                    return TextureFormat::ERROR;
        }
    }
    [[nodiscard]] constexpr static uint8_t BitPerPixel(const TextureFormat dformat) noexcept
    {
        const auto category = dformat & TextureFormat::MASK_DTYPE_CAT;
        switch (category)
        {
        case TextureFormat::DTYPE_CAT_COMPRESS:
            {
                switch (REMOVE_MASK(dformat, TextureFormat::MASK_SRGB))
                {
                case TextureFormat::BC1:
                case TextureFormat::BC1A:
                case TextureFormat::BC4:
                    return 4;
                case TextureFormat::BC2:
                case TextureFormat::BC3:
                case TextureFormat::BC5:
                case TextureFormat::BC6H:
                case TextureFormat::BC7:
                    return 8;
                default:
                    return 0;
                }
            }
        case TextureFormat::DTYPE_CAT_COMP:
            {
                switch (dformat & TextureFormat::MASK_DTYPE)
                {
                case TextureFormat::COMP_332:
                    return 8;
                case TextureFormat::COMP_4444:
                case TextureFormat::COMP_565:
                case TextureFormat::COMP_5551:
                    return 16;
                case TextureFormat::COMP_10_2:
                case TextureFormat::COMP_11_10:
                case TextureFormat::COMP_999_5:
                    return 32;
                default:
                    return 0;
                }
            }
        case TextureFormat::DTYPE_CAT_PLAIN:
            {
                uint8_t size = 0;
                switch (dformat & TextureFormat::MASK_DTYPE_RAW)
                {
                case TextureFormat::DTYPE_U8:
                case TextureFormat::DTYPE_I8:
                    size = 8; break;
                case TextureFormat::DTYPE_U16:
                case TextureFormat::DTYPE_I16:
                case TextureFormat::DTYPE_HALF:
                    size = 16; break;
                case TextureFormat::DTYPE_U32:
                case TextureFormat::DTYPE_I32:
                case TextureFormat::DTYPE_FLOAT:
                    size = 32; break;
                default:
                    return 0;
                }
                switch (dformat & TextureFormat::MASK_CHANNEL_RAW)
                {
                case TextureFormat::CHANNEL_R:
                case TextureFormat::CHANNEL_G:
                case TextureFormat::CHANNEL_B:
                case TextureFormat::CHANNEL_A:
                    return size * 1;
                case TextureFormat::CHANNEL_RG:
                    return size * 2;
                case TextureFormat::CHANNEL_RGB:
                    return size * 3;
                case TextureFormat::CHANNEL_RGBA:
                    return size * 4;
                default:
                    return 0;
                }
            }
        default:
            return 0;
        }
    }
    [[nodiscard]] constexpr static ImageDataType ToImageDType(const TextureFormat format, const bool relaxConvert = false) noexcept
    {
        if ((format & TextureFormat::MASK_DTYPE_CAT) != TextureFormat::DTYPE_CAT_PLAIN)
            return ImageDataType::UNKNOWN_RESERVE;
        ImageDataType floatType = ImageDataType::EMPTY_MASK;
        switch (format & TextureFormat::MASK_DTYPE)
        {
        case TextureFormat::DTYPE_I8 | TextureFormat::DTYPE_PLAIN_RAW:
        case TextureFormat::DTYPE_U8 | TextureFormat::DTYPE_PLAIN_RAW:
        case TextureFormat::DTYPE_I8:
            if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE; 
            [[fallthrough]];
        case TextureFormat::DTYPE_U8:       floatType = ImageDataType::EMPTY_MASK; break;
        case TextureFormat::DTYPE_FLOAT:    floatType = ImageDataType::FLOAT_MASK; break;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
        switch (format & TextureFormat::MASK_CHANNEL)
        {
        case TextureFormat::CHANNEL_G:
        case TextureFormat::CHANNEL_B:
            if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE;
            [[fallthrough]];
        case TextureFormat::CHANNEL_R:      return ImageDataType::GRAY | floatType;
        case TextureFormat::CHANNEL_RG:     return ImageDataType::RA   | floatType;
        case TextureFormat::CHANNEL_RGB:    return ImageDataType::RGB  | floatType;
        case TextureFormat::CHANNEL_RGBA:   return ImageDataType::RGBA | floatType;
        case TextureFormat::CHANNEL_BGR:    return ImageDataType::BGR  | floatType;
        case TextureFormat::CHANNEL_BGRA:   return ImageDataType::BGRA | floatType;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
    }
    [[nodiscard]] IMGUTILAPI static std::string GetFormatDetail(const TextureFormat format) noexcept;
    [[nodiscard]] IMGUTILAPI static std::u16string_view GetFormatName(const TextureFormat format) noexcept;

};

}