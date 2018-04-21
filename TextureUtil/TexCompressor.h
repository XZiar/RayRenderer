#pragma once
#include "TexUtilRely.h"

namespace oglu::texcomp
{

common::AlignedBuffer<32> TEXCOMPAPI CompressToDat(const Image& img, const TextureInnerFormat format, const bool needAlpha = true);
common::PromiseResult<oglTex2DV> TEXCOMPAPI CompressToTex(const Image& img, const TextureInnerFormat format, const bool needAlpha = true);


}
