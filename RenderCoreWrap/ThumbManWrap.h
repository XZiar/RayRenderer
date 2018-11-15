#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "Async.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;
using System::Windows::Media::Imaging::BitmapSource;


namespace Dizz
{

public ref class TexHolder
{
private:
    Common::NativeWrapper<std::weak_ptr<void>> WeakRef;
    const uint8_t TypeId;
    TexHolder(const oglu::oglTex2D& tex);
    TexHolder(const rayr::FakeTex& tex);
internal:
    static TexHolder^ CreateTexHolder(const rayr::TexHolder& holder);
    rayr::TexHolder ExtractHolder();
public:
    ~TexHolder() { this->!TexHolder(); }
    !TexHolder();
};


public ref class ThumbnailMan
{
internal:
    std::map<xziar::img::ImageView, gcroot<BitmapSource^>, common::AlignBufLessor> * const ThumbnailMap = nullptr;
    const std::weak_ptr<rayr::ThumbnailManager> *ThumbMan;

    ThumbnailMan(const common::Wrapper<rayr::ThumbnailManager>& thumbMan);

    BitmapSource^ GetThumbnail(const xziar::img::ImageView& img);
    BitmapSource^ GetThumbnail2(Common::CLIWrapper<std::optional<xziar::img::ImageView>>^ img);
    BitmapSource^ GetThumbnail(const rayr::TexHolder& holder);
public:
    ~ThumbnailMan() { this->!ThumbnailMan(); }
    !ThumbnailMan();
    //BitmapSource^ GetThumbnail(TexHolder^ holder) { return GetThumbnail(holder->ExtractHolder()); }
    Common::WaitObj<std::optional<xziar::img::ImageView>, BitmapSource^>^ GetThumbnailAsync(TexHolder^ holder);
};


}

