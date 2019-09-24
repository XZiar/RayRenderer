#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"


namespace xziar::img
{


//Tex data type collection --- focusing on uncompressed image type, mainly on gpu side.
//[unused|channel-reverse|channels|normalized|composite|data type]
//[------|******12*******|11.....8|****7*****|****6****|5.......0]
enum class TextureDataFormat : uint16_t
{
    EMPTY = 0x0000, ERROR = 0xffff,
    
    //format(channel)
    CHANNEL_MASK = 0x1f00, CHANNEL_REVERSE_MASK = 0x1000, CHANNEL_RAW_MASK = 0x0f00,
    
    CHANNEL_R    = 0x0100, CHANNEL_G    = 0x0200, CHANNEL_B    = 0x0400, CHANNEL_A    = 0x0800,
    CHANNEL_RG   = 0x0900, CHANNEL_RA   = 0x0900, CHANNEL_RGB  = 0x0700, CHANNEL_RGBA = 0x0f00,
    CHANNEL_BGR  = CHANNEL_RGB | CHANNEL_REVERSE_MASK, CHANNEL_BGRA = CHANNEL_RGBA | CHANNEL_REVERSE_MASK,
    
    //data type
    DTYPE_MASK = 0x00ff, DTYPE_INTEGER_MASK = 0x0080, DTYPE_COMP_MASK = 0x0040, DTYPE_RAW_MASK = 0x003f,
    
    DTYPE_U8 = 0x0, DTYPE_I8 = 0x1, DTYPE_U16 = 0x2, DTYPE_I16 = 0x3, DTYPE_U32 = 0x4, DTYPE_I32 = 0x5, DTYPE_HALF = 0x6, DTYPE_FLOAT = 0x7,

    //composed type
    COMP_332   = DTYPE_COMP_MASK | 0x0 | CHANNEL_RGB , COMP_233R   = CHANNEL_REVERSE_MASK | COMP_332,
    COMP_565   = DTYPE_COMP_MASK | 0x1 | CHANNEL_RGB , COMP_565R   = CHANNEL_REVERSE_MASK | COMP_565,
    COMP_4444  = DTYPE_COMP_MASK | 0x2 | CHANNEL_RGBA, COMP_4444R  = CHANNEL_REVERSE_MASK | COMP_4444,
    COMP_5551  = DTYPE_COMP_MASK | 0x3 | CHANNEL_RGBA, COMP_1555R  = CHANNEL_REVERSE_MASK | COMP_5551,
    COMP_8888  = DTYPE_COMP_MASK | 0x4 | CHANNEL_RGBA, COMP_8888R  = CHANNEL_REVERSE_MASK | COMP_8888,
    COMP_10_2  = DTYPE_COMP_MASK | 0x5 | CHANNEL_RGBA, COMP_10_2R  = CHANNEL_REVERSE_MASK | COMP_10_2,
    COMP_11_10 = DTYPE_COMP_MASK | 0x6 | CHANNEL_RGB , COMP_11_10R = CHANNEL_REVERSE_MASK | COMP_11_10,
    COMP_999_5 = DTYPE_COMP_MASK | 0x7 | CHANNEL_RGB , COMP_999_5R = CHANNEL_REVERSE_MASK | COMP_999_5,

    //normalized integer[0,1]
    R8   = CHANNEL_R | DTYPE_U8 , RG8   = CHANNEL_RG | DTYPE_U8 , RGB8   = CHANNEL_RGB | DTYPE_U8 , BGR8   = CHANNEL_BGR | DTYPE_U8 , RGBA8   = CHANNEL_RGBA | DTYPE_U8 , BGRA8   = CHANNEL_BGRA | DTYPE_U8 ,
    R16  = CHANNEL_R | DTYPE_U16, RG16  = CHANNEL_RG | DTYPE_U16, RGB16  = CHANNEL_RGB | DTYPE_U16, BGR16  = CHANNEL_BGR | DTYPE_U16, RGBA16  = CHANNEL_RGBA | DTYPE_U16, BGRA16  = CHANNEL_BGRA | DTYPE_U16,
    R32  = CHANNEL_R | DTYPE_U32, RG32  = CHANNEL_RG | DTYPE_U32, RGB32  = CHANNEL_RGB | DTYPE_U32, BGR32  = CHANNEL_BGR | DTYPE_U32, RGBA32  = CHANNEL_RGBA | DTYPE_U32, BGRA32  = CHANNEL_BGRA | DTYPE_U32,

    //normalized integer[-1,1]
    R8S  = CHANNEL_R | DTYPE_I8 , RG8S  = CHANNEL_RG | DTYPE_I8 , RGB8S  = CHANNEL_RGB | DTYPE_I8 , BGR8S  = CHANNEL_BGR | DTYPE_I8 , RGBA8S  = CHANNEL_RGBA | DTYPE_I8 , BGRA8S  = CHANNEL_BGRA | DTYPE_I8,
    R16S = CHANNEL_R | DTYPE_I16, RG16S = CHANNEL_RG | DTYPE_I16, RGB16S = CHANNEL_RGB | DTYPE_I16, BGR16S = CHANNEL_BGR | DTYPE_I16, RGBA16S = CHANNEL_RGBA | DTYPE_I16, BGRA16S = CHANNEL_BGRA | DTYPE_I16,
    R32S = CHANNEL_R | DTYPE_I32, RG32S = CHANNEL_RG | DTYPE_I32, RGB32S = CHANNEL_RGB | DTYPE_I32, BGR32S = CHANNEL_BGR | DTYPE_I32, RGBA32S = CHANNEL_RGBA | DTYPE_I32, BGRA32S = CHANNEL_BGRA | DTYPE_I32,

    //non-normalized integer(unsigned)
    R8U  = R8  | DTYPE_INTEGER_MASK, RG8U  = RG8  | DTYPE_INTEGER_MASK, RGB8U  = RGB8  | DTYPE_INTEGER_MASK, BGR8U  = BGR8  | DTYPE_INTEGER_MASK, RGBA8U  = RGBA8  | DTYPE_INTEGER_MASK, BGRA8U  = BGRA8  | DTYPE_INTEGER_MASK,
    R16U = R16 | DTYPE_INTEGER_MASK, RG16U = RG16 | DTYPE_INTEGER_MASK, RGB16U = RGB16 | DTYPE_INTEGER_MASK, BGR16U = BGR16 | DTYPE_INTEGER_MASK, RGBA16U = RGBA16 | DTYPE_INTEGER_MASK, BGRA16U = BGRA16 | DTYPE_INTEGER_MASK,
    R32U = R32 | DTYPE_INTEGER_MASK, RG32U = RG32 | DTYPE_INTEGER_MASK, RGB32U = RGB32 | DTYPE_INTEGER_MASK, BGR32U = BGR32 | DTYPE_INTEGER_MASK, RGBA32U = RGBA32 | DTYPE_INTEGER_MASK, BGRA32U = BGRA32 | DTYPE_INTEGER_MASK,

    //non-normalized integer(signed)
    R8I  = R8S  | DTYPE_INTEGER_MASK, RG8I  = RG8S  | DTYPE_INTEGER_MASK, RGB8I  = RGB8S  | DTYPE_INTEGER_MASK, BGR8I  = BGR8S  | DTYPE_INTEGER_MASK, RGBA8I  = RGBA8S  | DTYPE_INTEGER_MASK, BGRA8I  = BGRA8S  | DTYPE_INTEGER_MASK,
    R16I = R16S | DTYPE_INTEGER_MASK, RG16I = RG16S | DTYPE_INTEGER_MASK, RGB16I = RGB16S | DTYPE_INTEGER_MASK, BGR16I = BGR16S | DTYPE_INTEGER_MASK, RGBA16I = RGBA16S | DTYPE_INTEGER_MASK, BGRA16I = BGRA16S | DTYPE_INTEGER_MASK,
    R32I = R32S | DTYPE_INTEGER_MASK, RG32I = RG32S | DTYPE_INTEGER_MASK, RGB32I = RGB32S | DTYPE_INTEGER_MASK, BGR32I = BGR32S | DTYPE_INTEGER_MASK, RGBA32I = RGBA32S | DTYPE_INTEGER_MASK, BGRA32I = BGRA32S | DTYPE_INTEGER_MASK,

    //half-float(FP16)
    Rh = CHANNEL_R | DTYPE_HALF , RGh = CHANNEL_RG | DTYPE_HALF , RGBh = CHANNEL_RGB | DTYPE_HALF , BGRh = CHANNEL_BGR | DTYPE_HALF , RGBAh = CHANNEL_RGBA | DTYPE_HALF , BGRAh = CHANNEL_BGRA | DTYPE_HALF ,

    //float(FP32)
    Rf = CHANNEL_R | DTYPE_FLOAT, RGf = CHANNEL_RG | DTYPE_FLOAT, RGBf = CHANNEL_RGB | DTYPE_FLOAT, BGRf = CHANNEL_BGR | DTYPE_FLOAT, RGBAf = CHANNEL_RGBA | DTYPE_FLOAT, BGRAf = CHANNEL_BGRA | DTYPE_FLOAT,

    //composite
    RGB10A2 = COMP_10_2, RGB10A2U = RGB10A2 | DTYPE_INTEGER_MASK,

    //dummy
    _DUMMY_I8 = DTYPE_I8 | DTYPE_INTEGER_MASK,
};
MAKE_ENUM_BITFIELD(TextureDataFormat)


struct TexDFormatUtil
{
    constexpr static bool IsMonoColor(const TextureDataFormat format) noexcept
    {
        const auto channel = format & TextureDataFormat::CHANNEL_MASK;
        return channel == TextureDataFormat::CHANNEL_R || channel == TextureDataFormat::CHANNEL_RA;
    }
    constexpr static bool HasAlpha(const TextureDataFormat format) noexcept
    {
        return HAS_FIELD(format, TextureDataFormat::CHANNEL_A);
    }
    constexpr static TextureDataFormat FromImageDType(const ImageDataType dtype, const bool normalized) noexcept
    {
        TextureDataFormat baseFormat = HAS_FIELD(dtype, ImageDataType::FLOAT_MASK) ? 
            TextureDataFormat::DTYPE_FLOAT :
            (normalized ? TextureDataFormat::EMPTY : TextureDataFormat::DTYPE_INTEGER_MASK) | TextureDataFormat::DTYPE_U8;
        switch (REMOVE_MASK(dtype, ImageDataType::FLOAT_MASK))
        {
        case ImageDataType::RGB:    return baseFormat | TextureDataFormat::CHANNEL_RGB;
        case ImageDataType::BGR:    return baseFormat | TextureDataFormat::CHANNEL_BGR;
        case ImageDataType::RGBA:   return baseFormat | TextureDataFormat::CHANNEL_RGBA;
        case ImageDataType::BGRA:   return baseFormat | TextureDataFormat::CHANNEL_BGRA;
        case ImageDataType::GRAY:   return baseFormat | TextureDataFormat::CHANNEL_R;
        case ImageDataType::GA:     return baseFormat | TextureDataFormat::CHANNEL_RG;
        default:                    return TextureDataFormat::ERROR;
        }
    }
    constexpr static uint8_t UnitSize(const TextureDataFormat dformat) noexcept
    {
        if (HAS_FIELD(dformat, TextureDataFormat::DTYPE_COMP_MASK))
        {
            switch (dformat & TextureDataFormat::DTYPE_RAW_MASK)
            {
            case TextureDataFormat::COMP_332:
                return 1;
            case TextureDataFormat::COMP_4444:
            case TextureDataFormat::COMP_565:
            case TextureDataFormat::COMP_5551:
                return 2;
            case TextureDataFormat::COMP_8888:
            case TextureDataFormat::COMP_10_2:
            case TextureDataFormat::COMP_11_10:
            case TextureDataFormat::COMP_999_5:
                return 4;
            default:
                return 0;
            }
        }
        uint8_t size = 0;
        switch (dformat & TextureDataFormat::DTYPE_RAW_MASK)
        {
        case TextureDataFormat::DTYPE_U8:
        case TextureDataFormat::DTYPE_I8:
            size = 1; break;
        case TextureDataFormat::DTYPE_U16:
        case TextureDataFormat::DTYPE_I16:
        case TextureDataFormat::DTYPE_HALF:
            size = 2; break;
        case TextureDataFormat::DTYPE_U32:
        case TextureDataFormat::DTYPE_I32:
        case TextureDataFormat::DTYPE_FLOAT:
            size = 4; break;
        default:
            return 0;
        }
        switch (dformat & TextureDataFormat::CHANNEL_RAW_MASK)
        {
        case TextureDataFormat::CHANNEL_R:
        case TextureDataFormat::CHANNEL_G:
        case TextureDataFormat::CHANNEL_B:
        case TextureDataFormat::CHANNEL_A:
            return size *= 1;
        case TextureDataFormat::CHANNEL_RG:
            return size *= 2;
        case TextureDataFormat::CHANNEL_RGB:
            return size *= 3;
        case TextureDataFormat::CHANNEL_RGBA:
            return size *= 4;
        default:
            return 0;
        }
    }
    constexpr static ImageDataType ToImageDType(const TextureDataFormat format, const bool relaxConvert = false) noexcept
    {
        if (HAS_FIELD(format, TextureDataFormat::DTYPE_COMP_MASK))
            return ImageDataType::UNKNOWN_RESERVE;
        ImageDataType floatType = ImageDataType::EMPTY_MASK;
        switch (format & TextureDataFormat::DTYPE_MASK)
        {
        case TextureDataFormat::DTYPE_I8 | TextureDataFormat::DTYPE_INTEGER_MASK:
        case TextureDataFormat::DTYPE_U8 | TextureDataFormat::DTYPE_INTEGER_MASK:
        case TextureDataFormat::DTYPE_I8:
            if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE; 
            //only pass through when relaxConvert
            //[[fallthrough]]
        case TextureDataFormat::DTYPE_U8:        floatType = ImageDataType::EMPTY_MASK; break;
        case TextureDataFormat::DTYPE_FLOAT:     floatType = ImageDataType::FLOAT_MASK; break;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
        switch (format & TextureDataFormat::CHANNEL_MASK)
        {
        case TextureDataFormat::CHANNEL_G:
        case TextureDataFormat::CHANNEL_B:
            if (!relaxConvert) return ImageDataType::UNKNOWN_RESERVE;
            //only pass through when relaxConvert
            //[[fallthrough]]
        case TextureDataFormat::CHANNEL_R:      return ImageDataType::GRAY | floatType;
        case TextureDataFormat::CHANNEL_RG:     return ImageDataType::RA   | floatType;
        case TextureDataFormat::CHANNEL_RGB:    return ImageDataType::RGB  | floatType;
        case TextureDataFormat::CHANNEL_RGBA:   return ImageDataType::RGBA | floatType;
        case TextureDataFormat::CHANNEL_BGR:    return ImageDataType::BGR  | floatType;
        case TextureDataFormat::CHANNEL_BGRA:   return ImageDataType::BGRA | floatType;
        default:                                return ImageDataType::UNKNOWN_RESERVE;
        }
    }
    static IMGUTILAPI std::string GetFormatDetail(const TextureDataFormat format) noexcept;
};

}