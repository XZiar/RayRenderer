#pragma once

#include "ImageUtilRely.h"

namespace xziar::img
{

Image __cdecl ReadImage(const fs::path& path, const ImageDataType dataType = ImageDataType::RGBA);
void __cdecl WriteImage(const Image& image, const fs::path& path);
void __cdecl RegisterImageSupport();

}