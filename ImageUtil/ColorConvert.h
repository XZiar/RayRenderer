#pragma once
#include "ImageUtilRely.h"
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/SystemCommonRely.h"

namespace xziar::img
{


class ColorConvertor final : public common::RuntimeFastPath<ColorConvertor>
{
    friend ::common::fastpath::PathHack;
private:
    common::CopyManager CopyEx;
    void(*G8ToGA8  )(uint16_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*G8ToRGB8 )(uint8_t*  __restrict dest, const uint8_t* __restrict src, size_t count) noexcept = nullptr;
    void(*G8ToRGBA8)(uint32_t* __restrict dest, const uint8_t* __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
public:
    IMGUTILAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    IMGUTILAPI ColorConvertor(common::span<const VarItem> requests = {}) noexcept;
    IMGUTILAPI ~ColorConvertor();
    IMGUTILAPI [[nodiscard]] bool IsComplete() const noexcept final;

    forceinline void GrayToGrayA(uint16_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        G8ToGA8(dest, src, count, alpha);
    }

    forceinline void GrayToRGB(uint8_t* const dest, const uint8_t* src, const size_t count) const noexcept
    {
        G8ToRGB8(dest, src, count);
    }

    forceinline void GrayToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        G8ToRGBA8(dest, src, count, alpha);
    }

    forceinline void GrayAToGray(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        CopyEx.TruncCopy(dest, src, count);
    }

    IMGUTILAPI static const ColorConvertor& Get() noexcept;
};


}