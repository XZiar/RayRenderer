#pragma once

#include "RenderCoreWrapRely.h"

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

private ref class ImageHolder
{
private:
    Common::NativeWrapper<xziar::img::ImageView> Img;
    //CLIWrapper<xziar::img::ImageView> Img;
public:
    ImageHolder(xziar::img::Image&& img);
    ImageHolder(const xziar::img::Image& img);
    ~ImageHolder() { this->!ImageHolder(); }
    !ImageHolder();
    void Save(String^ filePath);
};


}
}

