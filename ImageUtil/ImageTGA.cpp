#include "ImageUtilRely.h"
#include "ImageTGA.h"
#include "DataConvertor.hpp"
#include "RGB15Converter.hpp"

namespace xziar::img::tga
{


TgaReader::TgaReader(FileObject& file) : OriginalFile(file), ImgFile(std::move(OriginalFile), 65536)
{
}

void TgaReader::Release()
{
    OriginalFile = std::move(ImgFile.Release());
}

bool TgaReader::Validate()
{
    using detail::TGAImgType;
    ImgFile.Rewind();
    if (!ImgFile.Read(Header))
        return false;
    switch (Header.ColorMapType)
    {
    case 1://paletted image
        if (REMOVE_MASK(Header.ImageType, TGAImgType::RLE_MASK) != TGAImgType::COLOR_MAP)
            return false;
        if (Header.PixelDepth != 8 && Header.PixelDepth != 16)
            return false;
        break;
    case 0://color image
        if (REMOVE_MASK(Header.ImageType, TGAImgType::RLE_MASK) == TGAImgType::GRAY)
        {
            if (Header.PixelDepth != 8)//gray must be 8 bit
                return false;
        }
        else if (REMOVE_MASK(Header.ImageType, TGAImgType::RLE_MASK) == TGAImgType::COLOR)
        {
            if (Header.PixelDepth != 15 && Header.PixelDepth != 16 && Header.PixelDepth != 24 && Header.PixelDepth != 32)//gray must not be 8 bit
                return false;
        }
        else
            return false;
        break;
    default:
        return false;
    }
    Width = (int16_t)convert::ParseWordLE(Header.Width);
    Height = (int16_t)convert::ParseWordLE(Header.Height);
    if (Width < 1 || Height < 1)
        return false;
    return true;
}


class TgaHelper
{
public:
    template<typename ReadFunc>
    static void ReadColorData4(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
    {
        if (output.GetElementSize() != 4)
            return;
        switch (colorDepth)
        {
        case 8:
            {
                AlignedBuffer<32> tmp(count);
                reader.Read(count, tmp.GetRawPtr());
                auto * __restrict destPtr = output.GetRawPtr();
                convert::GraysToRGBAs(destPtr, tmp.GetRawPtr(), count);
            }break;
        case 15:
            {
                std::vector<uint16_t> tmp(count);
                reader.Read(count, tmp);
                uint32_t * __restrict destPtr = output.GetRawPtr<uint32_t>();
                if (isOutputRGB)
                    convert::BGR15ToRGBAs(destPtr, tmp.data(), tmp.size());
                else
                    convert::RGB15ToRGBAs(destPtr, tmp.data(), tmp.size());
            }break;
        case 16:
            {
                std::vector<uint16_t> tmp(count);
                reader.Read(count, tmp);
                uint32_t * __restrict destPtr = output.GetRawPtr<uint32_t>();
                if (isOutputRGB)
                    convert::BGR16ToRGBAs(destPtr, tmp.data(), count);
                else
                    convert::RGB16ToRGBAs(destPtr, tmp.data(), count);
            }break;
        case 24://BGR
            {
                auto * __restrict destPtr = output.GetRawPtr();
                auto * __restrict srcPtr = destPtr + count;
                reader.Read(3 * count, srcPtr);
                if (isOutputRGB)
                    convert::BGRsToRGBAs(destPtr, srcPtr, count);
                else
                    convert::RGBsToRGBAs(destPtr, srcPtr, count);
            }break;
        case 32:
            {//BGRA
                reader.Read(4 * count, output.GetRawPtr());
                if (isOutputRGB)
                    convert::BGRAsToRGBAs(output.GetRawPtr(), count);
            }break;
        }
    }

    template<typename ReadFunc>
    static void ReadColorData3(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
    {
        if (output.GetElementSize() != 3)
            return;
        //auto& color16Map = isOutputRGB ? convert::BGR16ToRGBAMapper : convert::RGB16ToRGBAMapper;
        switch (colorDepth)
        {
        case 8:
            {
                AlignedBuffer<32> tmp(count);
                reader.Read(count, tmp.GetRawPtr());
                auto * __restrict destPtr = output.GetRawPtr();
                convert::GraysToRGBs(destPtr, tmp.GetRawPtr(), count);
            }break;
        case 15:
        case 16:
            {
                std::vector<uint16_t> tmp(count);
                reader.Read(count, tmp);
                auto * __restrict destPtr = output.GetRawPtr();
                if (isOutputRGB)
                    convert::BGR15ToRGBs(destPtr, tmp.data(), count);
                else
                    convert::RGB15ToRGBs(destPtr, tmp.data(), count);
            }break;
        case 24://BGR
            {
                reader.Read(3 * count, output.GetRawPtr());
                if (isOutputRGB)
                    convert::BGRsToRGBs(output.GetRawPtr(), count);
            }break;
        case 32://BGRA
            {
                Image tmp(ImageDataType::RGBA);
                tmp.SetSize(output.GetWidth(), 1);
                for (uint32_t row = 0; row < output.GetHeight(); ++row)
                {
                    reader.Read(4 * output.GetWidth(), tmp.GetRawPtr());
                    if (isOutputRGB)
                        convert::BGRAsToRGBs(output.GetRawPtr(row), tmp.GetRawPtr(), count);
                    else
                        convert::RGBAsToRGBs(output.GetRawPtr(row), tmp.GetRawPtr(), count);
                }
            }break;
        }
    }

    template<typename MapperReader, typename IndexReader>
    static void ReadFromColorMapped(const detail::TgaHeader& header, Image& image, MapperReader& mapperReader, IndexReader& reader)
    {
        const uint64_t count = (uint64_t)image.GetWidth() * image.GetHeight();
        const bool isOutputRGB = REMOVE_MASK(image.GetDataType(), ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::RGB;
        const bool needAlpha = HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK);

        const detail::ColorMapInfo mapInfo(header);
        mapperReader.Skip(mapInfo.Offset);
        Image mapper(needAlpha ? ImageDataType::RGBA : ImageDataType::RGB);
        mapper.SetSize(mapInfo.Size, 1);
        if (needAlpha)
            ReadColorData4(mapInfo.ColorDepth, mapInfo.Size, mapper, isOutputRGB, mapperReader);
        else
            ReadColorData3(mapInfo.ColorDepth, mapInfo.Size, mapper, isOutputRGB, mapperReader);

        if (needAlpha)
        {
            const uint32_t * __restrict const mapPtr = mapper.GetRawPtr<uint32_t>();
            uint32_t * __restrict destPtr = image.GetRawPtr<uint32_t>();
            if (header.PixelDepth == 8)
            {
                std::vector<uint8_t> idxes;
                reader.Read(count, idxes);
                for (auto idx : idxes)
                    *destPtr++ = mapPtr[idx];
            }
            else if (header.PixelDepth == 16)
            {
                std::vector<uint16_t> idxes;
                reader.Read(count, idxes);
                for (auto idx : idxes)
                    *destPtr++ = mapPtr[idx];
            }
        }
        else
        {
            const auto * __restrict const mapPtr = mapper.GetRawPtr();
            auto * __restrict destPtr = image.GetRawPtr();
            if (header.PixelDepth == 8)
            {
                std::vector<uint8_t> idxes;
                reader.Read(count, idxes);
                for (auto idx : idxes)
                {
                    const size_t idx3 = idx * 3;
                    *destPtr++ = mapPtr[idx3 + 0];
                    *destPtr++ = mapPtr[idx3 + 1];
                    *destPtr++ = mapPtr[idx3 + 2];
                }
            }
            else if (header.PixelDepth == 16)
            {
                std::vector<uint16_t> idxes;
                reader.Read(count, idxes);
                for (auto idx : idxes)
                {
                    const size_t idx3 = idx * 3;
                    *destPtr++ = mapPtr[idx3 + 0];
                    *destPtr++ = mapPtr[idx3 + 1];
                    *destPtr++ = mapPtr[idx3 + 2];
                }
            }
        }
    }

    template<typename ReadFunc>
    static void ReadDirect(const detail::TgaHeader& header, Image& image, ReadFunc& reader)
    {
        const uint64_t count = (uint64_t)image.GetWidth() * image.GetHeight();
        const bool needAlpha = HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK);
        if (image.isGray())
        {
            if (needAlpha)
            {
                reader.Read(count, image.GetRawPtr() + count);
                convert::GraysToGrayAs(image.GetRawPtr(), image.GetRawPtr() + count, count);
            }
            else
            {
                reader.Read(count, image.GetRawPtr());
            }
        }
        else
        {
            const bool isOutputRGB = REMOVE_MASK(image.GetDataType(), ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::RGB;
            if (needAlpha)
                ReadColorData4(header.PixelDepth, count, image, isOutputRGB, reader);
            else
                ReadColorData3(header.PixelDepth, count, image, isOutputRGB, reader);
        }
    }

    template<typename Writer>
    static void WriteRLE1(const byte* __restrict ptr, uint32_t len, const bool isRepeat, Writer& writer)
    {
        if (isRepeat)
        {
            const auto color = *ptr;
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(0x80 + (size - 1));
                writer.Write(flag);
                writer.Write(color);
            }
        }
        else
        {
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(size - 1);
                writer.Write(flag);
                writer.Write(size, ptr);
                ptr += size;
            }
        }
    }

    template<typename Writer>
    static void WriteRLE3(const byte* __restrict ptr, uint32_t len, const bool isRepeat, Writer& writer)
    {
        if (isRepeat)
        {
            byte color[3];
            color[0] = ptr[2], color[1] = ptr[1], color[2] = ptr[0];
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(0x80 + (size - 1));
                writer.Write(flag);
                writer.Write(color);
            }
        }
        else
        {
            AlignedBuffer<32> buffer(3 * 128);
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(size - 1);
                writer.Write(flag);
                convert::BGRsToRGBs(buffer.GetRawPtr(), ptr, size);
                ptr += 3 * size;
                writer.Write(3 * size, buffer.GetRawPtr());
            }
        }
    }

    template<typename Writer>
    static void WriteRLE4(const byte* __restrict ptr, uint32_t len, const bool isRepeat, Writer& writer)
    {
        if (isRepeat)
        {
            byte color[4];
            color[0] = ptr[2], color[1] = ptr[1], color[2] = ptr[0], color[3] = ptr[3];
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(0x80 + (size - 1));
                writer.Write(flag);
                writer.Write(color);
            }
        }
        else
        {
            AlignedBuffer<32> buffer(4 * 128);
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(size - 1);
                writer.Write(flag);
                convert::BGRAsToRGBAs(buffer.GetRawPtr(), ptr, size);
                ptr += 4 * size;
                writer.Write(4 * size, buffer.GetRawPtr());
            }
        }
    }

    template<typename Writer>
    static void WriteRLEColor3(const Image& image, Writer& writer)
    {
        if (image.GetElementSize() != 3)
            return;
        const uint32_t colMax = image.GetWidth() * 3;//tga's limit should promise this will not overflow
        for (uint32_t row = 0; row < image.GetHeight(); ++row)
        {
            const uint8_t * __restrict data = image.GetRawPtr<uint8_t>(row);
            uint32_t last = 0;
            uint32_t len = 0;
            bool repeat = false;
            for (uint32_t col = 0; col < colMax; col += 3, ++len)
            {
                uint32_t cur = data[col] + (data[col + 1] << 8) + (data[col + 2] << 16);
                switch (len)
                {
                case 0:
                    break;
                case 1:
                    repeat = (cur == last);
                    break;
                default:
                    if (cur == last && !repeat)//changed
                    {
                        WriteRLE3((const byte*)&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (cur != last && repeat)//changed
                    {
                        WriteRLE3((const byte*)&data[col - len], len, true, writer);
                        len = 0, repeat = false;
                    }
                }
                last = cur;
            }
            if (len > 0)
                WriteRLE3((const byte*)&data[image.GetWidth() - len], len, repeat, writer);
        }
    }

    template<typename Writer>
    static void WriteRLEColor4(const Image& image, Writer& writer)
    {
        if (image.GetElementSize() != 4)
            return;
        for (uint32_t row = 0; row < image.GetHeight(); ++row)
        {
            const uint32_t * __restrict data = image.GetRawPtr<uint32_t>(row);
            uint32_t last = 0;
            uint32_t len = 0;
            bool repeat = false;
            for (uint32_t col = 0; col < image.GetWidth(); ++col, ++len)
            {
                switch (len)
                {
                case 0:
                    break;
                case 1:
                    repeat = (data[col] == last);
                    break;
                default:
                    if (data[col] == last && !repeat)//changed
                    {
                        WriteRLE4((const byte*)&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (data[col] != last && repeat)//changed
                    {
                        WriteRLE4((const byte*)&data[col - len], len, true, writer);
                        len = 0, repeat = false;
                    }
                }
                last = data[col];
            }
            if (len > 0)
                WriteRLE4((const byte*)&data[image.GetWidth() - len], len, repeat, writer);
        }
    }
    template<typename Writer>
    static void WriteRLEGray(const Image& image, Writer& writer)
    {
        if (image.GetElementSize() != 1)
            return;
        for (uint32_t row = 0; row < image.GetHeight(); ++row)
        {
            const byte * __restrict data = image.GetRawPtr(row);
            byte last{};
            uint32_t len = 0;
            bool repeat = false;
            for (uint32_t col = 0; col < image.GetWidth(); ++col, ++len)
            {
                switch (len)
                {
                case 0:
                    break;
                case 1:
                    repeat = (data[col] == last);
                    break;
                default:
                    if (data[col] == last && !repeat)//changed
                    {
                        WriteRLE1(&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (data[col] != last && repeat)//changed
                    {
                        WriteRLE1(&data[col - len], len, true, writer);
                        len = 0, repeat = false;
                    }
                }
                last = data[col];
            }
            if (len > 0)
                WriteRLE1(&data[image.GetWidth() - len], len, repeat, writer);
        }
    }
};

//implementation promise each read should be at least a line, so no need for worry about overflow
//reading from file is the bottleneck
class RLEFileDecoder
{
private:
    BufferedFileReader& ImgFile;
    const uint8_t ElementSize;
    static inline uint8_t ByteToSize(const byte b)
    {
        return std::to_integer<uint8_t>(b & byte(0x7f)) + 1;
    }
    bool Read1(size_t limit, uint8_t * __restrict output)
    {
        while (limit)
        {
            const byte info = ImgFile.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                const auto obj = ImgFile.ReadByteNE<uint8_t>();
                common::copy::BroadcastMany(output, size, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!ImgFile.Read(size, output))
                    return false;
                output += size;
            }
        }
        return true;
    }
    bool Read2(size_t limit, uint16_t * __restrict output)
    {
        while (limit)
        {
            const byte info = ImgFile.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint16_t obj;
                if (!ImgFile.Read(obj))
                    return false;
                common::copy::BroadcastMany(output, size, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!ImgFile.Read(size * 2, output))
                    return false;
                output += size;
            }
        }
        return true;
    }
    bool Read3(size_t limit, uint8_t * __restrict output)
    {
        while (limit)
        {
            const byte info = ImgFile.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint8_t obj[3];
                if (!ImgFile.Read(3, obj))//use array load will keep invoke ftell(), which serverely decrease the performance
                    return false;
                for (auto count = size; count--;)
                {
                    *output++ = obj[0];
                    *output++ = obj[1];
                    *output++ = obj[2];
                }
            }
            else
            {
                if (!ImgFile.Read(size * 3, output))
                    return false;
                output += size * 3;
            }
        }
        return true;
    }
    bool Read4(size_t limit, uint32_t * __restrict output)
    {
        while (limit)
        {
            const byte info = ImgFile.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint32_t obj;
                if (!ImgFile.Read(obj))
                    return false;
                common::copy::BroadcastMany(output, size, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!ImgFile.Read(size * 4, output))
                    return false;
                output += size;
            }
        }
        return true;
    }
public:
    RLEFileDecoder(BufferedFileReader& file, const uint8_t elementDepth) : ImgFile(file), ElementSize(elementDepth == 15 ? 2 : (elementDepth / 8)) {}
    void Skip(const size_t offset = 0) { ImgFile.Skip(offset); }

    template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
    size_t Read(const size_t count, T& output)
    {
        return Read(count * sizeof(typename T::value_type), output.data()) ? count : 0;
    }

    bool Read(const size_t len, void *ptr)
    {
        switch (ElementSize)
        {
        case 1:
            return Read1(len, (uint8_t*)ptr);
        case 2:
            return Read2(len / 2, (uint16_t*)ptr);
        case 3:
            return Read3(len / 3, (uint8_t*)ptr);
        case 4:
            return Read4(len / 4, (uint32_t*)ptr);
        default:
            return false;
        }
    }
};

Image TgaReader::Read(const ImageDataType dataType)
{
    common::SimpleTimer timer;
    timer.Start();
    Image image(dataType);
    if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))//NotSuported 
        return image;
    if (image.isGray() && REMOVE_MASK(Header.ImageType, detail::TGAImgType::RLE_MASK) != detail::TGAImgType::GRAY)//down-convert, not supported
        return image;
    ImgFile.Rewind(detail::TGA_HEADER_SIZE + Header.IdLength);//Next ColorMap(optional)
    image.SetSize(Width, Height);
    if (Header.ColorMapType)
    {
        if (HAS_FIELD(Header.ImageType, detail::TGAImgType::RLE_MASK))
        {
            RLEFileDecoder decoder(ImgFile, Header.PixelDepth);
            TgaHelper::ReadFromColorMapped(Header, image, ImgFile, decoder);
        }
        else
            TgaHelper::ReadFromColorMapped(Header, image, ImgFile, ImgFile);
    }
    else
    {
        if (HAS_FIELD(Header.ImageType, detail::TGAImgType::RLE_MASK))
        {
            RLEFileDecoder decoder(ImgFile, Header.PixelDepth);
            TgaHelper::ReadDirect(Header, image, decoder);
        }
        else
            TgaHelper::ReadDirect(Header, image, ImgFile);
    }
    timer.Stop();
    ImgLog().debug(u"zextga read cost {} ms\n", timer.ElapseMs());
    timer.Start();
    switch ((Header.ImageDescriptor & 0x30) >> 4)//origin position
    {
    case 0://left-down
        image.FlipVertical(); break;
    case 1://right-down
        image.Rotate180(); break;
    case 2://left-up
        break;//no need to flip
    case 3://right-up
        image.FlipHorizontal(); break;
    }
    timer.Stop();
    ImgLog().debug(u"zextga flip cost {} ms\n", timer.ElapseMs()); 
    return image;
}

TgaWriter::TgaWriter(FileObject& file) : ImgFile(file)
{
}

void TgaWriter::Write(const Image& image)
{
    constexpr char identity[] = "Truevision TGA file created by zexTGA";
    if (image.GetWidth() > INT16_MAX || image.GetHeight() > INT16_MAX)
        return;
    if (HAS_FIELD(image.GetDataType(), ImageDataType::FLOAT_MASK))
        return;
    if (image.GetDataType() == ImageDataType::GA)
        return;
    detail::TgaHeader header;
    
    header.IdLength = sizeof(identity);
    header.ColorMapType = 0;
    header.ImageType = detail::TGAImgType::RLE_MASK | (image.isGray() ? detail::TGAImgType::GRAY : detail::TGAImgType::COLOR);
    memset(&header.ColorMapData, 0x0, 5);//5 bytes for color map spec
    convert::WordToLE(header.OriginHorizontal, 0);
    convert::WordToLE(header.OriginVertical, 0);
    convert::WordToLE(header.Width, (uint16_t)image.GetWidth());
    convert::WordToLE(header.Height, (uint16_t)image.GetHeight());
    header.PixelDepth = image.GetElementSize() * 8;
    header.ImageDescriptor = HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK) ? 0x28 : 0x20;
    
    ImgFile.Write(header);
    ImgFile.Write(identity);
    SimpleTimer timer;
    timer.Start();
    //next: true image data
    if (image.isGray())
        TgaHelper::WriteRLEGray(image, ImgFile);
    else if (HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK))
        TgaHelper::WriteRLEColor4(image, ImgFile);
    else
        TgaHelper::WriteRLEColor3(image, ImgFile);
    timer.Stop();
    ImgLog().debug(u"zextga write cost {} ms\n", timer.ElapseMs());
}


static auto DUMMY = RegistImageSupport<TgaSupport>();

}