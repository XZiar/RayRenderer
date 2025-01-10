#include "ImageUtilPch.h"
#include "ImageWIC.h"
#include "ImageUtil.h"
#include "SystemCommon/Format.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/ErrorCodeHelper.h"
#include "common/StaticLookup.hpp"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <wrl/client.h>
#include <initguid.h>
#include <windows.h>
#include <wincodec.h>

#define THROW_HR(eval, msg) do { if (const common::HResultHolder hr___ = eval; !hr___)  \
{                                                                                       \
    ImgLog().Warning(msg ".\n");                                                        \
    COMMON_THROWEX(common::BaseException, msg)                                          \
        .Attach("HResult", hr___).Attach("detail", hr___.ToStr());                      \
} } while(0)


namespace xziar::img
{
using common::BaseException;
using common::HResultHolder;

namespace wic
{
using common::com::PtrProxy;

struct WICFactory : public IWICImagingFactory
{
    using RealType = IWICImagingFactory;
};
struct WICDecoder : public IWICBitmapDecoder
{
    using RealType = IWICBitmapDecoder;
};
struct WICEncoder : public IWICBitmapEncoder
{
    using RealType = IWICBitmapEncoder;
};


struct DataTypeCvt
{
    ImgDType MidType;
    const GUID* Guid;
};
#define DtypeGuidPair(type, mid, name) { ImageDataType::type.Value, { ImageDataType::mid, &GUID_WICPixelFormat##name } }
static constexpr auto DataTypeGuidLookup = BuildStaticLookup(uint8_t, DataTypeCvt,
    DtypeGuidPair(RGBA,  RGBA,  32bppRGBA),
    DtypeGuidPair(BGRA,  BGRA,  32bppBGRA),
    DtypeGuidPair(RGB,   RGB,   24bppRGB),
    DtypeGuidPair(BGR,   BGR,   24bppBGR),
    DtypeGuidPair(RGBAf, RGBAf, 128bppRGBAFloat),
    DtypeGuidPair(BGRAf, RGBAf, 128bppRGBAFloat),
    DtypeGuidPair(RGBf,  RGBf,  96bppRGBFloat),
    DtypeGuidPair(BGRf,  RGBf,  96bppRGBFloat),
    DtypeGuidPair(RGBAh, RGBAh, 64bppRGBAHalf),
    DtypeGuidPair(BGRAh, RGBAh, 64bppRGBAHalf),
    DtypeGuidPair(RGBh,  RGBh,  48bppRGBHalf),
    DtypeGuidPair(BGRh,  RGBh,  48bppRGBHalf),
    DtypeGuidPair(GRAY,  GRAY,  8bppGray),
    DtypeGuidPair(GRAYf, GRAYf, 32bppGrayFloat),
    DtypeGuidPair(GRAYh, GRAYh, 16bppGrayHalf));
#undef DtypeGuidPair


static std::optional<ImgDType> TryGetBitmapDType(IWICBitmapSource& wicbitmap)
{
    WICPixelFormatGUID srcFormat = {};
    THROW_HR(wicbitmap.GetPixelFormat(&srcFormat), u"Failed to get wicbitmap's format");
    for (const auto& cvt : wic::DataTypeGuidLookup.Items)
    {
        if (IsEqualGUID(srcFormat, *cvt.Value.Guid))
        {
            return cvt.Value.MidType;
        }
    }
    return {};
}

static std::optional<Image> TryDirectGetImage(IWICBitmapSource& bitmap, uint32_t width, uint32_t height)
{
    WICPixelFormatGUID srcFormat = {};
    THROW_HR(bitmap.GetPixelFormat(&srcFormat), u"Failed to get wicbitmap's format");
    for (const auto& cvt : wic::DataTypeGuidLookup.Items)
    {
        if (IsEqualGUID(srcFormat, *cvt.Value.Guid))
        {
            Image img(cvt.Value.MidType);
            img.SetSize(width, height, false);
            THROW_HR(bitmap.CopyPixels(nullptr, gsl::narrow_cast<uint32_t>(img.GetElementSize() * width), gsl::narrow_cast<uint32_t>(img.GetSize()), img.GetRawPtr<BYTE>()),
                u"Failed to copy pixels");
            return img;
        }
    }
    return {};
}

WicReader::WicReader(std::shared_ptr<const WicSupport>&& support, common::com::PtrProxy<WICDecoder>&& decoder) :
    Support(std::move(support)), Decoder(std::move(decoder))
{
}
WicReader::~WicReader() {}

bool WicReader::Validate()
{
    uint32_t count = 0;
    const HResultHolder hr = Decoder->GetFrameCount(&count);
    return hr && count > 0;
}

Image WicReader::Read(ImgDType dataType)
{

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    THROW_HR(Decoder->GetFrame(0, frame.GetAddressOf()), u"Failed to get frame");
    uint32_t width = 0, height = 0;
    THROW_HR(frame->GetSize(&width, &height), u"Failed to get size");

    if (!dataType)
    {
        try
        {
            if (auto img = wic::TryDirectGetImage(*frame.Get(), width, height); img)
                return *std::move(img);
            else
                ImgLog().Verbose(u"Does not find direct mapping with format GUID, fallback to RGBA.\n");
        }
        catch (BaseException&) // already logged
        {
            ImgLog().Verbose(u"Can not find direct mapping datatype, fallback to RGBA.\n");
        }
        catch (...)
        {
            ImgLog().Verbose(u"Unknown error, can not find direct mapping datatype, fallback to RGBA.\n");
        }
        dataType = ImageDataType::RGBA; // default
    }

    const auto cvt = DataTypeGuidLookup(dataType.Value);
    if (!cvt)
        COMMON_THROWEX(BaseException, u"datatype not supported");

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    THROW_HR(Support->Factory->CreateFormatConverter(&converter), u"Failed to create converter");
    THROW_HR(converter->Initialize(frame.Get(), *cvt->Guid, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom),
        u"Failed to init converter");

    Image image(cvt->MidType);
    image.SetSize(width, height, false);
    THROW_HR(converter->CopyPixels(nullptr, gsl::narrow_cast<uint32_t>(image.GetElementSize() * width), gsl::narrow_cast<uint32_t>(image.GetSize()), image.GetRawPtr<BYTE>()),
        u"Failed to copy pixels");

    if (image.GetDataType() != dataType)
        return image.ConvertTo(dataType);
    return image;
}


WicWriter::WicWriter(std::shared_ptr<const WicSupport>&& support, common::com::PtrProxy<WICEncoder>&& encoder, ImgType type) :
    Support(std::move(support)), Encoder(std::move(encoder)), Type(type)
{
}
WicWriter::~WicWriter() {}

void WicWriter::Write(ImageView image, const uint8_t quality)
{
    const auto origType = image.GetDataType();
    const auto cvt = DataTypeGuidLookup(origType.Value);
    if (!cvt)
    {
        COMMON_THROWEX(BaseException, u"datatype not supported");
    }
    if (origType != cvt->MidType)
        image = image.ConvertTo(cvt->MidType);

    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
    Microsoft::WRL::ComPtr<IPropertyBag2> props;
    THROW_HR(Encoder->CreateNewFrame(frame.GetAddressOf(), props.GetAddressOf()), u"Failed to create frame");
    std::vector<PROPBAG2> options;
    std::vector<VARIANT> values;
    switch (Type)
    {
    case ImgType::TIF:
    {
        options.emplace_back().pstrName = const_cast<LPOLESTR>(L"CompressionQuality");
        auto& value = values.emplace_back();
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = quality / 100.0f;
    } break;
    case ImgType::JPG:
    case ImgType::HEIF:
    {
        options.emplace_back().pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        auto& value = values.emplace_back();
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = quality / 100.0f;
    } break;
    }
    if (!options.empty())
        THROW_HR(props->Write(static_cast<ULONG>(options.size()), options.data(), values.data()), u"Failed to set props");
    THROW_HR(frame->Initialize(props.Get()), u"Failed to init frame");
    THROW_HR(frame->SetSize(image.GetWidth(), image.GetHeight()), u"Failed to set size");
    GUID targetFormat = *cvt->Guid;
    THROW_HR(frame->SetPixelFormat(&targetFormat), u"Failed to set pix format");
    if (IsEqualGUID(targetFormat, *cvt->Guid))
    {
        THROW_HR(frame->WritePixels(image.GetHeight(), gsl::narrow_cast<uint32_t>(image.GetElementSize() * image.GetWidth()), gsl::narrow_cast<uint32_t>(image.GetSize()),
            const_cast<BYTE*>(image.GetRawPtr<BYTE>())), u"Failed to write pixels");
    }
    else
    {
        ImgLog().Verbose(u"WIC asks for pixel format conversion.\n");
        Microsoft::WRL::ComPtr<IWICBitmap> srcBitmap;
        THROW_HR(Support->Factory->CreateBitmapFromMemory(image.GetWidth(), image.GetHeight(), *cvt->Guid,
            gsl::narrow_cast<uint32_t>(image.GetElementSize() * image.GetWidth()), gsl::narrow_cast<uint32_t>(image.GetSize()),
            const_cast<BYTE*>(image.GetRawPtr<BYTE>()), srcBitmap.GetAddressOf()), u"Failed to create bitmap");

        Microsoft::WRL::ComPtr<IWICPalette> platte;
        std::optional<uint32_t> platteColorCount;
#define SetPlatte(bpp) if (IsEqualGUID(targetFormat, GUID_WICPixelFormat##bpp##bppIndexed)) platteColorCount = 1u << bpp
        SetPlatte(1);
else SetPlatte(2);
    else SetPlatte(4);
    else SetPlatte(8);
#undef SetPlatte
    if (platteColorCount)
    {
        ImgLog().Verbose(u"WIC asks for platte pixel format.\n");
        THROW_HR(Support->Factory->CreatePalette(platte.GetAddressOf()), u"Failed to create platte");
        THROW_HR(platte->InitializeFromBitmap(srcBitmap.Get(), *platteColorCount, origType.HasAlpha()), u"Failed to init platte");
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    THROW_HR(Support->Factory->CreateFormatConverter(converter.GetAddressOf()), u"Failed to create converter");
    THROW_HR(converter->Initialize(srcBitmap.Get(), targetFormat, WICBitmapDitherTypeNone, platte.Get(), 0.0f, WICBitmapPaletteTypeCustom),
        u"Failed to init converter");

    THROW_HR(frame->WriteSource(converter.Get(), nullptr), u"Failed to write bitmap");
    }
    frame->Commit();
    Encoder->Commit();
}


WicSupport::WicSupport() : ImgSupport(u"Wic")
{
    if (const HResultHolder ret = CoInitializeEx(nullptr, COINIT_MULTITHREADED); ret.Value == S_OK || ret.Value == S_FALSE)
    {
        // Create WIC factory
        THROW_HR(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Factory)), u"Init WIC Factory failed");
    }
    else
        COMMON_THROWEX(BaseException, u"Init multi-thread COM failed").Attach("HResult", ret).Attach("detail", ret.ToStr());
}
WicSupport::~WicSupport()
{
}
PtrProxy<WICFactory> WicSupport::GetFactory() const { return Factory; }



struct WrapInputStream final : public IStream
{
    std::atomic<uint32_t> RefCount;
    common::io::RandomInputStream* Stream;
    WrapInputStream(common::io::RandomInputStream& stream) noexcept : RefCount{ 1u }, Stream(&stream) {}

    ULONG STDMETHODCALLTYPE AddRef(void) final { return ++RefCount; }
    ULONG STDMETHODCALLTYPE Release(void) final
    {
        const auto count = --RefCount;
        if (!count)
            delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) final
    {
        if (!ppvObject) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IStream) || riid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead) final
    {
        const auto cnt = gsl::narrow_cast<ULONG>(Stream->ReadMany(cb, 1, pv));
        if (pcbRead) *pcbRead = cnt;
        return cnt == cb ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, _Out_opt_ ULARGE_INTEGER* plibNewPosition) final
    {
        if (plibNewPosition) plibNewPosition->QuadPart = 0;
        const auto size = Stream->GetSize();
        size_t newPos = 0;
        switch (dwOrigin)
        {
        case STREAM_SEEK_SET:
            if (const auto pos = static_cast<ULONGLONG>(dlibMove.QuadPart); pos > size)
                return STG_E_INVALIDFUNCTION;
            else
                newPos = pos;
            break;
        case STREAM_SEEK_CUR:
        {
            const auto cur = Stream->CurrentPos();
            if (dlibMove.QuadPart >= 0)
            {
                const auto add = static_cast<ULONGLONG>(dlibMove.QuadPart);
                if (size - cur < add)
                    return STG_E_INVALIDFUNCTION;
                newPos = cur + add;
            }
            else
            {
                const auto sub = static_cast<ULONGLONG>(-dlibMove.QuadPart);
                if (cur < sub)
                    return STG_E_INVALIDFUNCTION;
                newPos = cur - sub;
            }
        } break;
        case STREAM_SEEK_END:
            if (dlibMove.QuadPart >= 0)
                return STG_E_INVALIDFUNCTION;
            if (const auto sub = static_cast<ULONGLONG>(-dlibMove.QuadPart); sub > size)
                return STG_E_INVALIDFUNCTION;
            else
                newPos = size - sub;
            break;
        default: return STG_E_INVALIDFUNCTION;
        }
        if (!Stream->SetPos(newPos))
            return STG_E_INVALIDFUNCTION;
        if (plibNewPosition)
            plibNewPosition->QuadPart = static_cast<ULONGLONG>(newPos);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE CopyTo(_In_ IStream* pstm, ULARGE_INTEGER cb, _Out_opt_ ULARGE_INTEGER* pcbRead, _Out_opt_ ULARGE_INTEGER* pcbWritten) final
    {
        if (pcbRead) pcbRead->QuadPart = 0;
        if (pcbWritten) pcbWritten->QuadPart = 0;
        if (!pstm) return STG_E_INVALIDPOINTER;
        const auto tmp = Stream->ReadToVector<std::byte>(gsl::narrow_cast<size_t>(cb.QuadPart));
        if (pcbRead) pcbRead->QuadPart = static_cast<ULONGLONG>(tmp.size());
        for (size_t offset = 0, left = tmp.size(); left;)
        {
            const auto count = common::saturate_cast<ULONG>(left);
            ULONG written = 0;
            const auto hr = pstm->Write(tmp.data() + offset, count, &written);
            if (hr == S_OK)
                offset += written, left -= written;
            else
                return STG_E_MEDIUMFULL;
        }
        if (pcbWritten) pcbWritten->QuadPart = static_cast<ULONGLONG>(tmp.size());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void*, [[maybe_unused]] _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) final
    {
        if (pcbWritten) *pcbWritten = 0;
        return STG_E_ACCESSDENIED;
    }
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) final
    {
        return STG_E_ACCESSDENIED;
    }

    HRESULT STDMETHODCALLTYPE Stat(__RPC__out STATSTG* pstatstg, DWORD) final
    {
        if (!pstatstg) return STG_E_INVALIDPOINTER;
        ZeroMemory(pstatstg, sizeof(STATSTG));
        pstatstg->type = STGTY_STREAM;
        pstatstg->grfMode = STGM_READ;
        pstatstg->cbSize.QuadPart = static_cast<ULONGLONG>(Stream->GetSize());
        pstatstg->grfLocksSupported = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Commit(DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Revert(void) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Clone(__RPC__deref_out_opt IStream**) final { return E_NOTIMPL; }
};


std::unique_ptr<ImgReader> WicSupport::GetReader(common::io::RandomInputStream& stream, std::u16string_view) const
{
    Microsoft::WRL::ComPtr<IStream> istream;
    if (const auto space = stream.TryGetAvaliableInMemory(); space && space->size() == stream.GetSize() && space->size() <= std::numeric_limits<DWORD>::max())
    { // all in memory and within uintmax
        ImgLog().Debug(u"WIC bypass Stream with mem region.\n");
        Microsoft::WRL::ComPtr<IWICStream> wicStream;
        THROW_HR(Factory->CreateStream(wicStream.GetAddressOf()), u"WIC failed to create WICStream");
        THROW_HR(wicStream->InitializeFromMemory(reinterpret_cast<BYTE*>(const_cast<std::byte*>(space->data())), static_cast<DWORD>(space->size())),
            u"WIC failed to init WICStream");
        THROW_HR(wicStream->QueryInterface(istream.GetAddressOf()), u"Failed to get IStream");
    }
    else
    {
        istream = new WrapInputStream(stream);
    }
    PtrProxy<WICDecoder> decoder;
    THROW_HR(Factory->CreateDecoderFromStream(istream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder), u"WIC failed to create WICDecoder");
    return std::make_unique<WicReader>(shared_from_this(), std::move(decoder));
}


struct WrapOutputStream final : public IStream
{
    std::atomic<uint32_t> RefCount;
    common::io::RandomOutputStream* Stream;
    size_t Pos = 0;
    WrapOutputStream(common::io::RandomOutputStream& stream) noexcept : RefCount{ 1u }, Stream(&stream) {}

    ULONG STDMETHODCALLTYPE AddRef(void) final { return ++RefCount; }
    ULONG STDMETHODCALLTYPE Release(void) final
    {
        const auto count = --RefCount;
        if (!count)
            delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) final
    {
        if (!ppvObject) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IStream) || riid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Read([[maybe_unused]] _Out_writes_bytes_to_(cb, *pcbRead) void* pv, [[maybe_unused]] _In_ ULONG cb, _Out_opt_ ULONG* pcbRead) final
    {
        if (pcbRead) *pcbRead = 0;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, _Out_opt_ ULARGE_INTEGER* plibNewPosition) final
    {
        if (plibNewPosition) plibNewPosition->QuadPart = 0;
        const auto size = Stream->GetSize();
        size_t newPos = 0;
        switch (dwOrigin)
        {
        case STREAM_SEEK_SET:
            newPos = static_cast<ULONGLONG>(dlibMove.QuadPart);
            break;
        case STREAM_SEEK_CUR:
        case STREAM_SEEK_END:
        {
            const auto base = dwOrigin == STREAM_SEEK_CUR ? Stream->CurrentPos() : size;
            if (dlibMove.QuadPart >= 0)
            {
                const auto add = static_cast<ULONGLONG>(dlibMove.QuadPart);
                if (base + add < base) // overflow
                    return STG_E_INVALIDFUNCTION;
                newPos = base + add;
            }
            else
            {
                const auto sub = static_cast<ULONGLONG>(-dlibMove.QuadPart);
                if (base < sub)
                    return STG_E_INVALIDFUNCTION;
                newPos = base - sub;
            }
        } break;
        default: return STG_E_INVALIDFUNCTION;
        }
        if (newPos <= size)
        {
            if (!Stream->SetPos(newPos))
                return STG_E_INVALIDFUNCTION;
        }
        Pos = newPos;
        if (plibNewPosition)
            plibNewPosition->QuadPart = static_cast<ULONGLONG>(newPos);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE CopyTo([[maybe_unused]] _In_ IStream* pstm, [[maybe_unused]] ULARGE_INTEGER cb, _Out_opt_ ULARGE_INTEGER* pcbRead, _Out_opt_ ULARGE_INTEGER* pcbWritten) final
    {
        if (pcbRead) pcbRead->QuadPart = 0;
        if (pcbWritten) pcbWritten->QuadPart = 0;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) final
    {
        if (pcbWritten) *pcbWritten = 0;
        const auto curPos = Stream->CurrentPos();
        Expects(Pos >= curPos);
        if (Pos > curPos)
        {
            if (!Stream->ReSize(Pos) || !Stream->SetPos(Pos))
                return STG_E_MEDIUMFULL;
        }
        Ensures(Pos == Stream->CurrentPos());
        const auto written = Stream->WriteMany(cb, 1, pv);
        Pos = Stream->CurrentPos();
        if (pcbWritten) *pcbWritten = static_cast<ULONG>(written);
        if (written < cb) return STG_E_MEDIUMFULL;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) final
    {
        const auto newSize = gsl::narrow_cast<size_t>(libNewSize.QuadPart);
        if (newSize != Stream->GetSize())
        {
            if (!Stream->ReSize(newSize))
                return STG_E_MEDIUMFULL;
            if (Pos <= newSize)
            {
                if (!Stream->SetPos(Pos))
                    return STG_E_MEDIUMFULL;
            }
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Stat(__RPC__out STATSTG* pstatstg, DWORD) final
    {
        if (!pstatstg) return STG_E_INVALIDPOINTER;
        ZeroMemory(pstatstg, sizeof(STATSTG));
        pstatstg->type = STGTY_STREAM;
        pstatstg->grfMode = STGM_WRITE;
        pstatstg->cbSize.QuadPart = static_cast<ULONGLONG>(Stream->GetSize());
        pstatstg->grfLocksSupported = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Commit(DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Revert(void) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) final { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Clone(__RPC__deref_out_opt IStream**) final { return E_NOTIMPL; }
};


std::unique_ptr<ImgWriter> WicSupport::GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const
{
    static const std::map<std::u16string_view, std::pair<ImgType, const GUID*>> extMapping =
    {
#define ExtGuidPair(ext, type, name) { u## #ext, { ImgType::type, &GUID_ContainerFormat##name } }
        ExtGuidPair(TIF,  TIF,  Tiff),
        ExtGuidPair(TIFF, TIF,  Tiff),
        ExtGuidPair(BMP,  BMP,  Bmp),
        ExtGuidPair(PNG,  PNG,  Png),
        ExtGuidPair(HEIF, HEIF, Heif),
        ExtGuidPair(WEBP, WEBP, Webp),
        ExtGuidPair(JXL,  JXL,  JpegXL),
#undef ExtGuidPair
    };
    const auto it = extMapping.find(ext);
    if (it == extMapping.end())
        COMMON_THROWEX(BaseException, std::u16string(u"Unsupported extension: ").append(ext));
    const auto [type, guid] = it->second;
    PtrProxy<WICEncoder> encoder;
    THROW_HR(Factory->CreateEncoder(*guid, nullptr, &encoder), u"WIC failed to create WICEncoder");
    Microsoft::WRL::ComPtr<IStream> ostream = new WrapOutputStream(stream);
    THROW_HR(encoder->Initialize(ostream.Get(), WICBitmapEncoderNoCache), u"WIC failed to init WICEncoder");
    return std::make_unique<WicWriter>(shared_from_this(), std::move(encoder), type);
}

uint8_t WicSupport::MatchExtension(std::u16string_view ext, ImgDType dataType, const bool isRead) const
{
    if (dataType && !DataTypeGuidLookup(dataType.Value))
        return 0;
    if (isRead)
    {
        if (ext == u"BMP" || ext == u"HEIF" || ext == u"WEBP" || ext == u"AVIF" || ext == u"TIFF" || ext == u"TIF")
            return 240;
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG")
            return 128;
    }
    else
    {
        if (ext == u"BMP")
            return (dataType && dataType.ChannelCount() <= 2) ? 64 : 240; // WIC asks for platte with Gray Bmp
        if (ext == u"TIFF" || ext == u"TIF")
            return 240;
        if (ext == u"JPG" || ext == u"JPEG" || ext == u"PNG" || ext == u"JXL" || ext == u"HEIF" || ext == u"WEBP")
            return 128;
    }
    return 0;
}


static auto DUMMY = []()
{
    try
    {
        auto support = std::make_shared<WicSupport>();
        RegistImageSupport(support);
        return support;
    }
    catch (const BaseException& be)
    {
        common::mlog::LogInitMessage(common::mlog::LogLevel::Warning, "ImageWIC",
            common::str::Formatter<char>{}.FormatStatic(FmtString("{}: {}.\n"), be.Message(), be.GetDetailMessage()));
    }
    return std::shared_ptr<WicSupport>{};
}();


}


static Image ConvertFromBITMAP(const HBITMAP& hbmp, const BITMAP& bmp, const HDC& hdc)
{
    ImgLog().Verbose(u"Image from HBITMAP: BITMAP w[{}] h[{}] plane[{}] bpp[{}] line[{}].\n", bmp.bmWidth, bmp.bmHeight, bmp.bmPlanes, bmp.bmBitsPixel, bmp.bmWidthBytes);
    
    ImgDType srcDT, realDT;
    switch (static_cast<uint32_t>(bmp.bmPlanes) | bmp.bmBitsPixel)
    {
    case (4 << 16 | 32):    srcDT = ImageDataType::BGRA; realDT = ImageDataType::BGRA; break;
    case (3 << 16 | 32):    srcDT = ImageDataType::BGRA; realDT = ImageDataType::BGR;  break;
    case (3 << 16 | 24):    srcDT = ImageDataType::BGR;  realDT = ImageDataType::BGR;  break;
    case (1 << 16 | 8 ):    srcDT = ImageDataType::GRAY; realDT = ImageDataType::GRAY; break;
    default: break;
    }
    if (srcDT && realDT)
    {
        Image img(realDT);
        img.SetSize(bmp.bmWidth, bmp.bmHeight, false);
        auto dstPtr = img.GetRawPtr();
        uint32_t batchCnt = bmp.bmHeight, batchPix = bmp.bmWidth;
        if (bmp.bmWidthBytes == img.RowSize())
            batchPix *= batchCnt, batchCnt = 1;

        common::AlignedBuffer buf;
        auto srcPtr = reinterpret_cast<const std::byte*>(bmp.bmBits);
        if (!srcPtr)
        {
            BITMAPINFO bmi =
            {
                .bmiHeader =
                {
                    .biSize = sizeof(BITMAPINFOHEADER),
                    .biWidth = bmp.bmWidth,
                    .biHeight = -bmp.bmHeight,
                    .biPlanes = 1,
                    .biBitCount = bmp.bmBitsPixel,
                    .biCompression = BI_RGB,
                    .biSizeImage = 0,
                    .biXPelsPerMeter = 96,
                    .biYPelsPerMeter = 96,
                    .biClrUsed = 0,
                    .biClrImportant = 0,
                }
            };
            if (batchCnt == 1) // full match
            {
                if (GetDIBits((HDC)hdc, hbmp, 0, bmp.bmHeight, dstPtr, &bmi, DIB_RGB_COLORS))
                    return img;
                else
                    return {};
            }
            buf = common::AlignedBuffer(bmp.bmWidthBytes * bmp.bmHeight);
            if (GetDIBits((HDC)hdc, hbmp, 0, 1, buf.GetRawPtr(), &bmi, DIB_RGB_COLORS))
                srcPtr = buf.GetRawPtr();
            else
                return {};
        }
        Ensures(srcPtr);
        if (srcDT == realDT)
        {
            for (; batchCnt--; dstPtr += img.RowSize(), srcPtr += img.RowSize())
                memcpy(dstPtr, srcPtr, batchPix * img.GetElementSize());
        }
        else
        {
            Ensures(srcDT == ImageDataType::BGRA && realDT == ImageDataType::BGR);
            const auto& cvter = ColorConvertor::Get();
            for (; batchCnt--; dstPtr += img.RowSize(), srcPtr += img.RowSize())
                cvter.RGBAToRGB(reinterpret_cast<uint8_t*>(dstPtr), reinterpret_cast<const uint32_t*>(srcPtr), batchPix);
        }
        return img;
    }
    return {};
};
Image ConvertFromHBITMAP(void* hbitmap, void* hdc)
{
    const auto hbmp = reinterpret_cast<HBITMAP>(hbitmap);
    if (wic::DUMMY)
    {
        try
        {
            const auto factory = wic::DUMMY->GetFactory();
            Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
            THROW_HR(factory->CreateBitmapFromHBITMAP(hbmp, nullptr, WICBitmapUseAlpha, wicBitmap.GetAddressOf()),
                u"Failed to create wic bitmap from hbitmap");
            uint32_t width = 0, height = 0;
            THROW_HR(wicBitmap->GetSize(&width, &height), u"Failed to get size");
            if (auto img = wic::TryDirectGetImage(*wicBitmap.Get(), width, height); img)
                return *std::move(img);
        }
        catch (BaseException&) {} // already logged
    }
    DIBSECTION dib{};
    if (GetObjectW(hbmp, sizeof(DIBSECTION), &dib))
        ImgLog().Verbose(u"Image from HBITMAP: DIBSECT w[{}] h[{}] plane[{}] bpp[{}] comp[{}].\n", dib.dsBmih.biWidth, dib.dsBmih.biHeight, dib.dsBmih.biPlanes, dib.dsBmih.biBitCount, dib.dsBmih.biCompression);
    else if (GetObjectW(hbmp, sizeof(BITMAP), &dib.dsBm)) {}
    
    if (dib.dsBm.bmWidth && dib.dsBm.bmHeight)
    {
        if (auto img = ConvertFromBITMAP(hbmp, dib.dsBm, reinterpret_cast<HDC>(hdc)); img.GetSize())
            return img;
    }
    return {};
}


}
