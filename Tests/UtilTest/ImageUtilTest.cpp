#include "TestRely.h"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "ImageUtil/ImageUtil.h"
#include "stblib/stblib.h"

using namespace common::mlog;
using namespace common;
namespace img = xziar::img;
using std::vector;
using std::tuple;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"ImgUtilTest", { GetConsoleBackend() });
    return log;
}

static img::Image stbToImage(const vector<uint32_t>& stbimg, const tuple<int32_t, int32_t> size)
{
    img::Image img(xziar::img::ImageDataType::RGBA);
    auto [width, height] = size;
    img.SetSize(width, height);
    memcpy(img.GetRawPtr(), stbimg.data(), img.GetSize());
    return img;
}

static img::Image keepGray(const img::Image& src)
{
    img::Image ret(img::ImageDataType::GA);
    ret.SetSize(src.GetWidth(), src.GetHeight());
    const auto step = src.GetElementSize();
    auto srcPtr = src.GetRawPtr(); auto destPtr = ret.GetRawPtr();
    for (auto cnt = src.PixelCount(); cnt--; srcPtr += step)
    {
        *destPtr++ = *srcPtr; *destPtr++ = srcPtr[step - 1];
    }
    return ret;
}

static void TestImageUtil()
{
    const fs::path basePath = FindPath() / u"Tests" / u"Data";
    SimpleTimer timer;
    {
        const fs::path srcPath = basePath / u"CTC16.TGA";
        log().info(u"Test TGA16-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"tga", img::ImageDataType::RGB);
        timer.Stop();
        log().debug(u"zextga read cost {} ms\n", timer.ElapseMs());
        ::stb::saveImage(basePath / u"CTC16-stb.png", img.GetRawPtr(), img.GetWidth(), img.GetHeight(), img.GetElementSize());
    }
    {
        const fs::path srcPath = basePath / u"pngtest.png";
        log().info(u"Test PNG(Alpha)-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"png");
        timer.Stop();
        log().debug(u"libpng read cost {} ms\n", timer.ElapseMs());

        std::vector<uint32_t> data;
        timer.Start();
        auto img2 = ::stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbpng read cost {} ms\n", timer.ElapseMs());

        log().info(u"Test PNG(Alpha)-Writing\n");
        auto size = img.GetWidth() * img.GetHeight() + data.size();
        timer.Start();
        ::stb::saveImage(basePath / u"pngtest-stb.png", img.GetRawPtr(), img.GetWidth(), img.GetHeight(), img.GetElementSize());
        timer.Stop();
        log().debug(u"stbpng write cost {} ms\n", timer.ElapseMs());

        log().info(u"Test Gray-Writing\n");
        const auto img3 = keepGray(img);
        timer.Start();
        img::WriteImage(img3, basePath / u"pngtest-graya-lpng.png");
        timer.Stop();
        log().debug(u"libpng write cost {} ms\n", timer.ElapseMs());

        const auto img4 = img3.ConvertTo(img::ImageDataType::GRAY);
        timer.Start();
        img::WriteImage(img4, basePath / u"pngtest-gray-zex.tga");
        timer.Stop();
        log().debug(u"zextga write cost {} ms\n", timer.ElapseMs());

        log().info(u"Test Gray-Reading\n");
        auto img5 = img::ReadImage(basePath / u"pngtest-gray-zex.tga");
        timer.Start();
        img::WriteImage(img5, basePath / u"pngtest-gray-lpng.png");
        timer.Stop();
        log().debug(u"libpng write cost {} ms\n", timer.ElapseMs());
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw11.png";
        log().info(u"Test PNG-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"png");
        timer.Stop();
        log().debug(u"libpng read cost {} ms\n", timer.ElapseMs());
        std::vector<uint32_t> data;
        timer.Start();
        auto img2 = ::stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbpng read cost {} ms\n", timer.ElapseMs());
        auto size = img.GetWidth() * img.GetHeight() + data.size();
        log().info(u"Test PNG-Writing\n");
        timer.Start();
        ::stb::saveImage(basePath / u"qw11-stb.png", img.GetRawPtr(), img.GetWidth(), img.GetHeight(), img.GetElementSize());
        timer.Stop();
        log().debug(u"stbpng write cost {} ms\n", timer.ElapseMs());
        timer.Start();
        img::WriteImage(img, basePath / u"qw11-lpng.png");
        timer.Stop();
        log().debug(u"libpng write cost {} ms\n", timer.ElapseMs());
    }
    {
        const fs::path srcPath = basePath / u"head2.tga";
        log().info(u"Test TGA-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        std::vector<uint32_t> data;
        timer.Start();
        auto size = stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbtga read cost {} ms\n", timer.ElapseMs());
        auto img2 = stbToImage(data, size);
        img::WriteImage(img2, basePath / u"head2-stb.bmp");

        timer.Start();
        auto img = img::ReadImage(fdata, u"tga");
        timer.Stop();
        log().debug(u"zextga read cost {} ms\n", timer.ElapseMs());
        file::FileOutputStream(file::FileObject::OpenThrow(basePath / "head-raw.dat", file::OpenFlag::CreatNewBinary))
            .Write(img.GetSize(), img.GetRawPtr<uint8_t>());
        img::WriteImage(img, basePath / u"head2-zex.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22.jpg";
        log().info(u"Test JPG-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = xziar::img::ReadImage(fdata, u"jpg");
        timer.Stop();
        log().debug(u"libjpeg read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22-ljpg.png");

        std::vector<uint32_t> data;
        timer.Start();
        auto size = stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbjpg read cost {} ms\n", timer.ElapseMs());
        const auto img2 = stbToImage(data, size);
        img::WriteImage(img, basePath / u"qw22-stb.png");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22b8.bmp";
        log().info(u"Test BMP-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"bmp");
        timer.Stop();
        log().debug(u"zexbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22b8-zex.png");

        std::vector<uint32_t> data;
        timer.Start();
        auto size = stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbbmp read cost {} ms\n", timer.ElapseMs());
        const auto img2 = stbToImage(data, size);
        img::WriteImage(img2, basePath / u"qw22b8-stb.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22b.bmp";
        log().info(u"Test BMP-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"bmp");
        timer.Stop();
        log().debug(u"zexbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22b-zex.png");

        std::vector<uint32_t> data;
        timer.Start();
        auto size = stb::loadImage(srcPath, data);
        timer.Stop();
        log().debug(u"stbbmp read cost {} ms\n", timer.ElapseMs());
        auto img2 = stbToImage(data, size);
        img::WriteImage(img2, basePath / u"qw22b-stb.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"head.tga";
        log().info(u"Test Image-Conversion\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        auto img = img::ReadImage(fdata, u"tga", img::ImageDataType::BGRA);
        auto imgs = img::Image(img::ImageDataType::RGB);
        imgs.SetSize(img.GetWidth() * 2, img.GetHeight() * 2);
        timer.Start();
        auto img2 = img.ConvertTo(img::ImageDataType::BGR);
        timer.Stop();
        log().debug(u"convert RGBA to BGR cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img2, basePath / u"head-BGR.bmp");

        timer.Start();
        imgs.PlaceImage(img2, 0, 0, 0, 0);
        timer.Stop();
        log().debug(u"place without convert cost {} ms\n", timer.ElapseMs());
        timer.Start();
        img2.FlipHorizontal();
        timer.Stop();
        log().debug(u"flip horizontal(3) cost {} ms\n", timer.ElapseMs());
        imgs.PlaceImage(img2, 0, 0, img.GetWidth(), 0);
        timer.Start();
        img.FlipHorizontal();
        timer.Stop();
        log().debug(u"flip horizontal(4) cost {} ms\n", timer.ElapseMs());
        timer.Start();
        imgs.PlaceImage(img, 0, 0, 0, img.GetHeight());
        timer.Stop();
        log().debug(u"place with convert cost {} ms\n", timer.ElapseMs());

        img::WriteImage(imgs, basePath / u"head-4.jpg");
    }
    log().success(u"Test ImageUtil over!\n");
}

static void TestDiffShow()
{
    const fs::path srcPath1 = u"D:/ProgramsTemps/RayRenderer/ShowNV.png";
    const fs::path srcPath2 = u"D:/ProgramsTemps/RayRenderer/Show.png";
    const fs::path srcPath3 = u"D:/ProgramsTemps/RayRenderer/Show-diff.bmp";
    const auto img1 = img::ReadImage(srcPath1, img::ImageDataType::GRAY);
    const auto img2 = img::ReadImage(srcPath2, img::ImageDataType::GRAY);
    img::Image img3(img::ImageDataType::GRAY);
    img3.SetSize(img2.GetWidth(), img2.GetHeight());
    auto p1 = img1.GetRawPtr<uint8_t>();
    auto p2 = img2.GetRawPtr<uint8_t>();
    auto p3 = img3.GetRawPtr<uint8_t>();
    for (auto size = img3.PixelCount(); size--;)
        *p3++ = (uint8_t)std::clamp(((*p1++) - (*p2++)) * 8 + 64, 0, 255);
    img::WriteImage(img3, srcPath3);
    log().success(u"Test diff show img over!\n");
}

static void TestFloat()
{
    img::Image gfimg(img::ImageDataType::GRAYf);
    gfimg.SetSize(16, 16);
    float* imgPtr = gfimg.GetRawPtr<float>();
    for (uint32_t i = 0; i < 256; ++i)
        imgPtr[i] = (float)i;
    auto gimg = gfimg.ConvertToFloat(255);
    auto newgfimg = gimg.ConvertToFloat(255);
    auto newgimg = newgfimg.ConvertToFloat(255);
}

static void ImgUtilTest()
{
    TestImageUtil();
    //TestDiffShow();
    TestFloat();
    getchar();
}

const static uint32_t ID = RegistTest("ImgUtilTest", &ImgUtilTest);
