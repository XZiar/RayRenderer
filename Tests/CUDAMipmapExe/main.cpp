#include "common/CommonRely.hpp"
#include "ImageUtil/ImageUtil.h"
#include "Tests/CUDAMipmap/CUDAMipmap.h"

using namespace xziar::img;

int main()
{
    Image src(ImageDataType::RGBA);
    const auto srcPath = fs::temp_directory_path() / u"src.png";
    if (fs::exists(srcPath))
        src = ReadImage(srcPath);
    else
    {
        src.SetSize(256, 256);
        for (uint32_t i = 0; i < 256 * 256; ++i)
            src.GetRawPtr<uint32_t>()[i] = i | 0xff000000u;
    }
    Image dst(ImageDataType::RGBA); dst.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);

    DoMipmap(src.GetRawPtr(), dst.GetRawPtr(), src.GetWidth(), src.GetHeight());



    WriteImage(dst, fs::temp_directory_path() / u"dst.png");

    getchar();
}