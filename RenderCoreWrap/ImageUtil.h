#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.hpp"

using namespace System;
using System::Windows::Media::Imaging::BitmapSource;


namespace XZiar
{
namespace Img
{

public ref class ImageUtil
{
internal:
public:
    static BitmapSource^ ReadImage(String^ filePath);
    static void WriteImage(BitmapSource^ image, String^ filePath);
    static void WriteImage(const xziar::img::Image& image, String^ filePath);

    static BitmapSource^ Convert(const xziar::img::Image& image);
    static xziar::img::Image Convert(BitmapSource^ image);
};

private ref class ImageSaver
{
private:
    CLIWrapper<xziar::img::Image> Img;
public:
    ImageSaver(xziar::img::Image&& img) : Img(img) {}
    ImageSaver(const xziar::img::Image& img) : Img(img) {}
    void Save(String^ filePath);
};


}
}

