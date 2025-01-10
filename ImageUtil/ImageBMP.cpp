#include "ImageUtilPch.h"
#include "ImageBMP.h"
#include "common/StaticLookup.hpp"
#include "SystemCommon/MiscIntrins.h"

namespace xziar::img::zex
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::SimpleTimer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


enum class PixFormat : uint8_t
{
    Category = 0xf0, Detail = 0x0f, RGB = 0x00, BGR = 0x08, NoAlpha = 0x00, HasAlpha = 0x04, IsGray = 0x80,
    Plate = 0x10, XXXX8888 = 0x20, XXX888 = 0x30, XXX555 = 0x40, XXX565 = 0x50, XX88 = IsGray | 0x10,
    RGBA8888 = XXXX8888 | HasAlpha | RGB, BGRA8888 = XXXX8888 | HasAlpha | BGR,
    RGBX888  = XXXX8888 |  NoAlpha | RGB, BGRX888  = XXXX8888 |  NoAlpha | BGR,
    RGB888   = XXX888   |  NoAlpha | RGB, BGR888   = XXX888   |  NoAlpha | BGR,
    RGBX555  = XXX555   |  NoAlpha | RGB, BGRX555  = XXX555   |  NoAlpha | BGR,
    RGBA5551 = XXX555   | HasAlpha | RGB, BGRA5551 = XXX555   | HasAlpha | BGR,
    RGB565   = XXX565   |  NoAlpha | RGB, BGR565   = XXX565   |  NoAlpha | BGR,
    RA88     = XX88     | HasAlpha | RGB, AR88     = XX88     | HasAlpha | BGR,
};
MAKE_ENUM_BITFIELD(PixFormat)


static void ReadUncompressed(Image& image, RandomInputStream& stream, AlignedBuffer& rowbuffer, bool needFlip, PixFormat format)
{
    const auto width = image.GetWidth(), height = image.GetHeight();
    const auto dataType = image.GetDataType();
    const auto needAlpha = dataType.HasAlpha();
    const auto hasAlpha = HAS_FIELD(format, PixFormat::HasAlpha);
    const auto needSwizzle = dataType.IsBGROrder() == HAS_FIELD(format, PixFormat::RGB);
    const size_t frowsize = rowbuffer.GetSize();
    const size_t irowsize = image.RowSize();
    const auto& cvter = ColorConvertor::Get();
    switch (REMOVE_MASK(format, PixFormat::Detail))
    {
    case PixFormat::XXXX8888:
    {
        const auto bufptr = rowbuffer.GetRawPtr<uint32_t>();
        for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
        {
            stream.Read(frowsize, bufptr);
            if (needAlpha)
            {
                const auto imgrow = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.RGBAToBGRA(imgrow, bufptr, width);
                else
                    memcpy_s(imgrow, irowsize, bufptr, frowsize);
                if (!hasAlpha)
                    util::FixAlpha(image.GetWidth(), imgrow);
            }
            else
            {
                const auto imgrow = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.RGBAToBGR(imgrow, bufptr, width);
                else
                    cvter.RGBAToRGB(imgrow, bufptr, width);
            }
        }
    } break;
    case PixFormat::XXX888:
    {
        auto bufptr = rowbuffer.GetRawPtr<uint8_t>();
        for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
        {
            stream.Read(frowsize, bufptr);
            if (needAlpha)
            {
                const auto imgrow = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.BGRToRGBA(imgrow, bufptr, width);
                else
                    cvter.RGBToRGBA(imgrow, bufptr, width);
            }
            else
            {
                const auto imgrow = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.RGBToBGR(imgrow, bufptr, width);
                else
                    memcpy_s(imgrow, irowsize, bufptr, frowsize);
            }
        }
    } break;
    case PixFormat::XXX555:
    {
        auto bufptr = rowbuffer.GetRawPtr<uint16_t>();
        for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
        {
            stream.Read(frowsize, bufptr);
            if (needAlpha)
            {
                const auto destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.BGR555ToRGBA(destPtr, bufptr, width, hasAlpha);
                else
                    cvter.RGB555ToRGBA(destPtr, bufptr, width, hasAlpha);
            }
            else
            {
                const auto destPtr = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.BGR555ToRGB(destPtr, bufptr, width);
                else
                    cvter.RGB555ToRGB(destPtr, bufptr, width);
            }
        }
    } break;
    case PixFormat::XXX565:
    {
        auto bufptr = rowbuffer.GetRawPtr<uint16_t>();
        for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
        {
            stream.Read(frowsize, bufptr);
            if (needAlpha)
            {
                const auto destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.BGR565ToRGBA(destPtr, bufptr, width);
                else
                    cvter.RGB565ToRGBA(destPtr, bufptr, width);
            }
            else
            {
                const auto destPtr = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.BGR565ToRGB(destPtr, bufptr, width);
                else
                    cvter.RGB565ToRGB(destPtr, bufptr, width);
            }
        }
    } break;
    case PixFormat::XX88:
    {
        Ensures(dataType.ChannelCount() < 3);
        auto bufptr = rowbuffer.GetRawPtr<uint16_t>();
        for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
        {
            stream.Read(frowsize, bufptr);
            if (needAlpha)
            {
                const auto destPtr = image.GetRawPtr<uint16_t>(needFlip ? j : i);
                if (needSwizzle)
                    for (uint32_t k = 0; k < width; ++k)
                        destPtr[k] = common::MiscIntrin.ByteSwap(bufptr[k]);
                else
                    memcpy_s(destPtr, irowsize, bufptr, frowsize);
            }
            else
            {
                const auto destPtr = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                if (needSwizzle)
                    cvter.GrayAToGray(destPtr, bufptr, width);
                else
                    cvter.GrayAToAlpha(destPtr, bufptr, width);
            }
        }
    } break;
    default:
        Ensures(false);
    }
}



BmpReader::BmpReader(RandomInputStream& stream) : Stream(stream), Info{}
{
}

constexpr size_t BMP_HEADER_SIZE = sizeof(detail::BmpHeader);
constexpr size_t BMP_INFO_SIZE = sizeof(detail::BmpInfo);
constexpr size_t BMP_INFO_V2_SIZE = sizeof(detail::BmpInfoV2);
constexpr size_t BMP_INFO_V3_SIZE = sizeof(detail::BmpInfoV3);
constexpr size_t BMP_INFO_V4_SIZE = sizeof(detail::BmpInfoV4);
constexpr size_t BMP_INFO_V5_SIZE = sizeof(detail::BmpInfoV5);

using Mask4 = std::array<uint32_t, 4>;

static forceinline constexpr bool CheckOverlap(const Mask4& mask) noexcept
{
    for (uint32_t i = 0; i < 4; ++i)
        for (uint32_t j = i + 1; j < 4; ++j)
            if (mask[i] & mask[j])
                return true;
    return false;
}

static constexpr auto Bit32MaskLUT = BuildStaticLookup(Mask4, PixFormat,
    { { 0x000000ffu, 0x0000ff00u, 0x00ff0000u, 0xff000000u }, PixFormat::RGBA8888 },
    { { 0x00ff0000u, 0x0000ff00u, 0x000000ffu, 0xff000000u }, PixFormat::BGRA8888 },
    { { 0x000000ffu, 0x0000ff00u, 0x00ff0000u, 0x00000000u }, PixFormat::RGBX888  },
    { { 0x00ff0000u, 0x0000ff00u, 0x000000ffu, 0x00000000u }, PixFormat::BGRX888  }
);
static constexpr auto Bit16MaskLUT = BuildStaticLookup(Mask4, PixFormat,
    { { 0x001fu, 0x03e0u, 0x7c00u, 0x8000u }, PixFormat::RGBA5551 },
    { { 0x7c00u, 0x03e0u, 0x001fu, 0x8000u }, PixFormat::BGRA5551 },
    { { 0x001fu, 0x03e0u, 0x7c00u, 0x0000u }, PixFormat::RGBX555  },
    { { 0x7c00u, 0x03e0u, 0x001fu, 0x0000u }, PixFormat::BGRX555  },
    { { 0x001fu, 0x07e0u, 0xf800u, 0x0000u }, PixFormat::RGB565   },
    { { 0xf800u, 0x07e0u, 0x001fu, 0x0000u }, PixFormat::BGR565   },
    { { 0x00ffu, 0x0000u, 0x0000u, 0xff00u }, PixFormat::RA88     },
    { { 0x0000u, 0x00ffu, 0x0000u, 0xff00u }, PixFormat::RA88     },
    { { 0x0000u, 0x0000u, 0x00ffu, 0xff00u }, PixFormat::RA88     },
    { { 0xff00u, 0x0000u, 0x0000u, 0x00ffu }, PixFormat::AR88     },
    { { 0x0000u, 0xff00u, 0x0000u, 0x00ffu }, PixFormat::AR88     },
    { { 0x0000u, 0x0000u, 0xff00u, 0x00ffu }, PixFormat::AR88     },
);

template<typename T>
static forceinline constexpr bool CheckOverlap(const T& items) noexcept
{
    for (const auto& item : items)
        if (CheckOverlap(item.Key))
            return true;
    return false;
}
static_assert(!CheckOverlap(Bit32MaskLUT.Items));
static_assert(!CheckOverlap(Bit16MaskLUT.Items));


bool BmpReader::Validate()
{
    Stream.SetPos(0);

    detail::BmpHeader header{};
    if (!Stream.Read(header))
        return false;
    if (header.Sig[0] != 'B' || header.Sig[1] != 'M' || header.Reserved != 0)
        return false;
    const auto size = util::ParseDWordLE(header.Size);
    const auto fsize = Stream.GetSize();
    if (size > 0 && size != Stream.GetSize())
        return false;
    if (util::ParseDWordLE(header.Offset) >= fsize)
        return false;

    return ValidateNoHeader(util::ParseDWordLE(header.Offset));
}

bool BmpReader::ValidateNoHeader(uint32_t pixOffset)
{
    InfoOffset = Stream.CurrentPos();

    if (!Stream.Read(static_cast<detail::BmpInfo&>(Info)))
        return false;
    if (Info.Planes != 1)
        return false;
    // allow colorful only
    if (Info.BitCount != 8 && Info.BitCount != 16 && Info.BitCount != 24 && Info.BitCount != 32)
        return false;
    if (util::ParseDWordLE(Info.Width) <= 0 || util::ParseDWordLE(Info.Height) == 0)
        return false;
    switch (Info.Size)
    {
    case BMP_INFO_V5_SIZE:
        if (!Stream.Read(BMP_INFO_V5_SIZE - BMP_INFO_SIZE, reinterpret_cast<std::byte*>(&Info) + BMP_INFO_SIZE))
            return false;
        break;
    case BMP_INFO_V4_SIZE:
        if (!Stream.Read(BMP_INFO_V4_SIZE - BMP_INFO_SIZE, reinterpret_cast<std::byte*>(&Info) + BMP_INFO_SIZE))
            return false;
        break;
    case BMP_INFO_V3_SIZE:
        if (!Stream.Read(BMP_INFO_V3_SIZE - BMP_INFO_SIZE, reinterpret_cast<std::byte*>(&Info) + BMP_INFO_SIZE))
            return false;
        break;
    case BMP_INFO_V2_SIZE:
        if (!Stream.Read(BMP_INFO_V2_SIZE - BMP_INFO_SIZE, reinterpret_cast<std::byte*>(&Info) + BMP_INFO_SIZE))
            return false;
        break;
    case BMP_INFO_SIZE: // need compatible changes
        if (Info.Compression == 3) // BI_BITFIELDS, assume it's V2 header
        {
            if (!Stream.Read(BMP_INFO_V2_SIZE - BMP_INFO_SIZE, reinterpret_cast<std::byte*>(&Info) + BMP_INFO_SIZE))
                return false;
        }
        else if (Info.Compression != 0) // allow BI_RGB only
            return false;
        if (Info.BitCount == 16)
            Info.MaskAlpha = 0x8000u;
        else if (Info.BitCount == 32)
            Info.MaskAlpha = 0xff000000u;
        break;
    default:
        // allow Windows Base/V4/V5 only
        return false;
    }

    if (pixOffset < Stream.CurrentPos()) // should not overlap
        return false;
    PixelOffset = pixOffset;

    switch (Info.Compression)
    {
    case 0: // BI_RGB
        if (Info.BitCount == 16)
            Format = common::enum_cast(Info.MaskAlpha ? PixFormat::BGRA5551 : PixFormat::BGRX555);
        else if (Info.BitCount == 32)
            Format = common::enum_cast(Info.MaskAlpha ? PixFormat::BGRA8888 : PixFormat::BGRX888);
        else if (Info.BitCount == 24)
            Format = common::enum_cast(PixFormat::BGR888);
        else if (Info.BitCount == 8)
            Format = common::enum_cast(PixFormat::Plate);
        else
            return false;
        break;
    case 3: // BI_BITFIELDS
        if (Info.BitCount != 16 && Info.BitCount != 32)
            return false;
        {
            Mask4 mask = { Info.MaskRed, Info.MaskGreen, Info.MaskBlue, Info.MaskAlpha };
            if (CheckOverlap(mask))
                return false; // must no overlapping
            const auto format = Info.BitCount == 32 ? Bit32MaskLUT(mask) : Bit16MaskLUT(mask);
            if (!format)
                return false;
            Format = common::enum_cast(*format);
        }
        break;
    default: // allow BI_RGB, BI_BITFIELDS only
        return false;
    }
    return true;
}

Image BmpReader::Read(ImgDType dataType)
{
    const auto format = static_cast<PixFormat>(Format);
    const auto isSrcGray = HAS_FIELD(format, PixFormat::IsGray);
    if (dataType)
    {
        if (dataType.IsFloat())
            COMMON_THROWEX(BaseException, u"Cannot read as float datatype");
        if (dataType.ChannelCount() < 3 && !isSrcGray)
            COMMON_THROWEX(BaseException, u"Cannot read as gray iamge");
    }
    else
    {
        const auto hasAlpha = HAS_FIELD(format, PixFormat::HasAlpha);
        const auto isBGR = HAS_FIELD(format, PixFormat::BGR);
        const auto ch = isSrcGray ? (hasAlpha ? ImgDType::Channels::RA : ImgDType::Channels::R) :
            (hasAlpha ? (isBGR ? ImgDType::Channels::BGRA : ImgDType::Channels::RGBA) : (isBGR ? ImgDType::Channels::BGR : ImgDType::Channels::RGB));
        dataType = ImgDType{ ch, ImgDType::DataTypes::Uint8 };
    }
    
    Image image(dataType);
    
    const int32_t h = util::ParseDWordLE(Info.Height);
    const bool needFlip = h > 0;
    const uint32_t height = std::abs(h);
    const uint32_t width = util::ParseDWordLE(Info.Width);
    image.SetSize(width, height);

    const size_t frowsize = ((Info.BitCount * width + 31) / 32) * 4;
    AlignedBuffer buffer(frowsize);

    if (format == PixFormat::Plate)
    {
        Ensures(dataType.ChannelCount() >= 3);
        const uint32_t paletteCount = Info.PaletteUsed ? Info.PaletteUsed : (1u << Info.BitCount);
        AlignedBuffer palette(paletteCount * 4);
        const auto pltptr = palette.GetRawPtr<uint32_t>();
        Stream.SetPos(InfoOffset + Info.Size);
        Stream.Read(paletteCount * 4, palette.GetRawPtr());
        Stream.SetPos(PixelOffset);

        const auto needAlpha = dataType.HasAlpha();
        const auto needSwizzle = !dataType.IsBGROrder();

        const auto& cvter = ColorConvertor::Get();

        const auto bufptr = buffer.GetRawPtr<uint8_t>();
        if (needAlpha) // need alpha
        {
            if (needSwizzle)
                cvter.RGBAToBGRA(pltptr, pltptr, paletteCount);
            util::FixAlpha(paletteCount, pltptr);
            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                Stream.Read(frowsize, bufptr);
                auto* __restrict destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                for (uint32_t col = 0; col < width; ++col)
                    destPtr[col] = pltptr[bufptr[col]];
            }
        }
        else // ignore alpha
        {
            AlignedBuffer palette3(paletteCount * 3);
            const auto plt3ptr = palette3.GetRawPtr<uint8_t>();
            if (needSwizzle)
                cvter.RGBAToBGR(plt3ptr, pltptr, paletteCount);
            else
                cvter.RGBAToRGB(plt3ptr, pltptr, paletteCount);

            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                Stream.Read(frowsize, bufptr);
                auto* __restrict destPtr = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                for (uint32_t col = 0; col < width; ++col)
                    memcpy_s(destPtr, 3, &plt3ptr[bufptr[col]], 3);
            }
        }
    }
    else
    {
        Stream.SetPos(PixelOffset);
        ReadUncompressed(image, Stream, buffer, needFlip, format);
    }
    return image;
}


BmpWriter::BmpWriter(RandomOutputStream& stream) : Stream(stream)
{
}

void BmpWriter::Write(ImageView image, const uint8_t)
{
    if (image.GetWidth() > INT32_MAX || image.GetHeight() > INT32_MAX)
        return;
    const auto origType = image.GetDataType();
    if (!origType.Is(ImgDType::DataTypes::Uint8))
        return;
    if (origType.Is(ImgDType::Channels::RA))
        image = image.ConvertTo(ImageDataType::BGRA);

    const auto dstDType = image.GetDataType();
    const bool isInputBGR = dstDType.IsBGROrder();
    const bool needAlpha = dstDType.HasAlpha();

    detail::BmpHeader header{};
    header.Sig[0] = 'B', header.Sig[1] = 'M';
    util::DWordToLE(header.Offset, BMP_HEADER_SIZE + BMP_INFO_SIZE);
    auto info = util::EmptyStruct<detail::BmpInfo>();
    info.Size = static_cast<uint32_t>(BMP_INFO_SIZE);
    util::DWordToLE(info.Width, image.GetWidth());
    util::DWordToLE(info.Height, -static_cast<int32_t>(image.GetHeight()));
    info.Planes = 1;
    info.BitCount = image.GetElementSize() * 8;
    info.Compression = 0;

    Stream.Write(header);
    Stream.Write(info);
    SimpleTimer timer;
    timer.Start();

    const size_t frowsize = ((info.BitCount * image.GetWidth() + 31) / 32) * 4;
    const size_t irowsize = image.RowSize();

    if (dstDType.Channel() == ImgDType::Channels::R)
    {
        static constexpr auto GrayToRGBAMAP = []()
        {
            std::array<uint32_t, 256> ret{ 0 };
            for (uint32_t i = 0; i < 256u; ++i)
                ret[i] = (i * 0x00010101u) | 0xff000000u;
            return ret;
        }();
        Stream.WriteFrom(GrayToRGBAMAP);
        if (frowsize == irowsize)
            Stream.Write(image.GetSize(), image.GetRawPtr());
        else
        {
            const byte* __restrict imgptr = image.GetRawPtr();
            const uint8_t empty[4] = { 0 };
            const size_t padding = frowsize - irowsize;
            for (uint32_t i = 0; i < image.GetHeight(); ++i)
            {
                Stream.Write(irowsize, imgptr);
                Stream.Write(padding, empty);
                imgptr += irowsize;
            }
        }
    }
    else if (frowsize == irowsize && isInputBGR) // perfect match, write directly
        Stream.Write(image.GetSize(), image.GetRawPtr());
    else
    {
        AlignedBuffer buffer(frowsize);
        byte* __restrict const bufptr = buffer.GetRawPtr();
        const auto& cvter = ColorConvertor::Get();
        for (uint32_t i = 0; i < image.GetHeight(); ++i)
        {
            auto rowptr = image.GetRawPtr(i);
            if (isInputBGR)
                Stream.Write(frowsize, rowptr);
            else if(needAlpha)
            {
                cvter.RGBAToBGRA(reinterpret_cast<uint32_t*>(bufptr), reinterpret_cast<const uint32_t*>(rowptr), image.GetWidth());
                Stream.Write(frowsize, bufptr);
            }
            else
            {
                cvter.RGBToBGR(reinterpret_cast<uint8_t*>(bufptr), reinterpret_cast<const uint8_t*>(rowptr), image.GetWidth());
                Stream.Write(frowsize, bufptr);
            }
        }
    }

    timer.Stop();
    ImgLog().Debug(u"zexbmp write cost {} ms\n", timer.ElapseMs());
}


uint8_t BmpSupport::MatchExtension(std::u16string_view ext, ImgDType dataType, const bool) const
{
    if (ext != u"BMP")
        return 0;
    if (dataType)
    {
        if (dataType.DataType() != ImgDType::DataTypes::Uint8)
            return 0;
    }
    return 192;
}

static auto DUMMY = RegistImageSupport<BmpSupport>();

}
