#include "ImageUtilRely.h"
#include "TexFormat.h"


namespace xziar::img
{

using namespace std::string_view_literals;
using std::string_view;
using std::string;


constexpr static string_view GetChannelSV(const TextureDataFormat format) noexcept
{
    switch (format & TextureDataFormat::CHANNEL_MASK)
    {
    case TextureDataFormat::CHANNEL_R:      return "R"sv;
    case TextureDataFormat::CHANNEL_G:      return "G"sv;
    case TextureDataFormat::CHANNEL_B:      return "B"sv;
    case TextureDataFormat::CHANNEL_A:      return "A"sv;
    case TextureDataFormat::CHANNEL_RG:     return "RG"sv;
    case TextureDataFormat::CHANNEL_RGB:    return "RGB"sv;
    case TextureDataFormat::CHANNEL_BGR:    return "BGR"sv;
    case TextureDataFormat::CHANNEL_RGBA:   return "RGBA"sv;
    case TextureDataFormat::CHANNEL_BGRA:   return "BGRA"sv;
    default:                                return "UNKNOWN"sv;
    }
}

//[unused|channel-reverse|channels|normalized|composed|data type]
//[------|******12*******|11.....8|****7*****|***6****|5.......0]
std::string TexDFormatUtil::GetFormatDetail(const TextureDataFormat format) noexcept
{
    if (format == TextureDataFormat::ERROR)
        return "ERROR";
    const auto rawType = format & TextureDataFormat::DTYPE_RAW_MASK;
    if (HAS_FIELD(format, TextureDataFormat::DTYPE_COMP_MASK))
    {
        const std::string_view rev = HAS_FIELD(format, TextureDataFormat::CHANNEL_REVERSE_MASK) ? "REV"sv : ""sv;
        string_view dtype;
        switch (rawType)
        {
        case TextureDataFormat::COMP_332:       dtype = "332"sv;   break;
        case TextureDataFormat::COMP_565:       dtype = "565"sv;   break;
        case TextureDataFormat::COMP_5551:      dtype = "5551"sv;  break;
        case TextureDataFormat::COMP_4444:      dtype = "4444"sv;  break;
        case TextureDataFormat::COMP_8888:      dtype = "8888"sv;  break;
        case TextureDataFormat::COMP_10_2:      dtype = "10_2"sv;  break;
        case TextureDataFormat::COMP_11_10R:    dtype = "11_10"sv; break;
        default:                                dtype = "UNKNOWN"sv; break;
        }
        const auto channel = GetChannelSV(REMOVE_MASK(format, TextureDataFormat::CHANNEL_REVERSE_MASK));
        return fmt::format(FMT_STRING("COMPOSITE[{:^7}{:3}]channel[{:^7}]"), dtype, rev, channel);
    }
    else
    {
        const string_view dflag = HAS_FIELD(format, TextureDataFormat::DTYPE_INTEGER_MASK) ? "INTEGER"sv : "NORMAL"sv;
        string_view dtype;
        switch (rawType)
        {
        case TextureDataFormat::DTYPE_U8:    dtype = "U8"sv;    break;
        case TextureDataFormat::DTYPE_I8:    dtype = "I8"sv;    break;
        case TextureDataFormat::DTYPE_U16:   dtype = "U16"sv;   break;
        case TextureDataFormat::DTYPE_I16:   dtype = "I16"sv;   break;
        case TextureDataFormat::DTYPE_U32:   dtype = "U32"sv;   break;
        case TextureDataFormat::DTYPE_I32:   dtype = "I32"sv;   break;
        case TextureDataFormat::DTYPE_HALF:  dtype = "Half"sv;  break;
        case TextureDataFormat::DTYPE_FLOAT: dtype = "Float"sv; break;
        default:                             dtype = "UNKNOWN"sv; break;
        }
        const auto channel = GetChannelSV(format);
        return fmt::format("[{:^7}({:7})]channel[{:^7}]", dtype, dflag, channel);
    }
}

}