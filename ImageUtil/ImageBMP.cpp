#include "ImageUtilRely.h"
#include "ImageBMP.h"
#include "DataConvertor.hpp"
#include "RGB15Converter.hpp"

namespace xziar::img::bmp
{


static void ReadUncompressed(Image& image, BufferedFileReader& imgfile, bool needFlip, const detail::BmpInfo& info)
{
    const auto width = image.Width, height = image.Height;
    const auto dataType = image.DataType;
    const size_t frowsize = ((info.BitCount * width + 31) / 32) * 4;
    const size_t irowsize = image.RowSize();
    AlignedBuffer<32> buffer(frowsize);
    switch (info.BitCount)
    {
    case 32://BGRA
        {
            const auto bufptr = buffer.GetRawPtr();
            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                imgfile.Read(frowsize, buffer.GetRawPtr());
                auto imgrow = image.GetRawPtr(needFlip ? j : i);
                switch (dataType)
                {
                case ImageDataType::BGRA:
                    memmove_s(imgrow, irowsize, bufptr, frowsize); break;
                case ImageDataType::RGBA:
                    convert::BGRAsToRGBAs(imgrow, bufptr, width); break;
                case ImageDataType::BGR:
                    convert::BGRAsToBGRs(imgrow, bufptr, width); break;
                case ImageDataType::RGB:
                    convert::BGRAsToRGBs(imgrow, bufptr, width); break;
                default:
                    return;
                }
            }
        }break;
    case 24://BGR
        {
            const auto bufptr = buffer.GetRawPtr();
            for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
            {
                imgfile.Read(frowsize, buffer.GetRawPtr());
                auto imgrow = image.GetRawPtr(needFlip ? j : i);
                switch (dataType)
                {
                case ImageDataType::BGR:
                    memcpy_s(imgrow, irowsize, bufptr, frowsize); break;
                case ImageDataType::RGB:
                    convert::BGRsToRGBs(imgrow, bufptr, width); break;
                case ImageDataType::BGRA:
                    convert::BGRsToBGRAs(imgrow, bufptr, width); break;
                case ImageDataType::RGBA:
                    convert::BGRsToRGBAs(imgrow, bufptr, width); break;
                default:
                    return;
                }
            }
        }break;
    case 16:
        {
            const bool isOutputRGB = REMOVE_MASK(dataType, ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::RGB;
            auto bufptr = buffer.GetRawPtr<uint16_t>();
            if (HAS_FIELD(dataType, ImageDataType::ALPHA_MASK))//need alpha
            {
                for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
                {
                    imgfile.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                    if (isOutputRGB)
                        convert::BGR15ToRGBAs(destPtr, bufptr, width);//ignore alpha
                    else
                        convert::RGB15ToRGBAs(destPtr, bufptr, width);//ignore alpha
                }
            }
            else//ignore alpha
            {
                for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
                {
                    imgfile.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr(needFlip ? j : i);
                    if (isOutputRGB)
                        convert::BGR15ToRGBs(destPtr, bufptr, width);//ignore alpha
                    else
                        convert::RGB15ToRGBs(destPtr, bufptr, width);//ignore alpha
                }
            }
        }break;
    case 8:
        {
            const auto carraypos = imgfile.CurrentPos();
            imgfile.Rewind(detail::BMP_HEADER_SIZE + info.Size);
            const uint32_t paletteCount = info.PaletteUsed ? info.PaletteUsed : (1u << info.BitCount);
            AlignedBuffer<32> palette(paletteCount * 4);
            imgfile.Read(paletteCount * 4, palette.GetRawPtr());
            convert::FixAlpha(paletteCount, palette.GetRawPtr<uint32_t>());
            imgfile.Rewind(carraypos);

            const bool isOutputRGB = REMOVE_MASK(dataType, ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::RGB;
            if (isOutputRGB)
                convert::BGRAsToRGBAs(palette.GetRawPtr(), paletteCount);//to RGBA

            const auto bufptr = buffer.GetRawPtr();
            const auto pltptr = palette.GetRawPtr<uint32_t>();
            if (HAS_FIELD(dataType, ImageDataType::ALPHA_MASK))//need alpha
            {
                for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
                {
                    imgfile.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr<uint32_t>(needFlip ? j : i);
                    for (uint32_t col = 0; col < width; ++col)
                    {
                        destPtr[col] = pltptr[std::to_integer<uint8_t>(bufptr[col])];
                    }
                }
            }
            else//ignore alpha
            {
                for (uint32_t i = 0, j = height - 1; i < height; ++i, --j)
                {
                    imgfile.Read(frowsize, bufptr);
                    auto * __restrict destPtr = image.GetRawPtr(needFlip ? j : i);
                    for (uint32_t col = 0; col < width; ++col)
                    {
                        const uint32_t color = pltptr[std::to_integer<uint8_t>(bufptr[col])];
                        convert::CopyRGBAToRGB(destPtr, color);
                    }
                }
            }
        }break;
    }

}



BmpReader::BmpReader(FileObject& file) : OriginalFile(file), ImgFile(std::move(OriginalFile), 65536)
{
}

void BmpReader::Release()
{
    OriginalFile = std::move(ImgFile.Release());
}

bool BmpReader::Validate()
{
    ImgFile.Rewind();
    if (!ImgFile.Read(Header))
        return false;
    if (!ImgFile.Read(Info))
        return false;

    if (Header.Sig[0] != 'B' || Header.Sig[1] != 'M' || Header.Reserved != 0)
        return false;
    auto size = convert::ParseDWordLE(Header.Size);
    const auto fsize = ImgFile.GetSize();
    if (size > 0 && size != ImgFile.GetSize())
        return false;
    if (convert::ParseDWordLE(Header.Offset) >= fsize)
        return false;
    //allow Windows V3/V4/V5 only
    if (Info.Size != 40 && Info.Size != 108 && Info.Size != 124)
        return false;
    if (Info.Planes != 1)
        return false;
    //allow colorful only
    if (Info.BitCount != 8 && Info.BitCount != 16 && Info.BitCount != 24 && Info.BitCount != 32)
        return false;
    //allow BI_RGB, BI_BITFIELDS only
    if (Info.Compression != 0 && Info.Compression != 3)
        return false;
    if (convert::ParseDWordLE(Info.Width) <= 0 || convert::ParseDWordLE(Info.Height) == 0)
        return false;
    
    return true;
}

Image BmpReader::Read(const ImageDataType dataType)
{
    if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
        return Image();
    Image image(dataType);
    if (image.isGray())
        return image;
    const int32_t h = convert::ParseDWordLE(Info.Height);
    const bool needFlip = h > 0;
    const uint32_t height = std::abs(h);
    const uint32_t width = convert::ParseDWordLE(Info.Width);
    image.SetSize(width, height);

    ImgFile.Rewind(convert::ParseDWordLE(Header.Offset));
    
    if (Info.Compression == 0)//BI_RGB
    {
        ReadUncompressed(image, ImgFile, needFlip, Info);
    }
    
    return image;
}


BmpWriter::BmpWriter(FileObject& file) : ImgFile(file)
{
}

void BmpWriter::Write(const Image& image)
{
    if (image.Width > INT32_MAX || image.Height > INT32_MAX)
        return;
    if (HAS_FIELD(image.DataType, ImageDataType::FLOAT_MASK))
        return;
    if (image.DataType == ImageDataType::GA)
        return;

    const bool isInputBGR = REMOVE_MASK(image.DataType, ImageDataType::FLOAT_MASK, ImageDataType::ALPHA_MASK) == ImageDataType::BGR;
    const bool needAlpha = HAS_FIELD(image.DataType, ImageDataType::ALPHA_MASK);

    auto header = convert::EmptyStruct<detail::BmpHeader>();
    header.Sig[0] = 'B', header.Sig[1] = 'M';
    convert::DWordToLE(header.Offset, detail::BMP_HEADER_SIZE + detail::BMP_INFO_SIZE);
    auto info = convert::EmptyStruct<detail::BmpInfo>();
    info.Size = static_cast<uint32_t>(detail::BMP_INFO_SIZE);
    convert::DWordToLE(info.Width, image.Width); convert::DWordToLE(info.Height, -static_cast<int32_t>(image.Height));
    info.Planes = 1;
    info.BitCount = image.ElementSize * 8;
    info.Compression = 0;

    ImgFile.Write(header);
    ImgFile.Write(info);
    SimpleTimer timer;
    timer.Start();

    const size_t frowsize = ((info.BitCount * image.Width + 31) / 32) * 4;
    const size_t irowsize = image.RowSize();
    
    if (image.isGray())//must be ImageDataType::Gray only
    {
        ImgFile.Write(256, convert::GrayToRGBAMAP);
        if (frowsize == irowsize)
            ImgFile.Write(image.GetSize(), image.GetRawPtr());
        else
        {
            const byte* __restrict imgptr = image.GetRawPtr();
            const uint8_t empty[4] = { 0 };
            const size_t padding = frowsize - irowsize;
            for (uint32_t i = 0; i < image.Height; ++i)
            {
                ImgFile.Write(irowsize, imgptr);
                ImgFile.Write(padding, empty);
            }
        }
    }
    else if (frowsize == irowsize && isInputBGR)//perfect match, write directly
        ImgFile.Write(image.GetSize(), image.GetRawPtr());
    else
    {
        AlignedBuffer<32> buffer(frowsize);
        byte* __restrict const bufptr = buffer.GetRawPtr();
        for (uint32_t i = 0; i < image.Height; ++i)
        {
            auto rowptr = image.GetRawPtr(i);
            if (isInputBGR)
                ImgFile.Write(frowsize, rowptr);
            else if(needAlpha)
            {
                convert::BGRAsToRGBAs(bufptr, rowptr, image.Width);
                ImgFile.Write(frowsize, bufptr);
            }
            else
            {
                convert::BGRsToRGBs(bufptr, rowptr, image.Width);
                ImgFile.Write(frowsize, bufptr);
            }
        }
    }

    timer.Stop();
    ImgLog().debug(u"zexbmp write cost {} ms\n", timer.ElapseMs());
}

BmpSupport::BmpSupport() : ImgSupport(u"Bmp")
{
}

}
