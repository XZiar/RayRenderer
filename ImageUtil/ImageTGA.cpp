#include "ImageUtilPch.h"
#include "ImageTGA.h"
#include "ColorConvert.h"

namespace xziar::img::zex
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


enum class TGAImgType : uint8_t
{
    RLE_MASK = 0x8, EMPTY = 0,
    COLOR_MAP = 1, COLOR = 2, GRAY = 3
};
MAKE_ENUM_BITFIELD(TGAImgType)

constexpr size_t TGA_HEADER_SIZE = sizeof(detail::TgaHeader);

struct ColorMapInfo
{
    uint16_t Offset;
    uint16_t Size;
    uint8_t ColorDepth;
    ColorMapInfo(const detail::TgaHeader& header)
    {
        Offset = common::simd::EndianReader<uint16_t, true>(header.ColorMapData.ColorMapOffset);
        Size = common::simd::EndianReader<uint16_t, true>(header.ColorMapData.ColorMapCount);
        ColorDepth = header.ColorMapData.ColorEntryDepth;
    }
};


class TgaHelper
{
public:
    template<typename ReadFunc>
    static void ReadColorData4(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
    {
        if (output.GetElementSize() != 4)
            return;
        const auto& cvter = ColorConvertor::Get();
        switch (colorDepth)
        {
        case 8:
        {
            AlignedBuffer tmp(count);
            reader.Read(count, tmp.GetRawPtr());
            cvter.GrayToRGBA(output.GetRawPtr<uint32_t>(), tmp.GetRawPtr<uint8_t>(), count);
        } break;
        case 15:
        {
            const auto tmp = reader.template ReadToVector<uint16_t>(count);
            uint32_t * __restrict destPtr = output.GetRawPtr<uint32_t>();
            if (isOutputRGB)
                cvter.BGR555ToRGBA(destPtr, tmp.data(), tmp.size(), false);
            else
                cvter.RGB555ToRGBA(destPtr, tmp.data(), tmp.size(), false);
        } break;
        case 16:
        {
            const auto tmp = reader.template ReadToVector<uint16_t>(count);
            uint32_t * __restrict destPtr = output.GetRawPtr<uint32_t>();
            if (isOutputRGB)
                cvter.BGR555ToRGBA(destPtr, tmp.data(), tmp.size(), true);
            else
                cvter.RGB555ToRGBA(destPtr, tmp.data(), tmp.size(), true);
        } break;
        case 24: // BGR
        {
            auto * __restrict destPtr = output.GetRawPtr();
            auto * __restrict srcPtr = destPtr + count;
            reader.Read(3 * count, srcPtr);
            if (isOutputRGB)
                cvter.BGRToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), count);
            else
                cvter.RGBToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), count);
        } break;
        case 32: // BGRA
        {
            reader.Read(4 * count, output.GetRawPtr());
            if (isOutputRGB)
                cvter.RGBAToBGRA(output.GetRawPtr<uint32_t>(), output.GetRawPtr<uint32_t>(), count);
        } break;
        }
    }

    template<typename ReadFunc>
    static void ReadColorData3(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
    {
        if (output.GetElementSize() != 3)
            return;
        const auto& cvter = ColorConvertor::Get();
        switch (colorDepth)
        {
        case 8:
        {
            AlignedBuffer tmp(count);
            reader.Read(count, tmp.GetRawPtr());
            cvter.GrayToRGB(output.GetRawPtr<uint8_t>(), tmp.GetRawPtr<uint8_t>(), count);
        } break;
        case 15:
        case 16:
        {
            const auto tmp = reader.template ReadToVector<uint16_t>(count);
            auto * __restrict destPtr = output.GetRawPtr<uint8_t>();
            if (isOutputRGB)
                cvter.BGR555ToRGB(destPtr, tmp.data(), tmp.size());
            else
                cvter.RGB555ToRGB(destPtr, tmp.data(), tmp.size());
        } break;
        case 24: // BGR
        {
            reader.Read(3 * count, output.GetRawPtr());
            if (isOutputRGB)
                cvter.RGBToBGR(output.GetRawPtr<uint8_t>(), output.GetRawPtr<uint8_t>(), count);
        } break;
        case 32: // BGRA
        {
            Image tmp(ImageDataType::RGBA);
            tmp.SetSize(output.GetWidth(), 1);
            for (uint32_t row = 0; row < output.GetHeight(); ++row)
            {
                reader.Read(4 * output.GetWidth(), tmp.GetRawPtr());
                if (isOutputRGB)
                    cvter.RGBAToBGR(output.GetRawPtr<uint8_t>(row), tmp.GetRawPtr<uint32_t>(), count);
                else
                    cvter.RGBAToRGB(output.GetRawPtr<uint8_t>(row), tmp.GetRawPtr<uint32_t>(), count);
            }
        } break;
        }
    }

    template<typename MapperReader, typename IndexReader>
    static void ReadFromColorMapped(const detail::TgaHeader& header, Image& image, MapperReader& mapperReader, IndexReader& reader)
    {
        const uint64_t count = (uint64_t)image.GetWidth() * image.GetHeight();
        const auto dataType = image.GetDataType();
        const bool isOutputRGB = !dataType.IsBGROrder();
        const bool needAlpha = dataType.HasAlpha();

        const ColorMapInfo mapInfo(header);
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
                const auto idxes = reader.template ReadToVector<uint8_t>(count);
                for (auto idx : idxes)
                    *destPtr++ = mapPtr[idx];
            }
            else if (header.PixelDepth == 16)
            {
                const auto idxes = reader.template ReadToVector<uint16_t>(count);
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
                const auto idxes = reader.template ReadToVector<uint8_t>(count);
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
                const auto idxes = reader.template ReadToVector<uint16_t>(count);
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
        const auto dataType = image.GetDataType();
        const bool needAlpha = dataType.HasAlpha();
        if (image.IsGray())
        {
            if (needAlpha)
            {
                reader.Read(count, image.GetRawPtr() + count);
                ColorConvertor::Get().GrayToGrayA(image.GetRawPtr<uint16_t>(), image.GetRawPtr<uint8_t>() + count, count);
            }
            else
            {
                reader.Read(count, image.GetRawPtr());
            }
        }
        else
        {
            const bool isOutputRGB = !dataType.IsBGROrder();
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
            AlignedBuffer buffer(3 * 128);
            const auto& cvter = ColorConvertor::Get();
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(size - 1);
                writer.Write(flag);
                cvter.RGBToBGR(buffer.GetRawPtr<uint8_t>(), reinterpret_cast<const uint8_t*>(ptr), size);
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
            AlignedBuffer buffer(4 * 128);
            const auto& cvter = ColorConvertor::Get();
            while (len)
            {
                const auto size = std::min(128u, len);
                len -= size;
                const auto flag = byte(size - 1);
                writer.Write(flag);
                cvter.RGBAToBGRA(buffer.GetRawPtr<uint32_t>(), reinterpret_cast<const uint32_t*>(ptr), size);
                ptr += 4 * size;
                writer.Write(4 * size, buffer.GetRawPtr());
            }
        }
    }

    template<typename Writer>
    static void WriteRLEColor3(const ImageView& image, Writer& writer)
    {
        if (image.GetElementSize() != 3)
            return;
        const uint32_t colMax = image.GetWidth() * 3; // tga's limit should promise this will not overflow
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
                    if (cur == last && !repeat) // changed
                    {
                        WriteRLE3((const byte*)&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (cur != last && repeat) // changed
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
    static void WriteRLEColor4(const ImageView& image, Writer& writer)
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
                    if (data[col] == last && !repeat) // changed
                    {
                        WriteRLE4((const byte*)&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (data[col] != last && repeat) // changed
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
    static void WriteRLEGray(const ImageView& image, Writer& writer)
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
                    if (data[col] == last && !repeat) // changed
                    {
                        WriteRLE1(&data[col - len], len - 1, false, writer);
                        len = 1, repeat = true;
                    }
                    else if (data[col] != last && repeat) // changed
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
    RandomInputStream& Stream;
    const uint8_t ElementSize;
    static inline uint8_t ByteToSize(const byte b)
    {
        return std::to_integer<uint8_t>(b & byte(0x7f)) + 1;
    }
    bool Read1(size_t limit, uint8_t * __restrict output)
    {
        while (limit)
        {
            const byte info = *Stream.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                const auto obj = *Stream.ReadByteNE<uint8_t>();
                common::CopyEx.BroadcastMany(output, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!Stream.Read(size, output))
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
            const byte info = *Stream.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint16_t obj;
                if (!Stream.Read(obj))
                    return false;
                common::CopyEx.BroadcastMany(output, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!Stream.Read(size * 2, output))
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
            const byte info = *Stream.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint8_t obj[3];
                if (!Stream.Read(3, obj)) // use array load will keep invoke ftell(), which serverely decrease the performance
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
                if (!Stream.Read(size * 3, output))
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
            const byte info = *Stream.ReadByteNE();
            const uint8_t size = ByteToSize(info);
            if (size > limit)
                return false;
            limit -= size;
            if (::HAS_FIELD(info, 0x80))
            {
                uint32_t obj;
                if (!Stream.Read(obj))
                    return false;
                common::CopyEx.BroadcastMany(output, obj, size);
                output += size;
                /*for (auto count = size; count--;)
                    *output++ = obj;*/
            }
            else
            {
                if (!Stream.Read(size * 4, output))
                    return false;
                output += size;
            }
        }
        return true;
    }
public:
    RLEFileDecoder(RandomInputStream& stream, const uint8_t elementDepth) 
        : Stream(stream), ElementSize(elementDepth == 15 ? 2 : (elementDepth / 8)) {}
    void Skip(const size_t offset = 0) { Stream.Skip(offset); }

    template<typename T>
    forceinline std::vector<T> ReadToVector(size_t count)
    {
        constexpr auto EleSize = sizeof(T);
        std::vector<T> output;
        output.resize(count);
        Read(count * EleSize, output.data());
        return output;
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


TgaReader::TgaReader(RandomInputStream& stream) : Stream(stream), Header{ util::EmptyStruct<detail::TgaHeader>() }
{
}

bool TgaReader::Validate()
{
    Stream.SetPos(0);
    if (!Stream.Read(Header))
        return false;
    Width  = common::simd::EndianReader<int16_t, true>(Header.Width);
    Height = common::simd::EndianReader<int16_t, true>(Header.Height);
    if (Width < 1 || Height < 1)
        return false;
    const auto type = static_cast<TGAImgType>(Header.ImageType);
    switch (Header.ColorMapType)
    {
    case 1: // paletted image
        if (REMOVE_MASK(type, TGAImgType::RLE_MASK) != TGAImgType::COLOR_MAP)
            return false;
        if (Header.PixelDepth != 8 && Header.PixelDepth != 16)
            return false;
        break;
    case 0: // color image
        if (REMOVE_MASK(type, TGAImgType::RLE_MASK) == TGAImgType::GRAY)
        {
            if (Header.PixelDepth != 8) // gray must be 8 bit
                return false;
        }
        else if (REMOVE_MASK(type, TGAImgType::RLE_MASK) == TGAImgType::COLOR)
        {
            if (Header.PixelDepth != 15 && Header.PixelDepth != 16 && Header.PixelDepth != 24 && Header.PixelDepth != 32) // color must not be 8 bit
                return false;
        }
        else
            return false;
        break;
    default:
        return false;
    }
    return true;
}

Image TgaReader::Read(ImgDType dataType)
{
    const auto type = static_cast<TGAImgType>(Header.ImageType);
    const auto isSrcGray = REMOVE_MASK(type, TGAImgType::RLE_MASK) == TGAImgType::GRAY;
    if (dataType)
    {
        if (!dataType.Is(ImgDType::DataTypes::Uint8))
            COMMON_THROWEX(BaseException, u"Cannot read as non-uint8 datatype");
        if (dataType.ChannelCount() < 3 && !isSrcGray)
            COMMON_THROWEX(BaseException, u"Cannot read as gray iamge");
    }
    else
    {
        const auto ch = isSrcGray ? ImgDType::Channels::R : 
            ((Header.PixelDepth == 15 || Header.PixelDepth == 24) ? ImgDType::Channels::BGR : ImgDType::Channels::BGRA);
        dataType = ImgDType{ ch, ImgDType::DataTypes::Uint8 };
    }
    Image image(dataType);
    image.SetSize(Width, Height);
    Stream.SetPos(TGA_HEADER_SIZE + Header.IdLength); // Next ColorMap(optional)
    common::SimpleTimer timer;
    timer.Start();
    const auto isRLE = HAS_FIELD(type, TGAImgType::RLE_MASK);
    if (Header.ColorMapType)
    {
        if (isRLE)
        {
            RLEFileDecoder decoder(Stream, Header.PixelDepth);
            TgaHelper::ReadFromColorMapped(Header, image, Stream, decoder);
        }
        else
            TgaHelper::ReadFromColorMapped(Header, image, Stream, Stream);
    }
    else
    {
        if (isRLE)
        {
            RLEFileDecoder decoder(Stream, Header.PixelDepth);
            TgaHelper::ReadDirect(Header, image, decoder);
        }
        else
            TgaHelper::ReadDirect(Header, image, Stream);
    }
    timer.Stop();
    const auto timeRead = timer.ElapseUs() / 1000.f;

    timer.Start();
    bool flipped = false;
    switch ((Header.ImageDescriptor & 0x30) >> 4)//origin position
    {
    case 0: flipped = true; image.FlipVertical(); break; // left-down
    case 1: flipped = true; image.Rotate180(); break; // right-down
    case 2: break; // left-up, no need to flip
    case 3: flipped = true; image.FlipHorizontal(); break; // right-up
    default: CM_UNREACHABLE(); break;
    }
    timer.Stop();
    const auto timeFlip = timer.ElapseUs() / 1000.f;

    const auto& syntax = common::str::FormatterCombiner::Combine(FmtString(u"zextga read[{}ms]\n"sv), FmtString(u"zextga read[{}ms] flip[{}ms]\n"sv));
    ImgLog().Debug(syntax(flipped), timeRead, timeFlip);
    return image;
}

TgaWriter::TgaWriter(RandomOutputStream& stream) : Stream(stream)
{
}

void TgaWriter::Write(ImageView image, const uint8_t)
{
    constexpr char identity[] = "Truevision TGA file created by zexTGA";
    if (image.GetWidth() > INT16_MAX || image.GetHeight() > INT16_MAX)
        return;
    const auto dstDType = image.GetDataType();
    if (!dstDType.Is(ImgDType::DataTypes::Uint8))
        return;
    if (dstDType == ImageDataType::GA)
        return;
    detail::TgaHeader header;
    
    header.IdLength = sizeof(identity);
    header.ColorMapType = 0;
    header.ImageType = common::enum_cast(TGAImgType::RLE_MASK | (image.IsGray() ? TGAImgType::GRAY : TGAImgType::COLOR));
    memset(&header.ColorMapData, 0x0, 5);//5 bytes for color map spec
    common::simd::EndianWriter<true>(header.OriginHorizontal, uint16_t(0));
    common::simd::EndianWriter<true>(header.OriginVertical,   uint16_t(0));
    common::simd::EndianWriter<true>(header.Width,  static_cast<uint16_t>(image.GetWidth()));
    common::simd::EndianWriter<true>(header.Height, static_cast<uint16_t>(image.GetHeight()));
    header.PixelDepth = image.GetElementSize() * 8;
    header.ImageDescriptor = dstDType.HasAlpha() ? 0x28 : 0x20;
    
    Stream.Write(header);
    Stream.Write(identity);
    SimpleTimer timer;
    timer.Start();
    // next: true image data
    switch (dstDType.ChannelCount())
    {
    case 1: TgaHelper::WriteRLEGray  (image, Stream); break;
    case 3: TgaHelper::WriteRLEColor3(image, Stream); break;
    case 4: TgaHelper::WriteRLEColor4(image, Stream); break;
    default: Ensures(false);
    }
    timer.Stop();
    ImgLog().Debug(u"zextga write cost {} ms\n", timer.ElapseMs());
}

uint8_t TgaSupport::MatchExtension(std::u16string_view ext, ImgDType dataType, const bool) const
{
    if (ext != u"TGA")
        return 0;
    if (dataType)
    {
        if (dataType.DataType() != ImgDType::DataTypes::Uint8)
            return 0;
    }
    return 160;
}

static auto DUMMY = RegistImageSupport<TgaSupport>();

}