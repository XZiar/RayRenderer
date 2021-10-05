#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
//#include "Async.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace msclr::interop;
using System::Windows::Media::Imaging::BitmapSource;


namespace Dizz
{

public ref class TexHolder
{
internal:
    Common::NativeWrapper<std::weak_ptr<void>> WeakRef;
private:
    const uint8_t TypeId;
    initonly String^ name;
    initonly String^ format;
    TexHolder(const dizz::TexHolder& holder);
internal:
    static TexHolder^ CreateTexHolder(const dizz::TexHolder& holder);
    dizz::TexHolder ExtractHolder();
public:
    ~TexHolder() { this->!TexHolder(); }
    !TexHolder();
    CLI_READONLY_PROPERTY(String^, Name, name)
    CLI_READONLY_PROPERTY(String^, Format, format)
};


public enum class TexLoadType : uint8_t { Color = (uint8_t)dizz::TexLoadType::Color, Normal = (uint8_t)dizz::TexLoadType::Normal };
public ref class TextureLoader
{
internal:
    const std::weak_ptr<dizz::TextureLoader> *Loader;
    TextureLoader(const std::shared_ptr<dizz::TextureLoader>& loader);
public:
    ~TextureLoader() { this->!TextureLoader(); }
    !TextureLoader();
    Task<TexHolder^>^ LoadTextureAsync(String^ fname, TexLoadType type);
    Task<TexHolder^>^ LoadTextureAsync(BitmapSource^ image, TexLoadType type);
};

public ref class ThumbnailMan
{
private:
    initonly ReaderWriterLockSlim^ Lock;
internal:
    //Common::NativeWrapper<std::map<xziar::img::ImageView, gcroot<BitmapSource^>, common::AlignBufLessor>> ThumbnailMap;
    std::map<xziar::img::ImageView, gcroot<BitmapSource^>, common::AlignBufLessor> * const ThumbnailMap = nullptr;
    const std::weak_ptr<dizz::ThumbnailManager> *ThumbMan;

    ThumbnailMan(const std::shared_ptr<dizz::ThumbnailManager>& thumbMan);

    BitmapSource^ GetThumbnail(const xziar::img::ImageView& img);
    //BitmapSource^ GetThumbnail3(IntPtr imgptr);
    BitmapSource^ GetThumbnail(const dizz::TexHolder& holder);
public:
    ~ThumbnailMan() { this->!ThumbnailMan(); }
    !ThumbnailMan();
    Task<BitmapSource^>^ GetThumbnailAsync(TexHolder^ holder);
};


}

