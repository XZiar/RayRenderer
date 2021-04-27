#pragma once
#include "TexUtilRely.h"

namespace oglu::texutil
{

TEXUTILAPI common::AlignedBuffer CompressToDat(const xziar::img::ImageView& img, const xziar::img::TextureFormat format, const bool needAlpha = true);


}
