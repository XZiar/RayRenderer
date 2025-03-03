﻿#include "TestRely.h"
#include "SystemCommon/FileEx.h"
#include "common/TimeUtil.hpp"
#include "ImageUtil/ImageUtil.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4996)
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "3rdParty/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdParty/stb/stb_image_write.h"
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


using namespace common::mlog;
using namespace common;
namespace img = xziar::img;
using common::file::FileException;
using common::file::FileErrReason;
using std::vector;
using std::tuple;
using std::byte;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"ImgUtilTest", { GetConsoleBackend() });
    return log;
}

static img::Image STBLoadImage(const std::vector<byte>& fdata)
{
    int width, height, comp;
    auto ret = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(fdata.data()), static_cast<int>(fdata.size()),
        &width, &height, &comp, 4);
    if (ret == nullptr)
        COMMON_THROW(BaseException, u"cannot parse image");

    img::Image img(xziar::img::ImageDataType::RGBA);
    img.SetSize(width, height);
    memcpy_s(img.GetRawPtr(), img.GetSize(), ret, width * height * 4);
    stbi_image_free(ret);
    return img;
}

void writeToFile(void* context, void* data, int size)
{
    auto& stream = *static_cast<common::file::FileOutputStream*>(context);
    stream.Write(size, data);
}

static void STBSaveImage(const common::fs::path& fpath, const img::Image& img)
{
    auto stream = file::FileOutputStream(file::FileObject::OpenThrow(fpath, file::OpenFlag::CreateNewBinary));
    const auto ret = stbi_write_png_to_func(&writeToFile, &stream, 
        (int)img.GetWidth(), (int)img.GetHeight(), img.GetElementSize(), img.GetRawPtr(), 0);
    if (ret == 0)
        COMMON_THROW(FileException, FileErrReason::WriteFail, fpath, u"cannot parse image");
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
        log().Info(u"Test TGA16-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"tga", img::ImageDataType::RGB);
        timer.Stop();
        log().Debug(u"zextga read cost {} ms\n", timer.ElapseMs());
        STBSaveImage(basePath / u"CTC16-stb.png", img);
    }
    {
        const fs::path srcPath = basePath / u"pngtest.png";
        log().Info(u"Test PNG(Alpha)-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);

        timer.Start();
        auto img = img::ReadImage(fdata, u"png");
        timer.Stop();
        log().Debug(u"libpng read cost {} ms\n", timer.ElapseMs());

        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbpng read cost {} ms\n", timer.ElapseMs());

        log().Info(u"Test PNG(Alpha)-Writing\n");
        timer.Start();
        STBSaveImage(basePath / u"pngtest-stb.png", img);
        timer.Stop();
        log().Debug(u"stbpng write cost {} ms\n", timer.ElapseMs());

        log().Info(u"Test Gray-Writing\n");
        const auto img3 = keepGray(img);
        timer.Start();
        img::WriteImage(img3, basePath / u"pngtest-graya-lpng.png");
        timer.Stop();
        log().Debug(u"libpng write cost {} ms\n", timer.ElapseMs());

        const auto img4 = img3.ConvertTo(img::ImageDataType::GRAY);
        timer.Start();
        img::WriteImage(img4, basePath / u"pngtest-gray-zex.tga");
        timer.Stop();
        log().Debug(u"zextga write cost {} ms\n", timer.ElapseMs());

        log().Info(u"Test Gray-Reading\n");
        auto img5 = img::ReadImage(basePath / u"pngtest-gray-zex.tga");
        timer.Start();
        img::WriteImage(img5, basePath / u"pngtest-gray-lpng.png");
        timer.Stop();
        log().Debug(u"libpng write cost {} ms\n", timer.ElapseMs());
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw11.png";
        log().Info(u"Test PNG-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);

        timer.Start();
        auto img = img::ReadImage(fdata, u"png");
        timer.Stop();
        log().Debug(u"libpng read cost {} ms\n", timer.ElapseMs());

        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbpng read cost {} ms\n", timer.ElapseMs());

        log().Info(u"Test PNG-Writing\n");
        timer.Start();
        STBSaveImage(basePath / u"qw11-stb.png", img);
        timer.Stop();
        log().Debug(u"stbpng write cost {} ms\n", timer.ElapseMs());

        timer.Start();
        img::WriteImage(img, basePath / u"qw11-lpng.png");
        timer.Stop();
        log().Debug(u"libpng write cost {} ms\n", timer.ElapseMs());
    }
    {
        const fs::path srcPath = basePath / u"head2.tga";
        log().Info(u"Test TGA-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbtga read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img2, basePath / u"head2-stb.bmp");

        timer.Start();
        auto img = img::ReadImage(fdata, u"tga");
        timer.Stop();
        log().Debug(u"zextga read cost {} ms\n", timer.ElapseMs());
        file::FileOutputStream(file::FileObject::OpenThrow(basePath / "head-raw.dat", file::OpenFlag::CreateNewBinary))
            .Write(img.GetSize(), img.GetRawPtr<uint8_t>());
        img::WriteImage(img, basePath / u"head2-zex.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22.jpg";
        log().Info(u"Test JPG-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = xziar::img::ReadImage(fdata, u"jpg");
        timer.Stop();
        log().Debug(u"libjpeg read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22-ljpg.png");

        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbjpg read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22-stb.png");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22b8.bmp";
        log().Info(u"Test BMP-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"bmp");
        timer.Stop();
        log().Debug(u"zexbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22b8-zex.png");

        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img2, basePath / u"qw22b8-stb.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"qw22b.bmp";
        log().Info(u"Test BMP-Reading\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        timer.Start();
        auto img = img::ReadImage(fdata, u"bmp");
        timer.Stop();
        log().Debug(u"zexbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img, basePath / u"qw22b-zex.png");

        timer.Start();
        auto img2 = STBLoadImage(fdata);
        timer.Stop();
        log().Debug(u"stbbmp read cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img2, basePath / u"qw22b-stb.bmp");
    }
    //if (false)
    {
        const fs::path srcPath = basePath / u"head.tga";
        log().Info(u"Test Image-Conversion\n");
        const auto fdata = file::ReadAll<std::byte>(srcPath);
        auto img = img::ReadImage(fdata, u"tga", img::ImageDataType::BGRA);
        auto imgs = img::Image(img::ImageDataType::RGB);
        imgs.SetSize(img.GetWidth() * 2, img.GetHeight() * 2);
        timer.Start();
        auto img2 = img.ConvertTo(img::ImageDataType::BGR);
        timer.Stop();
        log().Debug(u"convert RGBA to BGR cost {} ms\n", timer.ElapseMs());
        img::WriteImage(img2, basePath / u"head-BGR.bmp");

        timer.Start();
        imgs.PlaceImage(img2, 0, 0, 0, 0);
        timer.Stop();
        log().Debug(u"place without convert cost {} ms\n", timer.ElapseMs());
        timer.Start();
        img2.FlipHorizontal();
        timer.Stop();
        log().Debug(u"flip horizontal(3) cost {} ms\n", timer.ElapseMs());
        imgs.PlaceImage(img2, 0, 0, img.GetWidth(), 0);
        timer.Start();
        img.FlipHorizontal();
        timer.Stop();
        log().Debug(u"flip horizontal(4) cost {} ms\n", timer.ElapseMs());
        timer.Start();
        imgs.PlaceImage(img, 0, 0, 0, img.GetHeight());
        timer.Stop();
        log().Debug(u"place with convert cost {} ms\n", timer.ElapseMs());

        img::WriteImage(imgs, basePath / u"head-4.jpg");
    }
    log().Success(u"Test ImageUtil over!\n");
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
    log().Success(u"Test diff show img over!\n");
}

static void ImgUtilTest()
{
    TestImageUtil();
    //TestDiffShow();
    getchar();
}

const static uint32_t ID = RegistTest("ImgUtilTest", &ImgUtilTest);
