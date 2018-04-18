#pragma once
#include "TexCompRely.h"

namespace oglu::texcomp
{

common::AlignedBuffer<32> TEXCOMPAPI CompressToDat(const Image& img, const TextureInnerFormat format);
common::PromiseResult<oglTex2DV> TEXCOMPAPI CompressToTex(const Image& img, const TextureInnerFormat format);


}
