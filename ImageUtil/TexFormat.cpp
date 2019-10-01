#include "ImageUtilPch.h"
#include "TexFormat.h"


namespace xziar::img
{

using namespace std::string_view_literals;
using std::string_view;
using std::string;


constexpr static string_view GetChannelSV(const TextureFormat format) noexcept
{
    switch (format & TextureFormat::MASK_CHANNEL)
    {
    case TextureFormat::CHANNEL_R:      return "R"sv;
    case TextureFormat::CHANNEL_G:      return "G"sv;
    case TextureFormat::CHANNEL_B:      return "B"sv;
    case TextureFormat::CHANNEL_A:      return "A"sv;
    case TextureFormat::CHANNEL_RG:     return "RG"sv;
    case TextureFormat::CHANNEL_RGB:    return "RGB"sv;
    case TextureFormat::CHANNEL_BGR:    return "BGR"sv;
    case TextureFormat::CHANNEL_RGBA:   return "RGBA"sv;
    case TextureFormat::CHANNEL_BGRA:   return "BGRA"sv;
    case TextureFormat::CHANNEL_ABGR:   return "ABGR"sv;
    default:                            return "UNKNOWN"sv;
    }
}

//[unused|sRGB|channel-order|channels|type categroy|data type]
//[------|*14*|13.........12|11.....8|7...........6|5.......0]
std::string TexFormatUtil::GetFormatDetail(const TextureFormat format) noexcept
{
    if (format == TextureFormat::ERROR)
        return "ERROR";
    const auto category = format & TextureFormat::MASK_DTYPE_CAT;
    const auto dataType = format & TextureFormat::MASK_DTYPE_RAW;
    const std::string_view srgb = HAS_FIELD(format, TextureFormat::MASK_SRGB) ? "[sRGB]"sv : ""sv;
    switch (category)
    {
    case TextureFormat::DTYPE_CAT_COMP:
        {
            const auto channelOrder = format & TextureFormat::MASK_CHANNEL_ORDER;
            if (channelOrder == TextureFormat::CHANNEL_ORDER_REV_ENDIAN) return "ERROR";
            const std::string_view rev = channelOrder == TextureFormat::CHANNEL_ORDER_REV_RGB ? "REV"sv : ""sv;
            string_view dtype;
            switch (REMOVE_MASK(dataType, TextureFormat::MASK_COMP_NORMALIZE))
            {
            case TextureFormat::COMP_332:       dtype = "332"sv;     break;
            case TextureFormat::COMP_565:       dtype = "565"sv;     break;
            case TextureFormat::COMP_5551:      dtype = "5551"sv;    break;
            case TextureFormat::COMP_4444:      dtype = "4444"sv;    break;
            case TextureFormat::COMP_999_5:     dtype = "RGB9E5"sv;  break;
            case TextureFormat::COMP_10_2:      dtype = "10_2"sv;    break;
            case TextureFormat::COMP_11_10R:    dtype = "11_10"sv;   break;
            default:                            dtype = "UNKNOWN"sv; break;
            }
            const auto channel = GetChannelSV(format & TextureFormat::MASK_CHANNEL_RAW);
            return fmt::format(FMT_STRING("COMPOSITE [{:^7}{:3}] channel[{:^7}] {}"), dtype, rev, channel, srgb);
        }
    case TextureFormat::DTYPE_CAT_COMPRESS:
        {
            const auto channelOrder = format & TextureFormat::MASK_CHANNEL_ORDER;
            if (channelOrder != TextureFormat::CHANNEL_ORDER_NORMAL) return "ERROR";
            string_view dtype;
            switch (REMOVE_MASK(dataType, TextureFormat::MASK_COMPRESS_SIGNESS))
            {
            case TextureFormat::CPRS_DXT1:      dtype = "DXT1"sv;    break;
            case TextureFormat::CPRS_DXT3:      dtype = "DXT3"sv;    break;
            case TextureFormat::CPRS_DXT5:      dtype = "DXT5"sv;    break;
            case TextureFormat::CPRS_RGTC:      dtype = "RGTC"sv;    break;
            case TextureFormat::CPRS_BPTC:      dtype = "BPTC"sv;    break;
            case TextureFormat::CPRS_BPTCf:     dtype = "BPTCf"sv;   break;
            case TextureFormat::CPRS_ETC:       dtype = "ETC"sv;     break;
            case TextureFormat::CPRS_ETC2:      dtype = "ETC2"sv;    break;
            case TextureFormat::CPRS_PVRTC:     dtype = "PVRTC"sv;   break;
            case TextureFormat::CPRS_PVRTC2:    dtype = "PVRTC2"sv;  break;
            case TextureFormat::CPRS_ASTC:      dtype = "ASTC"sv;    break;
            default:                            dtype = "UNKNOWN"sv; break;
            }
            const auto channel = GetChannelSV(format & TextureFormat::MASK_CHANNEL_RAW);
            return fmt::format(FMT_STRING("COMPRESS [{:^7}] channel[{:^7}] {}"), dtype, channel, srgb);
        }
    case TextureFormat::DTYPE_CAT_PLAIN:
        {
            string_view dtype, dflag;
            if (dataType == TextureFormat::DTYPE_FLOAT) dtype = "Float"sv;
            else if (dataType == TextureFormat::DTYPE_HALF) dtype = "Half"sv;
            else
            {
                dflag = (dataType & TextureFormat::MASK_PLAIN_NORMALIZE) ==
                    TextureFormat::DTYPE_PLAIN_RAW ? "INTEGER"sv : "NORMAL"sv;
                switch (REMOVE_MASK(dataType, TextureFormat::MASK_PLAIN_NORMALIZE))
                {
                case TextureFormat::DTYPE_U8:   dtype = "U8"sv;    break;
                case TextureFormat::DTYPE_I8:   dtype = "I8"sv;    break;
                case TextureFormat::DTYPE_U16:  dtype = "U16"sv;   break;
                case TextureFormat::DTYPE_I16:  dtype = "I16"sv;   break;
                case TextureFormat::DTYPE_U32:  dtype = "U32"sv;   break;
                case TextureFormat::DTYPE_I32:  dtype = "I32"sv;   break;
                default:                        dtype = "UNKNOWN"sv; break;
                }
            }
            const auto channel = GetChannelSV(format);
            return fmt::format(FMT_STRING("PLAIN [{:^7}({:7})] channel[{:^7}] {}"), dtype, dflag, channel, srgb);
        }
    default:
        return "ERROR";
    }
}

std::u16string_view TexFormatUtil::GetFormatName(const TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::R8:            return u"R8"sv;
    case TextureFormat::RG8:           return u"RG8"sv;
    case TextureFormat::RGB8:          return u"RGB8"sv;
    case TextureFormat::SRGB8:         return u"SRGB8"sv;
    case TextureFormat::RGBA8:         return u"RGBA8"sv;
    case TextureFormat::SRGBA8:        return u"SRGBA8"sv;
    case TextureFormat::R16:           return u"R16"sv;
    case TextureFormat::RG16:          return u"RG16"sv;
    case TextureFormat::RGB16:         return u"RGB16"sv;
    case TextureFormat::RGBA16:        return u"RGBA16"sv;
    case TextureFormat::R8S:           return u"R8S"sv;
    case TextureFormat::RG8S:          return u"RG8S"sv;
    case TextureFormat::RGB8S:         return u"RGB8S"sv;
    case TextureFormat::RGBA8S:        return u"RGBA8S"sv;
    case TextureFormat::R16S:          return u"R16S"sv;
    case TextureFormat::RG16S:         return u"RG16S"sv;
    case TextureFormat::RGB16S:        return u"RGB16S"sv;
    case TextureFormat::RGBA16S:       return u"RGBA16S"sv;
    case TextureFormat::R8U:           return u"R8U"sv;
    case TextureFormat::RG8U:          return u"RG8U"sv;
    case TextureFormat::RGB8U:         return u"RGB8U"sv;
    case TextureFormat::RGBA8U:        return u"RGBA8U"sv;
    case TextureFormat::R16U:          return u"R16U"sv;
    case TextureFormat::RG16U:         return u"RG16U"sv;
    case TextureFormat::RGB16U:        return u"RGB16U"sv;
    case TextureFormat::RGBA16U:       return u"RGBA16U"sv;
    case TextureFormat::R32U:          return u"R32U"sv;
    case TextureFormat::RG32U:         return u"RG32U"sv;
    case TextureFormat::RGB32U:        return u"RGB32U"sv;
    case TextureFormat::RGBA32U:       return u"RGBA32U"sv;
    case TextureFormat::R8I:           return u"R8I"sv;
    case TextureFormat::RG8I:          return u"RG8I"sv;
    case TextureFormat::RGB8I:         return u"RGB8I"sv;
    case TextureFormat::RGBA8I:        return u"RGBA8I"sv;
    case TextureFormat::R16I:          return u"R16I"sv;
    case TextureFormat::RG16I:         return u"RG16I"sv;
    case TextureFormat::RGB16I:        return u"RGB16I"sv;
    case TextureFormat::RGBA16I:       return u"RGBA16I"sv;
    case TextureFormat::R32I:          return u"R32I"sv;
    case TextureFormat::RG32I:         return u"RG32I"sv;
    case TextureFormat::RGB32I:        return u"RGB32I"sv;
    case TextureFormat::RGBA32I:       return u"RGBA32I"sv;
    case TextureFormat::Rh:            return u"Rh"sv;
    case TextureFormat::RGh:           return u"RGh"sv;
    case TextureFormat::RGBh:          return u"RGBh"sv;
    case TextureFormat::RGBAh:         return u"RGBAh"sv;
    case TextureFormat::Rf:            return u"Rf"sv;
    case TextureFormat::RGf:           return u"RGf"sv;
    case TextureFormat::RGBf:          return u"RGBf"sv;
    case TextureFormat::RGBAf:         return u"RGBAf"sv;
    case TextureFormat::RG11B10:       return u"RG11B10"sv;
    case TextureFormat::RGB332:        return u"RGB332"sv;
    case TextureFormat::RGBA4444:      return u"RGBA4444"sv;
    case TextureFormat::RGB5A1:        return u"RGB5A1"sv;
    case TextureFormat::RGB565:        return u"RGB565"sv;
    case TextureFormat::RGB10A2:       return u"RGB10A2"sv;
    case TextureFormat::RGB10A2U:      return u"RGB10A2U"sv;
    //case TextureFormat::RGBA12:      return u"RGBA12"sv;
    case TextureFormat::BC1:           return u"BC1"sv;
    case TextureFormat::BC1A:          return u"BC1A"sv;
    case TextureFormat::BC2:           return u"BC2"sv;
    case TextureFormat::BC3:           return u"BC3"sv;
    case TextureFormat::BC4:           return u"BC4"sv;
    case TextureFormat::BC5:           return u"BC5"sv;
    case TextureFormat::BC6H:          return u"BC6H"sv;
    case TextureFormat::BC7:           return u"BC7"sv;
    case TextureFormat::BC1SRGB:       return u"BC1SRGB"sv;
    case TextureFormat::BC1ASRGB:      return u"BC1ASRGB"sv;
    case TextureFormat::BC2SRGB:       return u"BC2SRGB"sv;
    case TextureFormat::BC3SRGB:       return u"BC3SRGB"sv;
    case TextureFormat::BC7SRGB:       return u"BC7SRGB"sv;
    default:                           return u"Other"sv;
    }
}


}