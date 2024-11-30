#include "ImageUtilPch.h"
#include "ImageNDK.h"
#include "SystemCommon/Format.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/DynamicLibrary.h"

#include "common/StaticLookup.hpp"

#undef __INTRODUCED_IN // not directly use, ignore api check
#include <android/bitmap.h>
#include <android/data_space.h>
#include <android/imagedecoder.h>


namespace xziar::img::ndk
{
using common::BaseException;
using namespace std::string_view_literals;

#define DEFINE_FUNC(func, name) using T_P##name = decltype(&func); static constexpr auto N_##name = #func ""sv
#define DECLARE_FUNC(name) T_P##name name = nullptr
#define LOAD_FUNC(name) name = Lib.GetFunction<T_P##name>(N_##name)
#define TrLd_FUNC(name) name = Lib.TryGetFunction<T_P##name>(N_##name)

DEFINE_FUNC(AImageDecoder_createFromBuffer,                 DecFromBuffer);
DEFINE_FUNC(AImageDecoder_delete,                           ReleaseDec);
DEFINE_FUNC(AImageDecoder_getHeaderInfo,                    GetInfo);
DEFINE_FUNC(AImageDecoderHeaderInfo_getWidth,               GetWidth);
DEFINE_FUNC(AImageDecoderHeaderInfo_getHeight,              GetHeight);
DEFINE_FUNC(AImageDecoderHeaderInfo_getAndroidBitmapFormat, GetFormat);
DEFINE_FUNC(AImageDecoder_getMinimumStride,                 GetStride);
DEFINE_FUNC(AImageDecoder_decodeImage,                      DecodeImg);
DEFINE_FUNC(AndroidBitmap_compress,                         EncodeImg);
DEFINE_FUNC(AImageDecoder_resultToString,                   RetcodeStr);


#define DecError(name) { ANDROID_IMAGE_DECODER_##name, #name }
static constexpr auto DecoderErrorLookup = BuildStaticLookup(int32_t, std::string_view,
    DecError(SUCCESS),
    DecError(INCOMPLETE),
    DecError(ERROR),
    DecError(INVALID_CONVERSION),
    DecError(INVALID_SCALE),
    DecError(BAD_PARAMETER),
    DecError(INVALID_INPUT),
    DecError(SEEK_ERROR),
    DecError(INTERNAL_ERROR),
    DecError(UNSUPPORTED_FORMAT),
    DecError(FINISHED),
    DecError(INVALID_STATE));
#undef DecError

#define EncError(name) { ANDROID_BITMAP_RESULT_##name, #name }
static constexpr auto EncoderErrorLookup = BuildStaticLookup(int32_t, std::string_view,
    EncError(SUCCESS),
    EncError(BAD_PARAMETER),
    EncError(JNI_EXCEPTION),
    EncError(ALLOCATION_FAILED));
#undef EncError

struct NdkSupport::ImgDecoder
{
    common::DynLib Lib;
    DECLARE_FUNC(DecFromBuffer);
    DECLARE_FUNC(ReleaseDec);
    DECLARE_FUNC(GetInfo);
    DECLARE_FUNC(GetWidth);
    DECLARE_FUNC(GetHeight);
    DECLARE_FUNC(GetFormat);
    DECLARE_FUNC(GetStride);
    DECLARE_FUNC(DecodeImg);
    DECLARE_FUNC(EncodeImg);
    DECLARE_FUNC(RetcodeStr);
    ImgDecoder() : Lib("libjnigraphics.so") 
    {
        LOAD_FUNC(DecFromBuffer);
        LOAD_FUNC(ReleaseDec);
        LOAD_FUNC(GetInfo);
        LOAD_FUNC(GetWidth);
        LOAD_FUNC(GetHeight);
        LOAD_FUNC(GetFormat);
        LOAD_FUNC(GetStride);
        LOAD_FUNC(DecodeImg);
        LOAD_FUNC(EncodeImg);
        TrLd_FUNC(RetcodeStr);
    }
    [[nodiscard]] std::string DecodeErrorString(int32_t err) const noexcept
    {
        if (RetcodeStr)
            RetcodeStr(err);
        return std::string(DecoderErrorLookup(err).value_or("Unknown Error"sv));
    }
    [[nodiscard]] std::string EncodeErrorString(int32_t err) const noexcept
    {
        return std::string(EncoderErrorLookup(err).value_or("Unknown Error"sv));
    }
};

#define THROW_EVAL(host, suc, type, eval, msg) do {                         \
if (const auto ret___ = host->eval; ret___ != suc)                          \
{                                                                           \
    ImgLog().Warning(msg ".\n");                                            \
    COMMON_THROWEX(common::BaseException, msg).Attach("errcode", ret___)    \
        .Attach("detail", host->type##ErrorString(ret___));                 \
} } while(0)

#define THROW_DEC(host, eval, msg) THROW_EVAL(host, ANDROID_IMAGE_DECODER_SUCCESS, Decode, eval, msg)
#define THROW_ENC(host, eval, msg) THROW_EVAL(host, ANDROID_BITMAP_RESULT_SUCCESS, Encode, eval, msg)


NdkReader::NdkReader(std::shared_ptr<const NdkSupport>&& support, common::AlignedBuffer&& src, void* decoder) noexcept : 
    Support(std::move(support)), Source(std::move(src)), Decoder(decoder) {}
NdkReader::~NdkReader() 
{
    Support->Host->ReleaseDec(reinterpret_cast<AImageDecoder*>(Decoder));
    Decoder = nullptr;
}

bool NdkReader::Validate()
{
    const auto decoder = reinterpret_cast<AImageDecoder*>(Decoder);
    const auto info = Support->Host->GetInfo(decoder);
    const auto width = Support->Host->GetWidth(info);
    const auto height = Support->Host->GetHeight(info);
    if (width < 0 || height < 0) return false;
    const auto format = Support->Host->GetFormat(info);
    switch (format)
    {
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
    case ANDROID_BITMAP_FORMAT_A_8:
    case ANDROID_BITMAP_FORMAT_RGBA_F16:
    case ANDROID_BITMAP_FORMAT_RGB_565:
        break;
    default:
        return false;
    }

    Width = width, Height = height;
    Format = format;
    return true;
}
Image NdkReader::Read(ImgDType dataType)
{
    const auto decoder = reinterpret_cast<AImageDecoder*>(Decoder);
    ImgDType origType;
    bool isGray = false;
    bool isFloat = false;
    switch (Format)
    {
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
        isGray = false; isFloat = false;
        origType = ImageDataType::RGBA;
        break;
    case ANDROID_BITMAP_FORMAT_A_8:
        isGray = true; isFloat = false;
        origType = ImageDataType::GRAY;
        break;
    case ANDROID_BITMAP_FORMAT_RGBA_F16:
        isGray = false; isFloat = true;
        origType = ImageDataType::RGBAh;
        break;
    case ANDROID_BITMAP_FORMAT_RGB_565:
        isGray = false; isFloat = false;
        origType = ImgDType{ dataType.Channel(), ImgDType::DataTypes::Uint8 };
        break;
    default: Expects(false); return {};
    }
    if (!isGray && dataType.ChannelCount() < 3)
        return {};
    if (isFloat != dataType.IsFloat())
        return {};

    Image image(origType);
    image.SetSize(Width, Height);

    const auto stride = Support->Host->GetStride(decoder);
    switch (Format)
    {
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
    case ANDROID_BITMAP_FORMAT_A_8:
    {
        // direct load
        Ensures(stride == image.GetElementSize() * image.GetWidth());
        THROW_DEC(Support->Host, DecodeImg(decoder, image.GetRawPtr(), stride, image.GetSize()), u"Failed to decode image");
    } break;
    case ANDROID_BITMAP_FORMAT_RGB_565:
    {
        Ensures(stride == 2 * image.GetWidth());
        common::AlignedBuffer buffer(stride * Height);
        THROW_DEC(Support->Host, DecodeImg(decoder, buffer.GetRawPtr(), stride, buffer.GetSize()), u"Failed to decode image");
        const auto& cvter = ColorConvertor::Get();
        if (dataType.HasAlpha())
        {
            if (!dataType.IsBGROrder())
                cvter.RGB565ToRGBA(image.GetRawPtr<uint32_t>(), buffer.GetRawPtr<uint16_t>(), Width * Height);
            else
                cvter.BGR565ToRGBA(image.GetRawPtr<uint32_t>(), buffer.GetRawPtr<uint16_t>(), Width * Height);
        }
        else
        {
            if (!dataType.IsBGROrder())
                cvter.RGB565ToRGB(image.GetRawPtr<uint8_t>(), buffer.GetRawPtr<uint16_t>(), Width * Height);
            else
                cvter.BGR565ToRGB(image.GetRawPtr<uint8_t>(), buffer.GetRawPtr<uint16_t>(), Width * Height);
        }
    } break;
    case ANDROID_BITMAP_FORMAT_RGBA_F16:
    {
        // FP16 -> FP32
        Ensures(stride == image.GetElementSize() * image.GetWidth());
        THROW_DEC(Support->Host, DecodeImg(decoder, image.GetRawPtr(), stride, image.GetSize()), u"Failed to decode image");
    } break;
    default: Expects(false); return {};
    }

    if (origType != dataType)
        image = image.ConvertTo(dataType);
    return image;
}


NdkWriter::NdkWriter(std::shared_ptr<const NdkSupport>&& support, common::io::RandomOutputStream& stream, int32_t format) noexcept :
    Support(std::move(support)), Stream(stream), Format(format) {}
NdkWriter::~NdkWriter()
{ }

static bool NdkWriterFunc(void* userContext, const void* data, size_t size)
{
    auto& stream = *reinterpret_cast<common::io::RandomOutputStream*>(userContext);
    return stream.WriteMany(size, 1, data) == size;
}

void NdkWriter::Write(const Image& image, const uint8_t quality)
{
    if (!image.GetDataType().Is(ImgDType::DataTypes::Uint8))
        return;
    ImageView view(image);
    AndroidBitmapFormat dataFormat = ANDROID_BITMAP_FORMAT_NONE;
    if (view.GetDataType() == ImageDataType::GRAY)
    {
        dataFormat = ANDROID_BITMAP_FORMAT_A_8;
    }
    else // convert to RGBA
    {
        dataFormat = ANDROID_BITMAP_FORMAT_RGBA_8888;
        view = view.ConvertTo(ImageDataType::RGBA);
    }

    AndroidBitmapInfo info
    {
        .width  = view.GetWidth(),
        .height = view.GetHeight(),
        .stride = view.GetWidth() * view.GetElementSize(),
        .format = dataFormat,
        .flags  = 0u,
    };
    THROW_ENC(Support->Host, EncodeImg(&info, ADATASPACE_UNKNOWN, view.GetRawPtr(), Format, quality, &Stream, &NdkWriterFunc), u"Failed to encode image");
}


NdkSupport::NdkSupport() : ImgSupport(u"NDK"), Host{std::make_unique<ImgDecoder>()}
{ }
NdkSupport::~NdkSupport() {}

std::unique_ptr<ImgReader> NdkSupport::GetReader(common::io::RandomInputStream& stream, std::u16string_view) const
{
    common::AlignedBuffer tmp;
    common::span<const std::byte> src;
    if (const auto space = stream.TryGetAvaliableInMemory(); space && space->size() == stream.GetSize())
    { // all in memory
        ImgLog().Verbose(u"NDK bypass Stream with mem region.\n");
        src = *space;
    }
    else
    {
        ImgLog().Verbose(u"NDK faces Non-MemoryStream, read all.\n");
        tmp = common::AlignedBuffer(stream.GetSize());
        stream.ReadMany(stream.GetSize(), 1, tmp.GetRawPtr());
        src = tmp.AsSpan();
    }
    AImageDecoder* decoder = nullptr;
    THROW_DEC(Host, DecFromBuffer(src.data(), src.size(), &decoder), u"Failed to create decoder");
    return std::make_unique<NdkReader>(shared_from_this(), std::move(tmp), decoder);
}

std::unique_ptr<ImgWriter> NdkSupport::GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const
{
    int32_t format = 0;
    if (ext == u"JPG" || ext == u"JPEG")
        format = ANDROID_BITMAP_COMPRESS_FORMAT_JPEG;
    else if (ext == u"PNG")
        format = ANDROID_BITMAP_COMPRESS_FORMAT_PNG;
    else if (ext == u"WEBP")
        format = ANDROID_BITMAP_COMPRESS_FORMAT_WEBP_LOSSY;
    else
        COMMON_THROWEX(BaseException, u"Unsupported file extension");
    return std::make_unique<NdkWriter>(shared_from_this(), stream, format);
}

uint8_t NdkSupport::MatchExtension(std::u16string_view ext, ImgDType datatype, const bool isRead) const
{
    if (isRead)
    {
        if (ext == u"JPG"sv || ext == u"JPEG"sv || ext == u"PNG"sv || ext == u"WEBP"sv || ext == u"BMP"sv || ext == u"HEIF"sv || ext == u"GIF"sv)
            return 128;
    }
    else
    {
        if (!datatype.Is(ImgDType::DataTypes::Uint8))
            return 0;
        if (ext == u"JPG"sv || ext == u"JPEG"sv || ext == u"PNG"sv || ext == u"WEBP"sv)
            return 128;
    }
    return 0;
}


static auto DUMMY = []()
{
    try
    {
        if (const auto ver = common::GetAndroidAPIVersion(); ver >= 30)
        {
            auto support = std::make_shared<NdkSupport>();
            common::mlog::LogInitMessage(common::mlog::LogLevel::Debug, "ImageNDK",
                common::str::Formatter<char>{}.FormatStatic(FmtString("Loaded jnigraphics with API[{}].\n"), ver));
            return RegistImageSupport(std::move(support));
        }
    }
    catch (const BaseException& be)
    {
        common::mlog::LogInitMessage(common::mlog::LogLevel::Warning, "ImageNDK",
            common::str::Formatter<char>{}.FormatStatic(FmtString("{}: {}.\n"), be.Message(), be.GetDetailMessage()));
    }
    return 0u;
}();

}