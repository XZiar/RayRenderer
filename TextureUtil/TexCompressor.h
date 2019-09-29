#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{

common::AlignedBuffer TEXUTILAPI CompressToDat(const ImageView& img, const TextureFormat format, const bool needAlpha = true);


}
