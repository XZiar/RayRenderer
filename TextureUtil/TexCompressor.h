#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{

common::AlignedBuffer TEXUTILAPI CompressToDat(const xziar::img::ImageView& img, const xziar::img::TextureFormat format, const bool needAlpha = true);


}
