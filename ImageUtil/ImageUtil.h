#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "ImagePNG.h"

namespace xziar::img
{

IMGUTILAPI Image __cdecl ReadImage(const fs::path& path, const ImageDataType dataType = ImageDataType::RGBA);
IMGUTILAPI void __cdecl WriteImage(const Image& image, const fs::path& path);
IMGUTILAPI void __cdecl RegisterImageSupport();

}