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
    void(*G8ToGA8    )(uint16_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*G8ToRGB8   )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*G8ToRGBA8  )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*GA8ToRGBA8 )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToRGBA8)(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*RGBA8ToRGB8)(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
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

    forceinline void GrayAToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        GA8ToRGBA8(dest, src, count);
    }

    forceinline void RGBToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        RGB8ToRGBA8(dest, src, count, alpha);
    }

    forceinline void GrayAToGray(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        CopyEx.TruncCopy(dest, src, count);
    }

    forceinline void RGBAToRGB(uint8_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToRGB8(dest, src, count);
    }

    IMGUTILAPI static const ColorConvertor& Get() noexcept;
};


class STBResize final : public common::RuntimeFastPath<STBResize>
{
    friend ::common::fastpath::PathHack;
public:
    struct ResizeInfo
    {
        const void* Input;
        void* Output;
        std::pair<uint32_t, uint32_t> InputSizes;
        std::pair<uint32_t, uint32_t> OutputSizes;
        uint8_t Layout;
        uint8_t Datatype;
        uint8_t Edge;
        uint8_t Filter;
    };
private:
    void(*DoResize)(const ResizeInfo& info) noexcept = nullptr;
public:
    IMGUTILAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    IMGUTILAPI STBResize(common::span<const VarItem> requests = {}) noexcept;
    IMGUTILAPI ~STBResize();
    IMGUTILAPI [[nodiscard]] bool IsComplete() const noexcept final;

    forceinline void Resize(const ResizeInfo& info) const noexcept
    {
        DoResize(info);
    }

    IMGUTILAPI static const STBResize& Get() noexcept;
};


}