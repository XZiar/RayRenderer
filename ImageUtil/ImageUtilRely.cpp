#include "ImageUtilPch.h"
#include "SystemCommon/RuntimeFastPath.h"
#include <cmath>


#if COMMON_ARCH_X86
DEFINE_FASTPATH_SCOPE(AVX512)
{
    return common::CheckCPUFeature("avx512f");
}
DEFINE_FASTPATH_SCOPE(AVX2)
{
    return common::CheckCPUFeature("avx2");
}
DEFINE_FASTPATH_SCOPE(SSE42)
{
    return true;
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, AVX512, AVX2, SSE42)
DECLARE_FASTPATH_PARTIALS(YCCConvertor, AVX512, AVX2, SSE42)
DECLARE_FASTPATH_PARTIALS(STBResize, AVX2, SSE42)
#elif COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
DEFINE_FASTPATH_SCOPE(A64)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A64)
DECLARE_FASTPATH_PARTIALS(YCCConvertor, A64)
DECLARE_FASTPATH_PARTIALS(STBResize, A64)
#   else
DEFINE_FASTPATH_SCOPE(A32)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A32)
DECLARE_FASTPATH_PARTIALS(YCCConvertor, A32)
DECLARE_FASTPATH_PARTIALS(STBResize, A32)
#   endif
#endif


namespace xziar::img
{
using namespace common::mlog;
using common::enum_cast;
MiniLogger<false>& ImgLog()
{
    static MiniLogger<false> imglog(u"ImageUtil", { GetConsoleBackend() });
    return imglog;
}

template<bool ToYCC, typename T>
static inline auto ComputeYCCMatrix8(YCCMatrix mat, [[maybe_unused]] double scale) noexcept
{
    using U = std::conditional_t<std::is_floating_point_v<T>, T, double>;
    auto tmp = ToYCC ? ComputeRGB2YCCMatrix8F<U>(mat) : ComputeYCC2RGBMatrix8F<U>(mat);
    // compress
    if constexpr (ToYCC)
    {
        tmp[6] = tmp[7];
        tmp[7] = tmp[8];
    }
    if constexpr (std::is_floating_point_v<T>)
        return tmp;
    else
    {
        static_assert(std::is_integral_v<T>, "need INT");
        common::RegionRounding rd(common::RoundMode::ToNearest);
        std::array<T, 9> ret{};
        for (uint32_t i = 0; i < 9; ++i)
            ret[i] = static_cast<T>(std::nearbyint(tmp[i] * scale));
        return ret;
    }
}
template<bool ToYCC, typename T>
static constexpr auto GenYCC8LUT(double scale = 1) noexcept
{
    constexpr uint32_t N = std::is_same_v<T, int8_t> ? 16 : 9;
    std::array<std::array<T, N>, 16> ret = {};
#define Gen(mat, rgb, ycc) do { const auto mval = EncodeYCCM(YCCMatrix::mat, rgb, ycc); ret[enum_cast(mval)] = ComputeYCCMatrix8<ToYCC, T>(mval, scale); } while(0)
    Gen(BT601,  false, false);
    Gen(BT601,  true,  false);
    Gen(BT601,  false, true);
    Gen(BT601,  true,  true);
    Gen(BT709,  false, false);
    Gen(BT709,  true,  false);
    Gen(BT709,  false, true);
    Gen(BT709,  true,  true);
    Gen(BT2020, false, false);
    Gen(BT2020, true,  false);
    Gen(BT2020, false, true);
    Gen(BT2020, true,  true);
#undef Gen
    return ret;
}
template<typename F>
static constexpr auto GenYCC8LUT2(F&& func) noexcept
{
    using T = decltype(func(YCCMatrix::BT601));
    std::array<T, 16> ret = {};
#define Gen(mat, rgb, ycc) do { const auto mval = EncodeYCCM(YCCMatrix::mat, rgb, ycc); ret[enum_cast(mval)] = func(mval); } while(0)
    Gen(BT601,  false, false);
    Gen(BT601,  true,  false);
    Gen(BT601,  false, true);
    Gen(BT601,  true,  true);
    Gen(BT709,  false, false);
    Gen(BT709,  true,  false);
    Gen(BT709,  false, true);
    Gen(BT709,  true,  true);
    Gen(BT2020, false, false);
    Gen(BT2020, true,  false);
    Gen(BT2020, false, true);
    Gen(BT2020, true,  true);
#undef Gen
    return ret;
}

static inline auto ComputeRGB2YCCMatrixI8x4(YCCMatrix mat) noexcept
{
    auto tmp = ComputeRGB2YCCMatrix8F<double>(mat);
    // compress
    tmp[6] = tmp[7];
    tmp[7] = tmp[8];
    common::RegionRounding rd(common::RoundMode::ToNearest);
    std::array<int32_t, 8> mid{};
    for (uint32_t i = 0; i < 8; ++i)
        mid[i] = static_cast<int32_t>(std::nearbyint(tmp[i] * 256));
    std::array<int8_t, 16> ret{};
    // y: RGGB, G > R > B, ensure G0 + R <= 0.5
    ret[0] = static_cast<int8_t>(mid[0]);
    ret[1] = static_cast<int8_t>(128 - mid[0]); // max 0.5
    ret[2] = static_cast<int8_t>(mid[1] - ret[1]);
    ret[3] = static_cast<int8_t>(mid[2]);
    // cb: RBGB, B > G > R, B/2 ~ (0.25, 0.5), G ~ (-0.5, -0.25)
    ret[4] = static_cast<int8_t>(mid[3]);
    ret[5] = static_cast<int8_t>(mid[5] / 2);
    ret[6] = static_cast<int8_t>(mid[4]);
    ret[7] = static_cast<int8_t>(mid[5] - ret[5]);
    // cr: RGRB, R > G > B, R/2 ~ (0.25, 0.5), G ~ (-0.5, -0.25)
    ret[ 8] = static_cast<int8_t>(mid[5] / 2);
    ret[ 9] = static_cast<int8_t>(mid[6]);
    ret[10] = static_cast<int8_t>(mid[5] - ret[8]);
    ret[11] = static_cast<int8_t>(mid[7]);
    return ret;
}
static inline auto ComputeRGB2YCCMatrixU8x4(YCCMatrix mat) noexcept
{
    auto tmp = ComputeRGB2YCCMatrix8F<double>(mat);
    // compress
    tmp[6] = tmp[7];
    tmp[7] = tmp[8];
    common::RegionRounding rd(common::RoundMode::ToNearest);
    std::array<int32_t, 8> mid{};
    for (uint32_t i = 0; i < 3; ++i)
        mid[i] = static_cast<int32_t>(std::nearbyint(tmp[i] * 512));
    for (uint32_t i = 3; i < 8; ++i)
        mid[i] = static_cast<int32_t>(std::nearbyint(tmp[i] * 256 + 128));
    mid[5] = static_cast<int32_t>(std::nearbyint(tmp[5] * 256 + 256));
    std::array<uint8_t, 16> ret{};
    // y: RGGB, G > R > B, ensure G0 + R <= 0.5
    ret[0] = static_cast<uint8_t>(mid[0]);
    ret[1] = static_cast<uint8_t>(256 - mid[0]); // max 0.5
    ret[2] = static_cast<uint8_t>(mid[1] - ret[1]);
    ret[3] = static_cast<uint8_t>(mid[2]);
    // cb: RBGB, B > G > R, B/2 ~ (0.25, 0.5), G ~ (-0.5, -0.25)
    ret[4] = static_cast<uint8_t>(mid[3]);
    ret[5] = static_cast<uint8_t>(mid[5] / 2);
    ret[6] = static_cast<uint8_t>(mid[4]);
    ret[7] = static_cast<uint8_t>(mid[5] - ret[5]);
    // cr: RGRB, R > G > B, R/2 ~ (0.25, 0.5), G ~ (-0.5, -0.25)
    ret[ 8] = static_cast<uint8_t>(mid[5] / 2);
    ret[ 9] = static_cast<uint8_t>(mid[6]);
    ret[10] = static_cast<uint8_t>(mid[5] - ret[8]);
    ret[11] = static_cast<uint8_t>(mid[7]);
    for (uint32_t i = 12; i < 16; ++i)
        ret[i] = 128;
    return ret;
}
// Y,0,Bcb,Rcr,Gcb,Gcr
static inline auto ComputeYCC2RGBMatrixI16M(YCCMatrix mat) noexcept
{
    auto tmp = ComputeYCC2RGBMatrix8F<double>(mat);
    common::RegionRounding rd(common::RoundMode::ToNearest);
    std::array<int32_t, 9> mid{};
    mid[0] = static_cast<int32_t>(std::nearbyint(tmp[0] * 16384));
    for (uint32_t i = 1; i < 9; ++i)
        mid[i] = static_cast<int32_t>(std::nearbyint(tmp[i] * 8192));
    std::array<int16_t, 8> ret{};
    ret[0] = static_cast<int16_t>(mid[0]); // y
    ret[1] = 0;
    ret[2] = static_cast<int16_t>(mid[7]); // B->cb
    ret[3] = static_cast<int16_t>(mid[2]); // R->cr
    ret[4] = static_cast<int16_t>(mid[4]); // G->cb
    ret[5] = static_cast<int16_t>(mid[5]); // G->cr
    ret[6] = 0;
    ret[7] = 0;
    return ret;
}


//static auto GenC8LUT() noexcept
//{
//    const auto mid = GenYCC8LUT<float>();
//    std::array<std::array<float, 5>, 16> ret = {};
//    for (uint32_t i = 0, j = 0; i < 16; ++i)
//    {
//        if (mid[i][0] > 0)
//        {
//            ret[j][0] = mid[i][3];
//            ret[j][1] = mid[i][4];
//            ret[j][2] = mid[i][5];
//            ret[j][3] = mid[i][6];
//            ret[j][4] = mid[i][7];
//            printf(common::str::Formatter<char>{}.FormatStatic(FmtString("{:7.4f} {:7.4f} {:7.4f} {:7.4f} {:7.4f}\n"),
//                ret[j][0], ret[j][1], ret[j][2], ret[j][3], ret[j][4]).c_str());
//            j++;
//        }
//    }
//    return ret;
//}
//static const auto klp = GenC8LUT();


std::array<std::array<  float,  9>, 16> RGB8ToYCC8LUTF32 = GenYCC8LUT<true,   float>();
std::array<std::array<int16_t,  9>, 16> RGB8ToYCC8LUTI16 = GenYCC8LUT<true, int16_t>(16384);
//std::array<std::array<int16_t,  9>, 16> RGB8ToYCC8LUTI16_15 = GenYCC8LUT<int16_t>(32768);
std::array<std::array< int8_t, 16>, 16> RGB8ToYCC8LUTI8x4 = GenYCC8LUT2(&ComputeRGB2YCCMatrixI8x4);
std::array<std::array<uint8_t, 16>, 16> RGB8ToYCC8LUTU8x4 = GenYCC8LUT2(&ComputeRGB2YCCMatrixU8x4);
std::array<std::array<  float,  9>, 16> YCC8ToRGB8LUTF32 = GenYCC8LUT<false,   float>();
std::array<std::array<int16_t,  8>, 16> YCC8ToRGB8LUTI16 = GenYCC8LUT2(&ComputeYCC2RGBMatrixI16M); // Y,0,Bcb,Rcr,Gcb,Gcr


DEFINE_FASTPATH_BASIC(ColorConvertor,
    G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGB8, GA8ToRGBA8, G16ToGA16, G16ToRGB16, G16ToRGBA16, GA16ToRGB16, GA16ToRGBA16, GfToGAf, GfToRGBf, GfToRGBAf, GAfToRGBf, GAfToRGBAf,
    RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, RGB16ToRGBA16, BGR16ToRGBA16, RGBA16ToRGB16, RGBA16ToBGR16, RGBfToRGBAf, RGBfToRGBAf, BGRfToRGBAf, RGBAfToRGBf, RGBAfToBGRf,
    RGB8ToBGR8, RGBA8ToBGRA8, RGB16ToBGR16, RGBA16ToBGRA16, RGBfToBGRf, RGBAfToBGRAf,
    RGB8ToR8, RGB8ToG8, RGB8ToB8, RGB16ToR16, RGB16ToG16, RGB16ToB16, RGBAfToRf, RGBAfToGf, RGBAfToBf, RGBAfToAf, RGBfToRf, RGBfToGf, RGBfToBf,
    Extract8x2, Extract8x3, Extract8x4, Extract16x2, Extract16x3, Extract16x4, Extract32x2, Extract32x3, Extract32x4,
    Combine8x2, Combine8x3, Combine8x4, Combine16x2, Combine16x3, Combine16x4, Combine32x2, Combine32x3, Combine32x4,
    G8ToG16, G16ToG8,
    RGB555ToRGB8, BGR555ToRGB8, RGB555ToRGBA8, BGR555ToRGBA8, RGB5551ToRGBA8, BGR5551ToRGBA8,
    RGB565ToRGB8, BGR565ToRGB8, RGB565ToRGBA8, BGR565ToRGBA8,
    RGB10ToRGBf, BGR10ToRGBf, RGB10ToRGBAf, BGR10ToRGBAf, RGB10A2ToRGBAf, BGR10A2ToRGBAf)

const ColorConvertor& ColorConvertor::Get() noexcept
{
    static ColorConvertor convertor;
    return convertor;
}


DEFINE_FASTPATH_BASIC(YCCConvertor, RGB8ToYCbCr8Fast, RGB8ToYCbCr8, RGBA8ToYCbCr8Fast, RGBA8ToYCbCr8, YCbCr8ToRGB8, YCbCr8ToRGBA8, RGB8ToYCbCr8PlanarFast, RGB8ToYCbCr8Planar, RGBA8ToYCbCr8PlanarFast, RGBA8ToYCbCr8Planar)

const YCCConvertor& YCCConvertor::Get() noexcept
{
    static YCCConvertor convertor;
    return convertor;
}


DEFINE_FASTPATH_BASIC(STBResize, DoResize)

const STBResize& STBResize::Get() noexcept
{
    static STBResize resizer;
    return resizer;
}


}