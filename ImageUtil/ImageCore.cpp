#include "ImageUtilPch.h"
#include "ImageCore.h"
#include "ColorConvert.h"

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

// #define STB_IMAGE_RESIZE2_IMPLEMENTATION
// #define STB_IMAGE_RESIZE_STATIC
#include "stb/stb_image_resize2.h"


namespace xziar::img
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;


std::string_view ImgDType::Stringify(Channels ch) noexcept
{
    switch (ch)
    {
    case Channels::R:    return "R"sv;
    case Channels::RA:   return "RA"sv;
    case Channels::RGB:  return "RGB"sv;
    case Channels::RGBA: return "RGBA"sv;
    case Channels::BGR:  return "BGR"sv;
    case Channels::BGRA: return "BGRA"sv;
    default:             return {};
    }
}
std::string_view ImgDType::Stringify(DataTypes type) noexcept
{
    switch (type)
    {
    case DataTypes::Uint8:   return "UINT8"sv;
    case DataTypes::Uint16:  return "UINT16"sv;
    case DataTypes::Fixed16: return "FIXED16"sv;
    case DataTypes::Float16: return "FLOAT16"sv;
    case DataTypes::Float32: return "FLOAT32"sv;
    default:                 return {};
    }
}
#define DTLookupItem(r, dummy, i, name) BOOST_PP_COMMA_IF(i) { ImageDataType::name.Value, STRINGIZE(name)""sv }
#define DTLookup(...) BuildStaticLookup(uint8_t, std::string_view, BOOST_PP_SEQ_FOR_EACH_I(DTLookupItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))
static constexpr auto DataTypeNameLookup = DTLookup(
    RGBA,   BGRA,   RGB,   BGR,   GA,   GRAY,
    RGBA16, BGRA16, RGB16, BGR16, GA16, GRAY16,
    RGBAf,  BGRAf,  RGBf,  BGRf,  GAf,  GRAYf,
    RGBAh,  BGRAh,  RGBh,  BGRh,  GAh,  GRAYh);
#undef DTLookupItem
#undef DTLookup
std::string ImgDType::Stringify(const ImgDType& type, bool detail) noexcept
{
    if (!detail)
    {
        const auto knownName = DataTypeNameLookup(type.Value);
        if (knownName) return std::string(*knownName);
        return {};
    }
    const auto ch = Stringify(type.Channel());
    const auto dt = Stringify(type.DataType());
    if (ch.empty() || dt.empty()) return "UNKNOWN";
    return std::string(dt).append("|").append(ch);
}
void ImgDType::FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& context, const common::str::FormatSpec* spec) const
{
    if (const auto knownName = DataTypeNameLookup(Value); knownName)
    {
        host.PutString(context, *knownName, spec);
        return;
    }
    const auto ch = Stringify(Channel());
    const auto dt = Stringify(DataType());
    if (ch.empty() || dt.empty())
    {
        host.PutString(context, "UNKNOWN"sv, spec);
        return;
    }
    host.PutString(context, dt, spec);
    host.PutString(context, "|"sv, spec);
    host.PutString(context, ch, spec);
}


void Image::ResetSize(const uint32_t width, const uint32_t height)
{
    ReAlloc(static_cast<size_t>(width)* height* ElementSize, 64);
    Width = width, Height = height;
}

void Image::FlipVertical()
{
    const auto lineStep = RowSize();
    auto rowUp = Data, rowDown = Data + (Height - 1) * lineStep;
    while (rowUp < rowDown)
    {
        common::CopyEx.Swap2Region<false>(rowUp, rowDown, lineStep);
        rowUp += lineStep, rowDown -= lineStep;
    }
}

void Image::FlipHorizontal()
{
    const auto lineStep = RowSize();
    auto ptr = Data;
    if (ElementSize == 4)
    {
        for (uint32_t row = 0; row < Height; ++row)
        {
            common::CopyEx.ReverseRegion(reinterpret_cast<uint32_t*>(ptr), Width);
            ptr += lineStep;
        }
    }
    else if (ElementSize == 3)
    {
        for (uint32_t row = 0; row < Height; ++row)
        {
            common::CopyEx.ReverseRegion(reinterpret_cast<std::array<uint8_t, 3>*>(ptr), Width);
            ptr += lineStep;
        }
    }
    else
    {
        COMMON_THROW(BaseException, u"unsupported operation");
    }
}

void Image::Rotate180()
{
    const auto count = Width * Height;
    if (ElementSize == 4)
    {
        common::CopyEx.ReverseRegion(reinterpret_cast<uint32_t*>(Data), count);
    }
    else if (ElementSize == 3)
    {
        common::CopyEx.ReverseRegion(reinterpret_cast<std::array<uint8_t, 3>*>(Data), count);
    }
    else
    {
        COMMON_THROW(BaseException, u"unsupported operation");
    }
}


Image Image::FlipToVertical() const
{
    Image img(DataType);
    img.SetSize(Width, Height);

    const auto lineStep = RowSize();
    const auto ptrFrom = Data + (Height - 1) * lineStep;
    auto* const ptrTo = img.Data;
    for (auto i = Height; i--;)
        memcpy_s(ptrTo, lineStep, ptrFrom, lineStep);
    return img;
}

Image Image::FlipToHorizontal() const
{
    Image img(*this);
    img.FlipHorizontal();
    return img;
}

Image Image::RotateTo180() const
{

    Image img(*this);
    img.Rotate180();
    return img;
}

static forceinline constexpr uint8_t ChPair(ImgDType::Channels a, ImgDType::Channels b) noexcept 
{
    return static_cast<uint8_t>((common::enum_cast(a) << 3) | common::enum_cast(b));
}
static forceinline constexpr uint8_t DtPair(ImgDType::DataTypes a, ImgDType::DataTypes b) noexcept
{
    return static_cast<uint8_t>((common::enum_cast(a) << 4) | common::enum_cast(b));
}

template<typename T1, typename T2, typename T4>
static forceinline void ConvertChannelCopy(std::byte* destPtr, size_t destStep, const std::byte* srcPtr, size_t srcStep, uint32_t batchCnt, const uint32_t batchPix,
    const ImgDType::Channels dstChannel, const ImgDType::Channels srcChannel)
{
    using T3 = T1;
    Expects(dstChannel != srcChannel);
    const auto& cvter = ColorConvertor::Get();
#define ChPairFunc(sCh, dCh, sT, dT, func) case ChPair(ImgDType::Channels::sCh, ImgDType::Channels::dCh): for (; batchCnt--; destPtr += destStep, srcPtr += srcStep) { cvter.func(reinterpret_cast<dT*>(destPtr), reinterpret_cast<const sT*>(srcPtr), batchPix); } break
    switch (ChPair(srcChannel, dstChannel))
    {
    ChPairFunc(RGBA, BGRA, T4, T4, RGBAToBGRA);
    ChPairFunc(RGBA, RGB,  T4, T3, RGBAToRGB);
    ChPairFunc(RGBA, BGR,  T4, T3, RGBAToBGR);
    ChPairFunc(BGRA, RGBA, T4, T4, RGBAToBGRA);
    ChPairFunc(BGRA, BGR,  T4, T3, RGBAToRGB);
    ChPairFunc(BGRA, RGB,  T4, T3, RGBAToBGR);

    ChPairFunc(RGB,  RGBA, T3, T4, RGBToRGBA);
    ChPairFunc(RGB,  BGRA, T3, T4, BGRToRGBA);
    ChPairFunc(RGB,  BGR,  T3, T3, RGBToBGR);
    ChPairFunc(BGR,  BGRA, T3, T4, RGBToRGBA);
    ChPairFunc(BGR,  RGBA, T3, T4, BGRToRGBA);
    ChPairFunc(BGR,  RGB,  T3, T3, RGBToBGR);

    ChPairFunc(RA,   RGBA, T2, T4, GrayAToRGBA);
    ChPairFunc(RA,   BGRA, T2, T4, GrayAToRGBA);
    ChPairFunc(RA,   RGB,  T2, T3, GrayAToRGB);
    ChPairFunc(RA,   BGR,  T2, T3, GrayAToRGB);
    ChPairFunc(RA,   R,    T2, T1, GrayAToGray);

    ChPairFunc(R,    RGBA, T1, T4, GrayToRGBA);
    ChPairFunc(R,    BGRA, T1, T4, GrayToRGBA);
    ChPairFunc(R,    RGB,  T1, T3, GrayToRGB);
    ChPairFunc(R,    BGR,  T1, T3, GrayToRGB);
    ChPairFunc(R,    RA,   T1, T2, GrayToGrayA);
    default: Ensures(false);
    }
#undef ChPairFunc
}
void Image::PlaceImage(ImageView src, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY)
{
    if (srcX >= src.Width || srcY >= src.Height || destX >= Width || destY >= Height)
        return;

    if (DataType.ChannelCount() <= 2 && src.DataType.ChannelCount() > 2) // not supported yet
        COMMON_THROW(BaseException, u"need explicit conversion from color to gray image");

    const auto colcnt = std::min(Width - destX, src.Width - srcX);
    const auto rowcnt = std::min(Height - destY, src.Height - srcY);
    Ensures(colcnt > 0 && rowcnt > 0);

    if (src.Data == Data) // place self
    {
        if (srcX == destX && srcY == destY) return; // no change
        Image tmpimg(DataType);
        tmpimg.SetSize(colcnt, rowcnt);
        tmpimg.PlaceImage(src, srcX, srcY, 0, 0);
        return PlaceImage(tmpimg, 0, 0, destX, destY);
    }

    const byte* __restrict srcPtr = src.GetRawPtr(srcY, srcX);
    byte* __restrict destPtr = GetRawPtr(destY, destX);
    auto srcStep = src.RowSize();
    const auto destStep = RowSize();
    const bool isCopyWholeRow = colcnt == Width && Width == src.Width; // treat as one row
    uint32_t batchPix = isCopyWholeRow ? colcnt * rowcnt : colcnt, batchCnt = isCopyWholeRow ? 1u : rowcnt;

    if (src.DataType == DataType) // plain copy
    {
        auto copysize = batchPix * ElementSize;
        for (; batchCnt--; destPtr += destStep, srcPtr += srcStep)
            memcpy_s(destPtr, copysize, srcPtr, copysize);
        return;
    }

    const auto srcDT = src.DataType.DataType(), dstDT = DataType.DataType();
    const auto srcCh = src.DataType.Channel(), dstCh = DataType.Channel();
    if (srcDT != dstDT)
    {
        const byte* __restrict srcPtr_ = srcPtr;
        byte* __restrict destPtr_ = destPtr;
        uint32_t batchUnit = colcnt * src.DataType.ChannelCount(), batchCnt_ = rowcnt;

        Image tmpimg(ImgDType{ srcCh, dstDT });
        if (srcCh != dstCh)
        {
            tmpimg.SetSize(colcnt, rowcnt);
            destPtr_ = tmpimg.GetRawPtr();
            const bool isCopyWholeRow_ = colcnt == src.Width;
            if (isCopyWholeRow_) // treat as one row
                batchUnit = batchUnit * batchCnt_, batchCnt_ = 1;
        }
        const auto& cvter = ColorConvertor::Get();
        switch (DtPair(srcDT, dstDT))
        {
        case DtPair(ImgDType::DataTypes::Uint8, ImgDType::DataTypes::Uint16):
            for (; batchCnt_--; destPtr_ += destStep, srcPtr_ += srcStep) { cvter.Gray8To16(reinterpret_cast<uint16_t*>(destPtr_), reinterpret_cast<const uint8_t*>(srcPtr_), batchUnit); } break;
        case DtPair(ImgDType::DataTypes::Uint16, ImgDType::DataTypes::Uint8):
            for (; batchCnt_--; destPtr_ += destStep, srcPtr_ += srcStep) { cvter.Gray16To8(reinterpret_cast<uint8_t*>(destPtr_), reinterpret_cast<const uint16_t*>(srcPtr_), batchUnit); } break;
        case DtPair(ImgDType::DataTypes::Float16, ImgDType::DataTypes::Float32):
            for (; batchCnt_--; destPtr_ += destStep, srcPtr_ += srcStep) { cvter.GetCopy().CopyFloat(reinterpret_cast<float*>(destPtr_), reinterpret_cast<const common::fp16_t*>(srcPtr_), batchUnit); } break;
        case DtPair(ImgDType::DataTypes::Float32, ImgDType::DataTypes::Float16):
            for (; batchCnt_--; destPtr_ += destStep, srcPtr_ += srcStep) { cvter.GetCopy().CopyFloat(reinterpret_cast<common::fp16_t*>(destPtr_), reinterpret_cast<const float*>(srcPtr_), batchUnit); } break;
        default:
            COMMON_THROW(BaseException, u"mixing datatype not supported");
        }
        if (srcCh == dstCh) return;
        
        src = tmpimg;
        srcPtr = tmpimg.GetRawPtr();
        srcStep = tmpimg.RowSize();
        const bool isCopyWholeRow_ = colcnt == Width;
        if (isCopyWholeRow_) // treat as one row
            batchPix *= batchCnt, batchCnt = 1;
    }
    Ensures(srcCh != dstCh);
    Ensures(src.DataType.DataType() == dstDT);
    switch(dstDT)
    {
    case ImgDType::DataTypes::Uint16:
    case ImgDType::DataTypes::Fixed16:
    case ImgDType::DataTypes::Float16: ConvertChannelCopy<uint16_t, uint16_t, uint16_t>(destPtr, destStep, srcPtr, srcStep, batchCnt, batchPix, dstCh, srcCh); break;
    case ImgDType::DataTypes::Float32: ConvertChannelCopy<   float,    float,    float>(destPtr, destStep, srcPtr, srcStep, batchCnt, batchPix, dstCh, srcCh); break;
    case ImgDType::DataTypes::Uint8:   ConvertChannelCopy< uint8_t, uint16_t, uint32_t>(destPtr, destStep, srcPtr, srcStep, batchCnt, batchPix, dstCh, srcCh); break;
    default: COMMON_THROW(BaseException, u"unsupported datatype");
    }
}

static void DoResize(const ImgDType datatype, const std::byte* src, std::byte* dst, const uint32_t srcRowStride, const uint32_t dstRowStride, const uint32_t srcW, const uint32_t srcH, const uint32_t dstW, const uint32_t dstH, const bool isSRGB, const bool mulAlpha)
{
    static const STBResize& resizer = []() -> const STBResize&
    {
        const auto& stbResizer = STBResize::Get();
        std::string pathtxt;
        common::str::Formatter<char> fmter;
        const auto selections = stbResizer.GetIntrinMap();
        for (const auto& pathinfo : STBResize::GetSupportMap())
        {
            fmter.FormatToStatic(pathtxt, FmtString("\n--[{}] get paths:"), pathinfo.FuncName.View);
            std::string_view uses;
            for (const auto& item : selections)
                if (item.first == pathinfo.FuncName.View)
                {
                    uses = item.second;
                    break;
                }
            const auto& syntax = common::str::FormatterCombiner::Combine(FmtString(" {}"), FmtString(" [{}]"));
            for (const auto& methods : pathinfo.Variants)
                syntax(methods.MethodName.View == uses ? 1 : 0).To(pathtxt, fmter, methods.MethodName.View);
        }
        ImgLog().Verbose(u"Loaded STBResizer:{}\n", pathtxt);
        return stbResizer;
    }();

    STBResize::ResizeInfo info
    {
        .Input = src,
        .Output = dst,
        .InputRowStride = srcRowStride,
        .OutputRowStride = dstRowStride,
        .InputSizes = { srcW, srcH },
        .OutputSizes = { dstW, dstH },
        .Layout = 0,
        .Datatype = 0,
        .Edge = static_cast<uint8_t>(STBIR_EDGE_REFLECT),
        .Filter = static_cast<uint8_t>(STBIR_FILTER_TRIANGLE),
    };
    switch (datatype.Channel())
    {
    case ImgDType::Channels::RGBA:  info.Layout = static_cast<uint8_t>(mulAlpha ? STBIR_RGBA_PM : STBIR_RGBA); break;
    case ImgDType::Channels::BGRA:  info.Layout = static_cast<uint8_t>(mulAlpha ? STBIR_BGRA_PM : STBIR_BGRA); break;
    case ImgDType::Channels::RGB:   info.Layout = static_cast<uint8_t>(STBIR_RGB); break;
    case ImgDType::Channels::BGR:   info.Layout = static_cast<uint8_t>(STBIR_BGR); break;
    case ImgDType::Channels::RA:    info.Layout = static_cast<uint8_t>(mulAlpha ? STBIR_RA_PM : STBIR_RA); break;
    case ImgDType::Channels::R:     info.Layout = static_cast<uint8_t>(STBIR_1CHANNEL); break;
    default: CM_UNREACHABLE();
    }
    switch (datatype.DataType())
    {
    case ImgDType::DataTypes::Uint8:    info.Datatype = static_cast<uint8_t>(isSRGB ? STBIR_TYPE_UINT8_SRGB : STBIR_TYPE_UINT8); break;
    case ImgDType::DataTypes::Float32:  info.Datatype = static_cast<uint8_t>(STBIR_TYPE_FLOAT); break;
    case ImgDType::DataTypes::Float16:  info.Datatype = static_cast<uint8_t>(STBIR_TYPE_HALF_FLOAT); break;
    default: COMMON_THROW(BaseException, u"unsupported datatype!");
    }
    resizer.Resize(info);
}

Image Image::ResizeTo(uint32_t width, uint32_t height, const bool isSRGB, const bool mulAlpha) const
{
    if (width == 0 && height == 0)
        COMMON_THROW(BaseException, u"image size cannot be all zero!");
    /*if (mulAlpha && !HAS_FIELD(DataType, ImageDataType::ALPHA_MASK))
        ImgLog().Warning(u"Asks for premultiplied alpha for non-alpha source, ignored");*/
    if (isSRGB && DataType.DataType() != ImgDType::DataTypes::Uint8)
        ImgLog().Warning(u"Asks for sRGB handling for non-Uint8 type, ignored");

    width = width == 0 ? (uint32_t)((uint64_t)height * Width / Height) : width;
    height = height == 0 ? (uint32_t)((uint64_t)width * Height / Width) : height;
    Image output(DataType);
    output.SetSize(width, height);
    DoResize(DataType, GetRawPtr(), output.GetRawPtr(), 0, 0, Width, Height, width, height, isSRGB, mulAlpha);
    return output;
}

void Image::ResizeTo(Image& dst, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY, uint32_t width, uint32_t height, const bool isSRGB, const bool mulAlpha) const
{
    if (dst.GetSize() == 0)
        COMMON_THROW(BaseException, u"dst image size cannot be zero!");
    const auto totalW = dst.GetWidth(), totalH = dst.GetHeight();
    if (srcX >= Width || srcY >= Height || destX >= totalW || destY >= totalH)
        return;
    auto dstW = totalW - destX, dstH = totalH - destY;
    auto srcW = Width - srcX, srcH = Height - srcY;
    /*if (mulAlpha && !HAS_FIELD(DataType, ImageDataType::ALPHA_MASK))
        ImgLog().Warning(u"Asks for premultiplied alpha for non-alpha source, ignored");*/
    if (isSRGB && DataType.DataType() != ImgDType::DataTypes::Uint8)
        ImgLog().Warning(u"Asks for sRGB handling for non-Uint8 type, ignored");

    if (width > dstW)
        srcW = dstW * srcW / width;
    else if (width > 0)
        dstW = width;
    if (height > dstH)
        srcH = dstH * srcH / height;
    else if (height > 0)
        dstH = height;

    if (DataType == dst.DataType)
        DoResize(DataType, GetRawPtr(srcY, srcX), dst.GetRawPtr(destY, destX), Width * DataType.ElementSize(), totalW * DataType.ElementSize(), srcW, srcH, dstW, dstH, isSRGB, mulAlpha);
    else
    {
        Image tmp(DataType);
        tmp.SetSize(dstW, dstH, false);
        DoResize(DataType, GetRawPtr(srcY, srcX), tmp.GetRawPtr(), Width * DataType.ElementSize(), 0, srcW, srcH, dstW, dstH, isSRGB, mulAlpha);
        dst.PlaceImage(tmp, 0, 0, destX, destY);
    }
}


Image Image::Region(const uint32_t x, const uint32_t y, uint32_t w, uint32_t h) const
{
    if (w == 0) w = Width;
    if (h == 0) h = Height;
    if (x == y && y == 0 && w == Width && h == Height)
        return *this;

    Image newimg(DataType);
    newimg.SetSize(w, h);
    if (x >= Width || y >= Height)
        return newimg;
    newimg.PlaceImage(*this, x, y, 0, 0);
    return newimg;
}

Image Image::ConvertTo(const ImgDType dataType, const uint32_t x, const uint32_t y, uint32_t w, uint32_t h) const
{
    if (w == 0) w = Width;
    if (h == 0) h = Height;
    if (dataType == DataType)
        return Region(x, y, w, h);

    Image newimg(dataType);
    newimg.SetSize(w, h);
    newimg.PlaceImage(*this, x, y, 0, 0);
    return newimg;
}

Image Image::ConvertFloat(const ImgDType::DataTypes dataType, const float floatRange) const
{
    const auto origType = DataType.DataType();
    const auto isOrigFloat = DataType.IsFloat(), isDestFloat = ImgDType::IsFloat(dataType);
    if (!isOrigFloat && !isDestFloat) COMMON_THROW(BaseException, u"no float dataType involved!");
    if (isOrigFloat && isDestFloat) COMMON_THROW(BaseException, u"cannot convert between float dataType!");

    Image newimg(ImgDType{ DataType.Channel(), dataType });
    newimg.SetSize(Width, Height);
    const auto size = Width * Height * DataType.ChannelCount();
    const auto& cvter = ColorConvertor::Get();
    if (isDestFloat) // non-float -> float
    {
#define DtPairFunc(sDT, dDT, sT, dT) case DtPair(ImgDType::DataTypes::sDT, ImgDType::DataTypes::dDT): cvter.GetCopy().CopyToFloat(reinterpret_cast<dT*>(newimg.Data), reinterpret_cast<const sT*>(Data), size, floatRange); break
        switch (DtPair(origType, dataType))
        {
        DtPairFunc(Uint8, Float32, uint8_t, float);
        default: COMMON_THROW(BaseException, u"unsupported dataType!");
        }
#undef DtPairFunc
    }
    else // float -> non-float
    {
#define DtPairFunc(sDT, dDT, sT, dT) case DtPair(ImgDType::DataTypes::sDT, ImgDType::DataTypes::dDT): cvter.GetCopy().CopyFromFloat(reinterpret_cast<dT*>(newimg.Data), reinterpret_cast<const sT*>(Data), size, floatRange); break
        switch (DtPair(origType, dataType))
        {
        DtPairFunc(Float32, Uint8, float, uint8_t);
        default: COMMON_THROW(BaseException, u"unsupported dataType!");
        }
#undef DtPairFunc
    }
    return newimg;
}

template<typename T1, typename T2, typename T4>
static forceinline void ExtractImgChannel(Image& newimg, const Image& img, uint8_t channel)
{
    using T3 = T1;
    const auto chCount = img.GetDataType().ChannelCount();
    Expects(chCount > 1 && chCount < 5);
    const auto& cvter = ColorConvertor::Get();
    const auto count = img.PixelCount();
    switch (chCount)
    {
    case 2:
        if (channel == 0u)
            cvter.GrayAToGray(newimg.GetRawPtr<T1>(), img.GetRawPtr<T2>(), count);
        else
            cvter.GrayAToAlpha(newimg.GetRawPtr<T1>(), img.GetRawPtr<T2>(), count);
        break;
    case 3:
        cvter.RGBGetChannel(newimg.GetRawPtr<T1>(), img.GetRawPtr<T3>(), count, channel);
        break;
    case 4:
        cvter.RGBAGetChannel(newimg.GetRawPtr<T1>(), img.GetRawPtr<T4>(), count, channel);
        break;
    default: CM_UNREACHABLE();
    }
}
Image Image::ExtractChannel(uint8_t channel) const
{
    const auto chCount = DataType.ChannelCount();
    if (channel >= chCount)
        COMMON_THROW(BaseException, u"invalid channel index when extract channel");

    if (DataType.IsBGROrder())
    {
        if (channel == 0) channel = 2;
        else if (channel == 2) channel = 0;
    }

    if (DataType == ImageDataType::GRAY) return *this;
    Ensures(chCount > 1u);

    Image newimg(ImgDType{ ImgDType::Channels::R, DataType.DataType() });
    newimg.SetSize(Width, Height);
    switch (DataType.DataType())
    {
    case ImgDType::DataTypes::Uint16:
    case ImgDType::DataTypes::Fixed16:
    case ImgDType::DataTypes::Float16: ExtractImgChannel<uint16_t, uint16_t, uint16_t>(newimg, *this, channel); break;
    case ImgDType::DataTypes::Float32: ExtractImgChannel<   float,    float,    float>(newimg, *this, channel); break;
    case ImgDType::DataTypes::Uint8:   ExtractImgChannel< uint8_t, uint16_t, uint32_t>(newimg, *this, channel); break;
    default: COMMON_THROW(BaseException, u"unsupported datatype!");
    }
    return newimg;
}

template<typename T1, typename T2, typename T4>
static forceinline std::vector<Image> ExtractImgChannels(const Image& img, const uint32_t chCount)
{
    using T3 = T1;
    Expects(chCount > 1 && chCount < 5);
    std::vector<Image> ret;
    const auto w = img.GetWidth(), h = img.GetHeight();
    const auto count = w * h;
    ret.resize(chCount, ImgDType{ ImgDType::Channels::R, img.GetDataType().DataType() });
    T1* ptrs[4] = { nullptr };
    for (uint32_t i = 0; i < chCount; ++i)
    {
        auto& item = ret[i];
        ret[i].SetSize(w, h);
        ptrs[i] = item.GetRawPtr<T1>();
    }
    const auto& cvter = ColorConvertor::Get();
    switch (chCount)
    {
    case 2: cvter.RAToPlanar  (common::span<T1* const, 2>{ ptrs, 2 }, img.GetRawPtr<T2>(), count); break;
    case 3: cvter.RGBToPlanar (common::span<T1* const, 3>{ ptrs, 3 }, img.GetRawPtr<T3>(), count); break;
    case 4: cvter.RGBAToPlanar(common::span<T1* const, 4>{ ptrs, 4 }, img.GetRawPtr<T4>(), count); break;
    default: CM_UNREACHABLE();
    }
    return ret;
}
std::vector<Image> Image::ExtractChannels() const
{
    const auto chCount = DataType.ChannelCount();
    if (chCount == 1) 
        return { *this };

    switch (DataType.DataType())
    {
    case ImgDType::DataTypes::Uint16:
    case ImgDType::DataTypes::Fixed16:
    case ImgDType::DataTypes::Float16: return ExtractImgChannels<uint16_t, uint16_t, uint16_t>(*this, chCount);
    case ImgDType::DataTypes::Float32: return ExtractImgChannels<   float,    float,    float>(*this, chCount);
    case ImgDType::DataTypes::Uint8:   return ExtractImgChannels< uint8_t, uint16_t, uint32_t>(*this, chCount);
    default: break;
    }
    COMMON_THROW(BaseException, u"unsupported datatype!");
}

template<typename T1, typename T2, typename T4>
static forceinline Image CombineImgChannels(const ImgDType dataType, common::span<const ImageView> channels, const std::pair<uint32_t, uint32_t>& wh)
{
    using T3 = T1;
    const auto chCount = channels.size();
    Expects(chCount > 1 && chCount < 5);
    const auto count = wh.first * wh.second;
    Image ret(dataType);
    ret.SetSize(wh.first, wh.second, false);
    
    const T1* ptrs[4] = { nullptr };
    for (uint32_t i = 0; i < channels.size(); ++i)
        ptrs[i] = channels[i].GetRawPtr<T1>();
    const auto& cvter = ColorConvertor::Get();
    switch (chCount)
    {
    case 2: cvter.PlanarToRA  (ret.GetRawPtr<T2>(), common::span<const T1* const, 2>{ ptrs, 2 }, count); break;
    case 3: cvter.PlanarToRGB (ret.GetRawPtr<T3>(), common::span<const T1* const, 3>{ ptrs, 3 }, count); break;
    case 4: cvter.PlanarToRGBA(ret.GetRawPtr<T4>(), common::span<const T1* const, 4>{ ptrs, 4 }, count); break;
    default: CM_UNREACHABLE();
    }
    return ret;
}
Image Image::CombineChannels(const ImgDType dataType, common::span<const ImageView> channels)
{
    if (channels.empty())
        COMMON_THROW(BaseException, u"no input!");
    const auto baseType = ImgDType(ImgDType::Channels::R, dataType.DataType());
    std::optional<std::pair<uint32_t, uint32_t>> wh;
    for (const auto& ch : channels)
    {
        if (ch.GetDataType() != baseType)
            COMMON_THROW(BaseException, u"only accept single channel image as input!");
        if (!wh)
            wh = { ch.GetWidth(), ch.GetHeight() };
        else if (wh->first != ch.GetWidth() || wh->second != ch.GetHeight())
            COMMON_THROW(BaseException, u"unmatch size in input channels!");
    }
    Ensures(wh);

    uint32_t chCount = dataType.ChannelCount();
    if (chCount != channels.size())
        COMMON_THROW(BaseException, u"unmacth channel count in input!");
    if (chCount == 1)
        return channels[0];

    switch (dataType.DataType())
    {
    case ImgDType::DataTypes::Uint16:
    case ImgDType::DataTypes::Fixed16:
    case ImgDType::DataTypes::Float16: return CombineImgChannels<uint16_t, uint16_t, uint16_t>(dataType, channels, *wh);
    case ImgDType::DataTypes::Float32: return CombineImgChannels<   float,    float,    float>(dataType, channels, *wh);
    case ImgDType::DataTypes::Uint8:   return CombineImgChannels< uint8_t, uint16_t, uint32_t>(dataType, channels, *wh);
    default: break;
    }
    COMMON_THROW(BaseException, u"unsupported datatype!");
}


struct TempHolder final : public common::AlignedBuffer::ExternBufInfo
{
    common::span<std::byte> Space;
    TempHolder(common::span<std::byte> space) : Space(space) {}
    ~TempHolder() final {}
    [[nodiscard]] size_t GetSize() const noexcept final { return Space.size(); }
    [[nodiscard]] std::byte* GetPtr() const noexcept final { return Space.data(); }
};
Image Image::CreateViewFromTemp(common::span<std::byte> space, const ImgDType dataType, uint32_t w, uint32_t h)
{
    const auto size = static_cast<size_t>(w) * h * dataType.ElementSize();
    if (size > space.size())
        COMMON_THROW(BaseException, common::str::Formatter<char16_t>{}.FormatStatic(FmtString(u"given size [{}] too small for [{}x{}][] image(expects [{}])"),
            space.size(), w, h, dataType, size));
    return { common::AlignedBuffer::CreateBuffer(std::make_unique<TempHolder>(space.subspan(0, size))), w, h, dataType };
}

}

