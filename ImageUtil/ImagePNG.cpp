#include "ImageUtilPch.h"
#include "ImagePNG.h"

#include "libpng/png.h"
#include "zlib-ng/zlib.h"


#pragma message("Compiling ImagePNG with libpng[" STRINGIZE(PNG_LIBPNG_VER_STRING) "] AND zlib-ng[" STRINGIZE(ZLIBNG_VERSION) "](zlib[" STRINGIZE(ZLIB_VERSION) "])")

namespace xziar::img::png
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
    ImgLog().error(u"LIBPNG report an error: {}\n", message);
    COMMON_THROW(BaseException, u"Libpng report an error");
}
static void OnWarn([[maybe_unused]] png_structrp pngStruct, const char *message)
{
    ImgLog().warning(u"LIBPNG warns: {}\n", message);
}

static png_structp CreateReadStruct()
{
    auto handle = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, OnError, OnWarn);
    if (!handle)
        COMMON_THROW(BaseException, u"Cannot alloc space for png struct");
    return handle;
}
static png_structp CreateWriteStruct()
{
    auto handle = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, OnError, OnWarn);
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


static void ReadPng(void *pngStruct, const uint32_t passes, Image& image, const bool needAlpha, const bool isColor)
{
    auto ptrs = image.GetRowPtrs<uint8_t>(needAlpha ? image.GetWidth() : 0);
    common::SimpleTimer timer;
    timer.Start();
    for (auto pass = passes; pass--;)
    {
        //Sparkle, read all rows at a time
        png_read_rows((png_structp)pngStruct, ptrs.data(), NULL, image.GetHeight());
    }
    timer.Stop();
    ImgLog().debug(u"[libpng]decode {} pass cost {} ms\n", passes, timer.ElapseMs());
    if (!needAlpha)
        return;

    //post process, add alpha
    auto *rowPtr = image.GetRawPtr();
    const size_t lineStep = image.GetElementSize() * image.GetWidth();
    timer.Start();
    if (isColor)
        for (uint32_t row = 0; row < image.GetHeight(); row++, rowPtr += lineStep)
        {
            auto * __restrict destPtr = rowPtr;
            auto * __restrict srcPtr = rowPtr + image.GetWidth();
            convert::RGBsToRGBAs(destPtr, srcPtr, image.GetWidth());
        }
    else
        for (uint32_t row = 0; row < image.GetHeight(); row++, rowPtr += lineStep)
        {
            auto * __restrict destPtr = rowPtr;
            auto * __restrict srcPtr = rowPtr + image.GetWidth();
            convert::GraysToGrayAs(destPtr, srcPtr, image.GetWidth());
        }
    timer.Stop();
    ImgLog().debug(u"[png]post add alpha cost {} ms\n", timer.ElapseMs());
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

Image PngReader::Read(const ImageDataType dataType)
{
    auto pngStruct = (png_structp)PngStruct;
    auto pngInfo = (png_infop)PngInfo;
    Image image(dataType);
    if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
        //NotSupported Yet
        return image;
    Stream.SetPos(0);

    png_read_info(pngStruct, pngInfo);
    uint32_t width = 0, height = 0;
    int32_t bitDepth = -1, colorType = -1, interlaceType = -1;
    png_get_IHDR((png_structp)PngStruct, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    image.SetSize(width, height);

    if (bitDepth == 16)
    {
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
        png_set_scale_16(pngStruct);
#else
        png_set_strip_16(PngStruct);
#endif
    }
    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
    png_set_packing(pngStruct);
    switch (colorType)
    {
    case PNG_COLOR_TYPE_PALETTE:
        if (image.IsGray())
            return image;
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
        if (!image.IsGray())
            png_set_gray_to_rgb(pngStruct);
        break;
    default://color
        if (image.IsGray())
            return image;
    }
    /* Expand paletted or RGB images with transparency to full alpha channels
    * so the data will be available as RGBA quartets.
    */
    if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS) != 0)
        png_set_tRNS_to_alpha(pngStruct);
    /* Strip alpha bytes from the input data */
    if (!HAS_FIELD(dataType, ImageDataType::ALPHA_MASK))
        png_set_strip_alpha(pngStruct);
    /* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
    if (REMOVE_MASK(dataType, ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::BGR)
        png_set_swap_alpha(pngStruct);

    //handle interlace
    const uint32_t passes = (interlaceType == PNG_INTERLACE_NONE) ? 1 : png_set_interlace_handling(pngStruct);
    png_start_read_image(pngStruct);
    const bool needAlpha = HAS_FIELD(dataType, ImageDataType::ALPHA_MASK) && (colorType & PNG_COLOR_MASK_ALPHA) == 0;
    ReadPng(PngStruct, passes, image, needAlpha, !image.IsGray());
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

void PngWriter::Write(const Image& image, const uint8_t quality)
{
    // [0,75]=0, [76,82]=1, [83,87]=2, [88,90]=3, [91,92]=4, [93,94]=5, [95,96]=6, [97,98]=7, [99]=8, [100]=9
    const auto compLevel = static_cast<uint16_t>(std::pow((quality - 1)*0.01, 8) * 10);
    auto pngStruct = (png_structp)PngStruct;
    auto pngInfo = (png_infop)PngInfo;
    if (HAS_FIELD(image.GetDataType(), ImageDataType::FLOAT_MASK))
        //NotSupported Yet
        return;
    Stream.SetPos(0);

    const auto alphaMask = HAS_FIELD(image.GetDataType(), ImageDataType::ALPHA_MASK) ? PNG_COLOR_MASK_ALPHA : 0;
    const auto colorMask = image.IsGray() ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB;
    const auto colorType = alphaMask | colorMask;
    png_set_IHDR(pngStruct, pngInfo, image.GetWidth(), image.GetHeight(), 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_set_compression_level(pngStruct, compLevel);
    png_write_info(pngStruct, pngInfo);

    if (REMOVE_MASK(image.GetDataType(), ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::BGR)
        png_set_swap_alpha(pngStruct);

    auto ptrs = image.GetRowPtrs();
    png_write_image(pngStruct, (png_bytepp)ptrs.data());
    png_write_end(pngStruct, pngInfo);
}


static auto DUMMY = RegistImageSupport<PngSupport>();

}