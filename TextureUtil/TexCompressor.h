#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{

common::AlignedBuffer TEXUTILAPI CompressToDat(const Image& img, const TextureInnerFormat format, const bool needAlpha = true);


}
