#include "ImageUtilPch.h"
#include "ImagePNG.h"

#include "libpng/png.h"
#include "zlib-ng/zlib.h"


#pragma message("Compiling ImagePNG with libpng[" STRINGIZE(PNG_LIBPNG_VER_STRING) "] AND zlib-ng[" STRINGIZE(ZLIBNG_VERSION) "](zlib[" STRINGIZE(ZLIB_VERSION) "])")

namespace xziar::img::libpng
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::BaseException;
using common::SimpleTimer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


constexpr static size_t PNG_BYTES_TO_CHECK = 8;


static void OnReadStream(png_structp pngStruct, uint8_t *data, size_t length)
{
    auto& stream = *(RandomInputStream*)png_get_io_ptr(pngStruct);
    stream.Read(length, data);
}
static void OnWriteStream(png_structp pngStruct, uint8_t *data, size_t length)
{
    auto& stream = *(RandomOutputStream*)png_get_io_ptr(pngStruct);
    stream.Write(length, data);
}
static void OnFlushFile([[maybe_unused]] png_structp pngStruct) {}

static void OnError([[maybe_unused]] png_structrp pngStruct, const char *message)
{
    ImgLog().Error(u"LIBPNG report an error: {}\n", message);
    COMMON_THROW(BaseException, u"Libpng report an error");
}
static void OnWarn([[maybe_unused]] png_structrp pngStruct, const char *message)
{
    ImgLog().Warning(u"LIBPNG warns: {}\n", message);
}

static png_structp CreateReadStruct()
{
    auto handle = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, OnError, OnWarn);
    if (!handle)
        COMMON_THROW(BaseException, u"Cannot alloc space for png struct");
    return handle;
}
static png_structp CreateWriteStruct()
{
    auto handle = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, OnError, OnWarn);
    if (!handle)
        COMMON_THROW(BaseException, u"Cannot alloc space for png struct");
    return handle;
}
static png_infop CreateInfo(png_structp pngStruct)
{
    auto handle = png_create_info_struct(pngStruct);
    if (!handle)
        COMMON_THROW(BaseException, u"Cannot alloc space for png info");
    return handle;
}


PngReader::PngReader(RandomInputStream& stream) :
    Stream(stream), PngStruct(CreateReadStruct()), PngInfo(CreateInfo((png_structp)PngStruct))
{
    png_set_read_fn((png_structp)PngStruct, &Stream, OnReadStream);
}

PngReader::~PngReader()
{
    if (PngStruct)
    {
        if (PngInfo)
            png_destroy_read_struct((png_structpp)&PngStruct, (png_infopp)&PngInfo, nullptr);
        else
            png_destroy_read_struct((png_structpp)&PngStruct, nullptr, nullptr);
    }
}

bool PngReader::Validate()
{
    Stream.SetPos(0);
    uint8_t buf[PNG_BYTES_TO_CHECK];
    if (Stream.ReadInto(buf) != PNG_BYTES_TO_CHECK)
        return false;
    return !png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK);
}

Image PngReader::Read(const ImgDType dataType)
{
    auto pngStruct = (png_structp)PngStruct;
    auto pngInfo = (png_infop)PngInfo;
    if (!dataType.Is(ImgDType::DataTypes::Uint8) && !dataType.Is(ImgDType::DataTypes::Uint16))
        COMMON_THROW(BaseException, u"only uint8 and uint16 datatype supported");
    auto targetType = dataType;
    Stream.SetPos(0);

    png_read_info(pngStruct, pngInfo);
    uint32_t width = 0, height = 0;
    int32_t bitDepth = -1, colorType = -1, interlaceType = -1;
    png_get_IHDR((png_structp)PngStruct, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    if (bitDepth == 16 && dataType.DataType() == ImgDType::DataTypes::Uint8)
    {
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
        png_set_scale_16(pngStruct);
#else
        png_set_strip_16(pngStruct);
#endif
    }
    else if (bitDepth == 8 && dataType.DataType() == ImgDType::DataTypes::Uint16)
        targetType.SetDatatype(ImgDType::DataTypes::Uint8); // convert later
    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
    png_set_packing(pngStruct);
    const auto askGray = targetType.ChannelCount() < 3;
    switch (colorType)
    {
    case PNG_COLOR_TYPE_PALETTE:
        if (askGray) // gray
            COMMON_THROW(BaseException, u"cannot read platte into gray");
        /* Expand paletted colors into true RGB triplets */
        png_set_palette_to_rgb(pngStruct);
        break;
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        if (bitDepth < 8)
        {
            if (bitDepth == 1)
                png_set_invert_mono(pngStruct);
            /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
            png_set_expand_gray_1_2_4_to_8(pngStruct);
        }
        if (!askGray)
            png_set_gray_to_rgb(pngStruct);
        break;
    default: // color
        if (askGray)
            COMMON_THROW(BaseException, u"cannot read RGB into gray");
        if (bitDepth < 8)
            COMMON_THROW(BaseException, u"RGB should not be less than 8bit");
    }
    /* Expand paletted or RGB images with transparency to full alpha channels
    * so the data will be available as RGBA quartets.
    */
    if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS) != 0)
        png_set_tRNS_to_alpha(pngStruct);
    const bool srcHasAlpha = colorType & PNG_COLOR_MASK_ALPHA;
    if (!dataType.HasAlpha() && srcHasAlpha)
        png_set_strip_alpha(pngStruct);
    else if (dataType.HasAlpha() && !srcHasAlpha)
        png_set_add_alpha(pngStruct, 0xffff, PNG_FILLER_AFTER);
    if (dataType.IsBGROrder())
        png_set_bgr(pngStruct);

    //handle interlace
    const uint32_t passes = (interlaceType == PNG_INTERLACE_NONE) ? 1 : png_set_interlace_handling(pngStruct);
    
    Image image(targetType);
    image.SetSize(width, height, false);
    png_start_read_image(pngStruct);
    {
        auto ptrs = image.GetRowPtrs<uint8_t>();
        common::SimpleTimer timer;
        timer.Start();
        for (uint32_t i = 0; i < passes; ++i)
        {
            // Sparkle, read all rows at a time
            png_read_rows(pngStruct, ptrs.data(), nullptr, image.GetHeight());
        }
        timer.Stop();
        ImgLog().Debug(u"[libpng]decode {} pass cost {} ms\n", passes, timer.ElapseMs());

        // post process, extend to 16bit
        if (targetType != dataType)
        {
            Ensures(targetType.Channel() == dataType.Channel());
            Ensures(targetType.DataType() == ImgDType::DataTypes::Uint8 && dataType.DataType() == ImgDType::DataTypes::Uint16);
            timer.Start();
            image = image.ConvertTo(dataType);
            timer.Stop();
            ImgLog().Debug(u"[png]post uint8->uint16 cost {} ms\n", timer.ElapseMs());
        }
    }
    png_read_end(pngStruct, pngInfo);
    return image;
}


PngWriter::PngWriter(RandomOutputStream& stream)
    : Stream(stream), PngStruct(CreateWriteStruct()), PngInfo(CreateInfo((png_structp)PngStruct))
{
    png_set_write_fn((png_structp)PngStruct, &Stream, OnWriteStream, OnFlushFile);
}

PngWriter::~PngWriter()
{
    if (PngStruct)
    {
        if (PngInfo)
            png_destroy_write_struct((png_structpp)&PngStruct, (png_infopp)&PngInfo);
        else
            png_destroy_write_struct((png_structpp)&PngStruct, nullptr);
    }
}

void PngWriter::Write(ImageView image, const uint8_t quality)
{
    // [0,75]=0, [76,82]=1, [83,87]=2, [88,90]=3, [91,92]=4, [93,94]=5, [95,96]=6, [97,98]=7, [99]=8, [100]=9
    const auto compLevel = static_cast<uint16_t>(std::pow((quality - 1)*0.01, 8) * 10);
    auto pngStruct = (png_structp)PngStruct;
    auto pngInfo = (png_infop)PngInfo;
    auto dstDType = image.GetDataType();
    if (!dstDType.Is(ImgDType::DataTypes::Uint8) && !dstDType.Is(ImgDType::DataTypes::Uint16))
        COMMON_THROW(BaseException, u"only uint8 and uint16 datatype supported");
    Stream.SetPos(0);

    const auto alphaMask = dstDType.HasAlpha() ? PNG_COLOR_MASK_ALPHA : 0;
    const auto colorMask = dstDType.ChannelCount() < 3 ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB;
    const auto colorType = alphaMask | colorMask;
    png_set_IHDR(pngStruct, pngInfo, image.GetWidth(), image.GetHeight(), dstDType.Is(ImgDType::DataTypes::Uint8) ? 8 : 16, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_set_compression_level(pngStruct, compLevel);
    if (dstDType.IsBGROrder())
        png_set_bgr(pngStruct);
    png_write_info(pngStruct, pngInfo);

    auto ptrs = image.GetRowPtrs();
    png_write_image(pngStruct, (png_bytepp)ptrs.data());
    png_write_end(pngStruct, pngInfo);
}


uint8_t PngSupport::MatchExtension(std::u16string_view ext, ImgDType type, const bool) const
{
    if (const auto dt = type.DataType(); dt != ImgDType::DataTypes::Uint16 && dt != ImgDType::DataTypes::Uint8)
        return 0;
    return ext == u"PNG" ? 240 : 0;
}

static auto DUMMY = RegistImageSupport<PngSupport>();

}