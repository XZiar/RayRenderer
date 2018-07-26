#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"

using namespace System;
using System::Windows::Media::Imaging::BitmapSource;


namespace XZiar
{
namespace Img
{

public ref class ImageUtil
{
public:
    static BitmapSource^ ReadImage(String^ filePath);
    static void WriteImage(BitmapSource^ image, String^ filePath);

    static BitmapSource^ Convert(const xziar::img::Image& image);
    static xziar::img::Image Convert(BitmapSource^ image);
};


}
}

