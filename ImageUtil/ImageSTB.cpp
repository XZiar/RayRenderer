#include "ImageUtilPch.h"
#include "ImageSTB.h"


#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_THREADLOCAL_FAILMSG
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-function"
#elif COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4996)
#endif
#include "3rdParty/stb/stb_image.h"
#include "3rdParty/stb/stb_image_write.h"
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

#if defined(STBI_SSE2)
#pragma message("Compiling ImageSTB with support of [SSE2]")
#endif


namespace xziar::img::stb
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


static int ReadStream(void *user, char *data, int size)
{
    auto& stream = *static_cast<RandomInputStream*>(user);
    return (int)stream.ReadMany(size, 1, data);
}
static void SkipStream(void *user, int n)
{
    auto& stream = *static_cast<RandomInputStream*>(user);
    stream.SetPos(stream.CurrentPos() + n); // n can be negative
}
static int IsEof(void *user)
{
    auto& stream = *static_cast<RandomInputStream*>(user);
    return stream.IsEnd() ? 1 : 0;
}


struct StbReader::Context : public stbi__context
{ };
StbReader::StbReader(RandomInputStream& stream) : Stream(stream), StbContext(std::make_unique<Context>())
{
    if (const auto space = Stream.TryGetAvaliableInMemory(); space && space->size() == Stream.GetSize() && space->size() <= std::numeric_limits<int>::max())
    { // all in memory and within intmax
        ImgLog().Debug(u"STB bypass Stream with mem region.\n");
        stbi__start_mem(StbContext.get(), reinterpret_cast<const unsigned char*>(space->data()), static_cast<int>(space->size()));
    }
    else
    {
        stbi_io_callbacks callBacks{ ReadStream, SkipStream, IsEof }; 
        stbi__start_callbacks(StbContext.get(), &callBacks, &Stream);
    }
}

StbReader::~StbReader()
{
}

static forceinline std::optional<ImgDType::Channels> CompToDType(const int comp) noexcept
{
    if (comp < 1 || comp > 4)
        return {};
    constexpr ImgDType::Channels Channels[4] = { ImgDType::Channels::R, ImgDType::Channels::RA, ImgDType::Channels::RGB, ImgDType::Channels::RGBA };
    return Channels[comp - 1];
}

bool StbReader::Validate()
{
    auto context = StbContext.get();
    int w = 0, h = 0, comp = 0;
    bool infoCheck = false, is16Bit = false;
    if (stbi__pnm_test(context))
    {
        TestedType = ImgType::PNM;
        infoCheck = stbi__pnm_info(context, &w, &h, &comp);
        is16Bit = stbi__png_is16(context);
    }
    else if (stbi__jpeg_test(context))
    {
        TestedType = ImgType::JPG;
        infoCheck = stbi__jpeg_info(context, &w, &h, &comp);
    }
    else if (stbi__png_test(context))
    {
        TestedType = ImgType::PNG;
        infoCheck = stbi__png_info(context, &w, &h, &comp);
        is16Bit = stbi__png_is16(context);
    }
    else if (stbi__bmp_test(context))
    {
        return false; // skip due to known issue https://github.com/nothings/stb/issues/1716
        // TestedType = ImgType::BMP;
    }
    else if (stbi__pic_test(context))
    {
        TestedType = ImgType::PIC;
        infoCheck = stbi__pic_info(context, &w, &h, &comp);
    }
    else if (stbi__tga_test(context))
    {
        TestedType = ImgType::TGA;
        infoCheck = stbi__tga_info(context, &w, &h, &comp);
    }
    else
        return false;
    if (!infoCheck || w <= 0 || h <= 0 || comp < 1 || comp > 4)
        return false;
    TestedDType = ImgDType{ *CompToDType(comp), is16Bit ? ImgDType::DataTypes::Uint16 : ImgDType::DataTypes::Uint8 };
    stbi__rewind(context); // looks like xxx_info does not rewind
    return true;
}

struct StbDataHolder final : public common::AlignedBuffer::ExternBufInfo
{
    void* Ptr = nullptr;
    size_t Size = 0;
    ~StbDataHolder() final
    {
        if (Ptr)
            stbi_image_free(Ptr);
    }
    [[nodiscard]] size_t GetSize() const noexcept final { return Size; }
    [[nodiscard]] std::byte* GetPtr() const noexcept final { return reinterpret_cast<std::byte*>(Ptr); }
};

Image StbReader::Read(ImgDType dataType)
{
    Expects(TestedDType);
    if (dataType)
    {
        if (!dataType.Is(ImgDType::DataTypes::Uint8))
            COMMON_THROW(BaseException, u"unsupported datatype");
    }
    else
        dataType = TestedDType;
    //const int32_t reqComp = Image::GetElementSize(dataType);
    int width = 0, height = 0, comp = 0;
    stbi__result_info resInfo;
    memset(&resInfo, 0, sizeof(stbi__result_info)); // make sure it's initialized if we add new fields
    resInfo.bits_per_channel = 8; // default is 8 so most paths don't have to be changed
    resInfo.channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
    resInfo.num_channels = 0;

    auto context = StbContext.get();
    auto stbret = std::make_unique<StbDataHolder>();
    switch (TestedType)
    {
    case ImgType::PNM:  stbret->Ptr = stbi__pnm_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::JPG:  stbret->Ptr = stbi__jpeg_load(context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::PNG:  stbret->Ptr = stbi__png_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::BMP:  stbret->Ptr = stbi__bmp_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::PIC:  stbret->Ptr = stbi__pic_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::TGA:  stbret->Ptr = stbi__tga_load (context, &width, &height, &comp, 0, &resInfo); break;
    default:            COMMON_THROW(BaseException, u"unvalidated image");
    }

    if (stbret->Ptr == nullptr)
        COMMON_THROW(BaseException, common::str::to_u16string(stbi_failure_reason()));
    if (width <= 0 || height <= 0)
        COMMON_THROW(BaseException, u"Image load by stb has unexpected shape");
    if (resInfo.bits_per_channel != 8 && resInfo.bits_per_channel != 16)
        COMMON_THROW(BaseException, u"Image load by stb has unexpected bits per channel");
    const auto retCh = CompToDType(comp);
    if (!retCh)
        COMMON_THROW(BaseException, u"cannot parse image");

    ImgDType retType{ *retCh, resInfo.bits_per_channel == 16 ? ImgDType::DataTypes::Uint16 : ImgDType::DataTypes::Uint8 };
    if (retType != TestedDType)
        ImgLog().Warning(u"stb loads [{}] image rather than [{}] during validate\n", retType, TestedDType);
    stbret->Size = static_cast<size_t>(width) * height * retType.ElementSize();
    Image img(common::AlignedBuffer::CreateBuffer(std::move(stbret)), static_cast<uint32_t>(width), static_cast<uint32_t>(height), retType);

    if (retType != dataType)
        return img.ConvertTo(dataType);
    return img;
}


StbWriter::StbWriter(RandomOutputStream& stream, std::u16string_view ext) : Stream(stream)
{
    if (ext == u"PNG")      
        TargetType = ImgType::PNG;
    else if (ext == u"BMP") 
        TargetType = ImgType::BMP;
    else if (ext == u"TGA") 
        TargetType = ImgType::TGA;
    else if (ext == u"JPG" || ext == u"JPEG") 
        TargetType = ImgType::JPG;
    else 
        COMMON_THROW(BaseException, u"unsupported image type");
}

StbWriter::~StbWriter()
{
    
}

static void WriteToFile(void *context, void *data, int size)
{
    auto& stream = *static_cast<RandomOutputStream*>(context);
    stream.Write(size, data);
}

void StbWriter::Write(ImageView image, const uint8_t quality)
{
    const auto origType = image.GetDataType();
    if (!origType.Is(ImgDType::DataTypes::Uint8))
        return;
    if (origType.IsBGROrder()) // STB always writes RGB order
        image = image.ConvertTo(ImgDType{ origType.HasAlpha() ? ImgDType::Channels::RGBA : ImgDType::Channels::RGB , origType.DataType()});

    const auto width = static_cast<int32_t>(image.GetWidth()), height = static_cast<int32_t>(image.GetHeight());
    const int32_t reqComp = Image::GetElementSize(image.GetDataType());
    int32_t ret = 0; 
    switch (TargetType)
    {
    case ImgType::BMP:  ret = stbi_write_bmp_to_func(&WriteToFile, &Stream, width, height, reqComp, image.GetRawPtr()); break;
    case ImgType::PNG:  ret = stbi_write_png_to_func(&WriteToFile, &Stream, width, height, reqComp, image.GetRawPtr(), 0); break;
    case ImgType::TGA:  ret = stbi_write_tga_to_func(&WriteToFile, &Stream, width, height, reqComp, image.GetRawPtr()); break;
    case ImgType::JPG:  ret = stbi_write_jpg_to_func(&WriteToFile, &Stream, width, height, reqComp, image.GetRawPtr(), quality); break;
    default:            COMMON_THROW(BaseException, u"unsupported image type");
    }
    if (ret == 0)
        COMMON_THROW(BaseException, u"cannot write image");
}


uint8_t StbSupport::MatchExtension(std::u16string_view ext, ImgDType datatype, const bool isRead) const
{
    if (isRead)
    {
        if (datatype && !datatype.Is(ImgDType::DataTypes::Uint8) && !datatype.Is(ImgDType::DataTypes::Uint16))
            return 0;
        if (ext == u"PPM" || ext == u"PGM")
            return 240;
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG" || ext == u"BMP" || ext == u"PIC" || ext == u"TGA")
            return 96;
    }
    else
    {
        if (!datatype.Is(ImgDType::DataTypes::Uint8))
            return 0;
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG" || ext == u"BMP" || ext == u"TGA")
            return 128;
    }
    return 0;
}

static auto DUMMY = RegistImageSupport<StbSupport>();

}