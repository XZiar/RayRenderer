#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;
using System::Windows::Media::Imaging::BitmapSource;


namespace Dizz
{

public ref class ThumbnailMan
{
internal:
    std::map<xziar::img::ImageView, gcroot<BitmapSource^>, common::AlignBufLessor> * const ThumbnailMap = nullptr;
    const std::weak_ptr<rayr::ThumbnailManager> *ThumbMan;

    ThumbnailMan(const common::Wrapper<rayr::ThumbnailManager>& thumbMan);
    ~ThumbnailMan() { this->!ThumbnailMan(); }
    !ThumbnailMan();

    BitmapSource^ GetThumbnail(const xziar::img::ImageView& img);

    BitmapSource^ GetThumbnail(const rayr::TexHolder& holder);
};


}

