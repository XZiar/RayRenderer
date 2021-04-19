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
//#   pragma warning(disable:4505 4100)
#endif
#include "3rdParty/stb/stb_image.h"
#if !COMMON_COMPILER_MSVC
#   undef __STDC_WANT_SECURE_LIB__
#endif
#include "3rdParty/stb/stb_image_write.h"
#if !COMMON_COMPILER_MSVC
#   define __STDC_WANT_SECURE_LIB__ 1
#endif
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
static stbi_io_callbacks IOCallBack{ ReadStream, SkipStream, IsEof };

struct StbData
{
    void* Ptr = nullptr;
    ~StbData()
    {
        if (Ptr)
            stbi_image_free(Ptr);
    }
};


StbReader::StbReader(RandomInputStream& stream) : Stream(stream)
{
    auto context = new stbi__context();
    StbContext = context;
    if (auto memStream = dynamic_cast<common::io::MemoryInputStream*>(&Stream))
    {
        ImgLog().verbose(u"STB faces MemoryStream, bypass it.\n");
        const auto [ptr, size] = memStream->ExposeAvaliable();
        stbi__start_mem(context, reinterpret_cast<const unsigned char*>(ptr), static_cast<int>(size));
    }
    else
        stbi__start_callbacks(context, &IOCallBack, &Stream);
}

StbReader::~StbReader()
{
    if (!StbContext)
        delete static_cast<stbi__context*>(StbContext);
}

bool StbReader::Validate()
{
    auto context = static_cast<stbi__context*>(StbContext);
    if (stbi__pnm_test(context))
        TestedType = ImgType::PNM;
    else if (stbi__jpeg_test(context))
        TestedType = ImgType::JPG;
    else if (stbi__png_test(context))
        TestedType = ImgType::PNG;
    else if (stbi__bmp_test(context))
        TestedType = ImgType::BMP;
    else if (stbi__pic_test(context))
        TestedType = ImgType::PIC;
    else if (stbi__tga_test(context))
        TestedType = ImgType::TGA;
    else
        return false;
    return true;
}

Image StbReader::Read(const ImageDataType dataType)
{
    //const int32_t reqComp = Image::GetElementSize(dataType);
    int32_t width, height, comp;
    stbi__result_info resInfo;
    memset(&resInfo, 0, sizeof(stbi__result_info)); // make sure it's initialized if we add new fields
    resInfo.bits_per_channel = 8; // default is 8 so most paths don't have to be changed
    resInfo.channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
    resInfo.num_channels = 0;

    auto context = static_cast<stbi__context*>(StbContext);
    StbData ret;
    switch (TestedType)
    {
    case ImgType::PNM:  ret.Ptr = stbi__pnm_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::JPG:  ret.Ptr = stbi__jpeg_load(context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::PNG:  ret.Ptr = stbi__png_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::BMP:  ret.Ptr = stbi__bmp_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::PIC:  ret.Ptr = stbi__pic_load (context, &width, &height, &comp, 0, &resInfo); break;
    case ImgType::TGA:  ret.Ptr = stbi__tga_load (context, &width, &height, &comp, 0, &resInfo); break;
    default:            COMMON_THROW(BaseException, u"unvalidated image");
    }
    if (ret.Ptr == nullptr)
    {
        COMMON_THROW(BaseException, common::str::to_u16string(stbi_failure_reason()));
    }

    ImageDataType retType;
    switch (comp)
    {
    case 1: retType = ImageDataType::GRAY; break;
    case 2: retType = ImageDataType::GA; break;
    case 3: retType = ImageDataType::RGB; break;
    case 4: retType = ImageDataType::RGBA; break;
    default: COMMON_THROW(BaseException, u"cannot parse image");
    }
    Image img(retType);
    img.SetSize(width, height);

    if (resInfo.bits_per_channel != 8)
    {
        if (resInfo.bits_per_channel != 16)
            COMMON_THROW(BaseException, u"Image load by stb has unexpected bits per channel");
        const size_t pixCount = width * height * comp;
        const uint16_t* __restrict u16Data = reinterpret_cast<const uint16_t*>(ret.Ptr);
        uint8_t* __restrict u8Data = img.GetRawPtr<uint8_t>();
        for (size_t i = 0; i < pixCount; ++i)
            u8Data[i] = (u16Data[i] >> 8);
    }
    else
    {
        memcpy_s(img.GetRawPtr(), comp * width * height, ret.Ptr, width * height * comp);
    }

    if (retType != dataType)
        img = img.ConvertTo(dataType);
    return img;
}


StbWriter::StbWriter(RandomOutputStream& stream, const u16string& ext) : Stream(stream)
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

void StbWriter::Write(const Image& image, const uint8_t quality)
{
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


uint8_t StbSupport::MatchExtension(const u16string& ext, const ImageDataType, const bool IsRead) const
{
    if (IsRead)
    {
        if (ext == u"PPM" || ext == u"PGM")
            return 240;
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG" || ext == u"BMP" || ext == u"PIC" || ext == u"TGA")
            return 128;
    }
    else
    {
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG" || ext == u"BMP" || ext == u"TGA")
            return 128;
    }
    return 0;
}

static auto DUMMY = RegistImageSupport<StbSupport>();

}