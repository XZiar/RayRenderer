#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
# else
#   define IMGUTILAPI _declspec(dllimport)
# endif
#else
# define IMGUTILAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/EnumEx.hpp"
#include "SystemCommon/Exceptions.h"
#include "common/MemoryStream.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>
#include <array>

namespace xziar::img
{

enum class YCCMatrix : uint8_t { BT601 = 0, BT709, BT2020, Invalid, TypeMask = 0x03, RGBFull = 0x04, YCCFull = 0x08 };
MAKE_ENUM_BITFIELD(YCCMatrix)

inline constexpr YCCMatrix EncodeYCCM(YCCMatrix matrix, bool isRGBFull, bool isYCCFull) noexcept
{
    matrix = matrix & YCCMatrix::TypeMask;
    if (isRGBFull) matrix |= YCCMatrix::RGBFull;
    if (isYCCFull) matrix |= YCCMatrix::YCCFull;
    return matrix;
}

inline constexpr std::array<double, 3> RGB2YCCBase[] =
{
    { 0.299,  0.587,  0.114  }, // BT.601
    { 0.2126, 0.7152, 0.0722 }, // BT.709
    { 0.2627, 0.6780, 0.0593 }, // BT.2020
};

template<typename T = float>
inline constexpr std::array<T, 9> ComputeRGB2YCCMatrix8F(YCCMatrix mat) noexcept
{
    static_assert(std::is_floating_point_v<T>, "need FP");
    const bool isRGBFull = HAS_FIELD(mat, YCCMatrix::RGBFull);
    const bool isYCCFull = HAS_FIELD(mat, YCCMatrix::YCCFull);
    const double scaleY = isRGBFull ?
        (isYCCFull ? 1.0 : (235 - 16 + 1) * 1.0 / (255 - 0 + 1)) :
        (isYCCFull ? (255 - 0 + 1) * 1.0 / (235 - 16 + 1) : 1.0);
    const double scaleC = isRGBFull ?
        (isYCCFull ? 1.0 : (240 - 16 + 1) * 1.0 / (255 - 0 + 1)) :
        (isYCCFull ? (255 - 0 + 1) * 1.0 / (235 - 16 + 1) : (240 - 16 + 1) * 1.0 / (235 - 16 + 1));
    const auto& coeff = RGB2YCCBase[common::enum_cast(mat & YCCMatrix::TypeMask)];
    const auto pbCoeff = 0.5 / (1.0 - coeff[2]), prCoeff = 0.5 / (1.0 - coeff[0]);

    return std::array<T, 9>{
        static_cast<T>(coeff[0] * scaleY), static_cast<T>(coeff[1] * scaleY), static_cast<T>(coeff[2] * scaleY),
        static_cast<T>(coeff[0] * -pbCoeff * scaleC), static_cast<T>(coeff[1] * -pbCoeff * scaleC), static_cast<T>(0.5 * scaleC),
        static_cast<T>(0.5 * scaleC), static_cast<T>(coeff[1] * -prCoeff * scaleC), static_cast<T>(coeff[2] * -prCoeff * scaleC) };
}

template<typename T = float>
inline constexpr std::array<T, 9> ComputeYCC2RGBMatrix8F(YCCMatrix mat) noexcept
{
    static_assert(std::is_floating_point_v<T>, "need FP");
    const bool isRGBFull = HAS_FIELD(mat, YCCMatrix::RGBFull);
    const bool isYCCFull = HAS_FIELD(mat, YCCMatrix::YCCFull);
    const double scaleY = isRGBFull ?
        (isYCCFull ? 1.0 : (255 - 0 + 1) * 1.0 / (235 - 16 + 1)) :
        (isYCCFull ? (235 - 16 + 1) * 1.0 / (255 - 0 + 1) : 1.0);
    const double scaleC = isRGBFull ?
        (isYCCFull ? 1.0 : (255 - 0 + 1) * 1.0 / (240 - 16 + 1)) :
        (isYCCFull ? (235 - 16 + 1) * 1.0 / (255 - 0 + 1) : (235 - 16 + 1) * 1.0 / (240 - 16 + 1));
    const auto& coeff = RGB2YCCBase[common::enum_cast(mat & YCCMatrix::TypeMask)];
    const auto pbCoeff = 2.0 * (1.0 - coeff[2]), prCoeff = 2.0 * (1.0 - coeff[0]);
    const auto pbGCoeff = pbCoeff * coeff[2] / coeff[1], prGCoeff = prCoeff * coeff[0] / coeff[1];

    return std::array<T, 9>{
        static_cast<T>(scaleY), T(0), static_cast<T>(prCoeff * scaleC),
        static_cast<T>(scaleY), static_cast<T>(pbGCoeff * scaleC), static_cast<T>(prGCoeff * scaleC),
        static_cast<T>(scaleY), static_cast<T>(pbCoeff * scaleC), T(0)};
}

}
