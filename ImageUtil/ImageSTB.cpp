#include "ImageUtilRely.h"
#include "ImageSTB.h"
#include "DataConvertor.hpp"


#define STBI_NO_JPEG
#define STBI_NO_PNG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#if defined(COMPILER_GCC) && COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(COMPILER_CLANG) && COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-function"
#elif defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4505 4100)
#endif
#include "3rdParty/stblib/stb_image.h"
#include "3rdParty/stblib/stb_image_write.h"
#if defined(COMPILER_GCC) && COMPILER_GCC
#   pragma GCC diagnostic pop
#elif defined(COMPILER_CLANG) && COMPILER_CLANG
#   pragma clang diagnostic pop
#elif defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(pop)
#endif


namespace xziar::img::stb
{

static int ReadFile(void *user, char *data, int size)
{
    auto& file = *static_cast<FileObject*>(user);
    return (int)file.ReadMany(size, data);
}
static void SkipFile(void *user, int n)
{
    auto& file = *static_cast<FileObject*>(user);
    file.Rewind(file.CurrentPos() + n);
}
static int IsEof(void *user)
{
    auto& file = *static_cast<FileObject*>(user);
    return file.IsEnd() ? 1 : 0;
}


StbReader::StbReader(FileObject& file) : ImgFile(file)
{
    auto callback = new stbi_io_callbacks{ ReadFile, SkipFile, IsEof };
    StbCallback = callback;
    auto context = new stbi__context();
    StbContext = context;
    stbi__start_callbacks(context, callback, &ImgFile);
}

StbReader::~StbReader()
{
    if (!StbCallback)
        delete static_cast<stbi_io_callbacks*>(StbCallback);
    if (!StbContext)
        delete static_cast<stbi__context*>(StbContext);
    if (!StbResult)
        delete static_cast<stbi__result_info*>(StbResult);
}

bool StbReader::Validate()
{
    auto ri = new stbi__result_info();
    StbResult = ri;
    memset(ri, 0, sizeof(stbi__result_info)); // make sure it's initialized if we add new fields
    ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
    ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
    ri->num_channels = 0;
    return stbi__pnm_test(static_cast<stbi__context*>(StbContext));
}

Image StbReader::Read(const ImageDataType dataType)
{
    const int reqComp = Image::GetElementSize(dataType);
    int width, height, comp;
    auto ret = stbi__pnm_load(static_cast<stbi__context*>(StbContext), &width, &height, &comp, reqComp, static_cast<stbi__result_info*>(StbResult));
    if (ret == nullptr)
        COMMON_THROW(BaseException, u"cannot parse image");
    ImageDataType retType;
    switch (reqComp)
    {
    case 1: retType = ImageDataType::GRAY; break;
    case 2: retType = ImageDataType::GA; break;
    case 3: retType = ImageDataType::RGB; break;
    case 4: retType = ImageDataType::RGBA; break;
    default: COMMON_THROW(BaseException, u"cannot parse image");
    }
    Image img(retType);
    img.SetSize(width, height);
    memcpy_s(img.GetRawPtr(), comp * width * height, ret, width * height * comp);
    stbi_image_free(ret);
    if (retType != dataType)
        img = img.ConvertTo(dataType);
    return img;
}


StbWriter::StbWriter(FileObject& file) : ImgFile(file)
{
}

StbWriter::~StbWriter()
{
    
}

void StbWriter::Write(const Image&)
{
    
}


static auto DUMMY = RegistImageSupport<StbSupport>();

}