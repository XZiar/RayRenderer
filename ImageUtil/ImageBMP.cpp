#include "ImageUtilPch.h"
#include "ImageBMP.h"

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


static void ReadUncompressed(Image& image, RandomInputStream& stream, bool needFlip, bool isRGB, bool hasAlpha, const detail::BmpInfo& info)
{
    const auto width = image.GetWidth(), height = image.GetHeight();
    const auto dataType = image.GetDataType();
    const auto baseDataType = REMOVE_MASK(dataType, ImageDataType::ALPHA_MASK);
    const auto needAlpha = HAS_FIELD(dataType, ImageDataType::ALPHA_MASK);
    const auto needSwizzle = baseDataType != (isRGB ? ImageDataType::RGB : ImageDataType::BGR);
    const size_t frowsize = ((info.BitCount * width + 31) / 32) * 4;
    const size_t irowsize = image.RowSize();
    AlignedBuffer buffer(frowsize);
    const auto& cvter = ColorConvertor::Get();
    switch (info.BitCount)
    {
    case 32:
        {
            const auto bufptr = buffer.GetRawPtr();
            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                stream.Read(frowsize, buffer.GetRawPtr());
                if (needAlpha)
                {
                    const auto imgrow = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                    if (needSwizzle)
                        cvter.RGBAToBGRA(reinterpret_cast<uint32_t*>(imgrow), reinterpret_cast<const uint32_t*>(bufptr), width);
                    else
                        memcpy_s(imgrow, irowsize, bufptr, frowsize);
                    if (!hasAlpha)
                        util::FixAlpha(image.GetWidth(), imgrow);
                }
                else
                {
                    const auto imgrow = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                    if (needSwizzle)
                        cvter.RGBAToBGR(imgrow, reinterpret_cast<const uint32_t*>(bufptr), width);
                    else
                        cvter.RGBAToRGB(imgrow, reinterpret_cast<const uint32_t*>(bufptr), width);
                }
            }
        } break;
    case 24:
        {
            const auto bufptr = buffer.GetRawPtr();
            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                stream.Read(frowsize, buffer.GetRawPtr());
                if (needAlpha)
                {
                    const auto imgrow = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                    if (needSwizzle)
                        cvter.BGRToRGBA(imgrow, reinterpret_cast<const uint8_t*>(bufptr), width);
                    else
                        cvter.RGBToRGBA(imgrow, reinterpret_cast<const uint8_t*>(bufptr), width);
                    if (!hasAlpha)
                        util::FixAlpha(image.GetWidth(), imgrow);
                }
                else
                {
                    const auto imgrow = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                    if (needSwizzle)
                        cvter.RGBToBGR(imgrow, reinterpret_cast<const uint8_t*>(bufptr), width);
                    else
                        memcpy_s(imgrow, irowsize, bufptr, frowsize);
                }
            }
        } break;
    case 16:
        {
            auto bufptr = buffer.GetRawPtr<uint16_t>();
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
    case 8:
        {
            const auto origPos = stream.CurrentPos();
            stream.SetPos(detail::BMP_HEADER_SIZE + info.Size);
            const uint32_t paletteCount = info.PaletteUsed ? info.PaletteUsed : (1u << info.BitCount);
            AlignedBuffer palette(paletteCount * 4);
            const auto pltptr = palette.GetRawPtr<uint32_t>();
            stream.Read(paletteCount * 4, palette.GetRawPtr());
            stream.SetPos(origPos);

            const auto bufptr = buffer.GetRawPtr<uint8_t>();
            if (needAlpha) // need alpha
            {
                if (needSwizzle)
                    cvter.RGBAToBGRA(pltptr, pltptr, paletteCount);
                util::FixAlpha(paletteCount, pltptr);
                for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
                {
                    stream.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
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
                    stream.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr<uint8_t>(needFlip ? j : i);
                    for (uint32_t col = 0; col < width; ++col)
                        memcpy_s(destPtr, 3, &plt3ptr[bufptr[col]], 3);
                }
            }
        } break;
    }

}



BmpReader::BmpReader(RandomInputStream& stream) : Stream(stream), Header{}, Info{}
{
}

constexpr size_t BMP_INFO_SIZE = sizeof(detail::BmpInfo);
constexpr size_t BMP_INFO_V4_SIZE = sizeof(detail::BmpInfoV4);
constexpr size_t BMP_INFO_V5_SIZE = sizeof(detail::BmpInfoV5);

bool BmpReader::Validate()
{
    Stream.SetPos(0);

    if (!Stream.Read(Header))
        return false;
    if (Header.Sig[0] != 'B' || Header.Sig[1] != 'M' || Header.Reserved != 0)
        return false;
    const auto size = util::ParseDWordLE(Header.Size);
    const auto fsize = Stream.GetSize();
    if (size > 0 && size != Stream.GetSize())
        return false;
    if (util::ParseDWordLE(Header.Offset) >= fsize)
        return false;

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
    case BMP_INFO_SIZE: // need compatible changes
        if (Info.Compression != 0) // allow BI_RGB only
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
    
    switch (Info.Compression)
    {
    case 0: // BI_RGB
        if (Info.BitCount == 16)
            HasAlpha = Info.MaskAlpha;
        else if (Info.BitCount == 32)
            HasAlpha = Info.MaskAlpha;
        break;
    case 3: // BI_BITFIELDS
        if (Info.BitCount != 16 && Info.BitCount != 32)
            return false;
        {
            uint32_t mask[4] = { Info.MaskRed, Info.MaskGreen, Info.MaskBlue, Info.MaskAlpha };
            for (uint32_t i = 0; i < 4; ++i)
                for (uint32_t j = i + 1; j < 4; ++j)
                    if (mask[i] & mask[j])
                        return false; // must no overlapping
            const uint32_t compBits = Info.BitCount == 32 ? 8u : 5u;
            const uint32_t comp0Mask = (1u << compBits) - 1;
            const uint32_t comp1Mask = comp0Mask << (compBits * 1);
            const uint32_t comp2Mask = comp1Mask << (compBits * 1);
            const uint32_t comp3Mask = Info.BitCount == 32 ? 0xff000000u : 0x8000u;
            if (Info.MaskGreen != comp1Mask) // G must be idx 1
                return false;
            if (Info.MaskAlpha == comp3Mask)
                HasAlpha = true;
            else if (Info.MaskAlpha == 0u)
                HasAlpha = false;
            else
                return false;
            if (Info.MaskRed == comp0Mask && Info.MaskBlue == comp2Mask) // RGB
                IsRGB = true;
            else if (Info.MaskRed == comp2Mask && Info.MaskBlue == comp0Mask) // RGB
                IsRGB = false;
            else
                return false;
        }
        break;
    default: // allow BI_RGB, BI_BITFIELDS only
        return false;
    }
    return true;
}

Image BmpReader::Read(ImageDataType dataType)
{
    if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
        return Image();
    Image image(dataType);
    if (image.IsGray())
        return image;
    
    const int32_t h = util::ParseDWordLE(Info.Height);
    const bool needFlip = h > 0;
    const uint32_t height = std::abs(h);
    const uint32_t width = util::ParseDWordLE(Info.Width);
    image.SetSize(width, height);

    Stream.SetPos(util::ParseDWordLE(Header.Offset));

    ReadUncompressed(image, Stream, needFlip, IsRGB, HasAlpha, Info);
    
    return image;
}


BmpWriter::BmpWriter(RandomOutputStream& stream) : Stream(stream), Header{}, Info{}
{
}

void BmpWriter::Write(const Image& image, const uint8_t)
{
    if (image.GetWidth() > INT32_MAX || image.GetHeight() > INT32_MAX)
        return;
    if (HAS_FIELD(image.GetDataType(), ImageDataType::FLOAT_MASK))
        return;
    if (image.GetDataType() == ImageDataType::GA)
        return;

    const bool isInputBGR = REMOVE_MASK(image.GetDataType(), ImageDataType::FLOAT_MASK, ImageDataType::ALPHA_MASK) == ImageDataType::BGR;
    const bool needAlpha = HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK);

    auto header = util::EmptyStruct<detail::BmpHeader>();
    header.Sig[0] = 'B', header.Sig[1] = 'M';
    util::DWordToLE(header.Offset, detail::BMP_HEADER_SIZE + BMP_INFO_SIZE);
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
    
    if (image.IsGray()) // must be ImageDataType::Gray only, use plate
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

static auto DUMMY = RegistImageSupport<BmpSupport>();

}
