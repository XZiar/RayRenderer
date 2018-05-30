#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "ImageCore.h"

namespace xziar::img
{

IMGUTILAPI Image CDECLCALL ReadImage(const fs::path& path, const ImageDataType dataType = ImageDataType::RGBA);
IMGUTILAPI void CDECLCALL WriteImage(const Image& image, const fs::path& path);
IMGUTILAPI void CDECLCALL RegisterImageSupport();

}