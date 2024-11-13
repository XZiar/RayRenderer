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
    void(*G8ToGA8           )(uint16_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*G8ToRGB8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*G8ToRGBA8         )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*GA8ToRGB8         )(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*GA8ToRGBA8        )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToRGBA8       )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*BGR8ToRGBA8       )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*RGBA8ToRGB8       )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToBGR8       )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToBGR8        )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToBGRA8      )(uint32_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToR8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToG8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToB8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToA8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToR8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToG8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToB8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToRA8        )(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToGA8        )(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToBA8        )(uint16_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB555ToRGB8      )(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*BGR555ToRGB8      )(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB555ToRGBA8     )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*BGR555ToRGBA8     )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB5551ToRGBA8    )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*BGR5551ToRGBA8    )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB565ToRGB8      )(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*BGR565ToRGB8      )(uint8_t*  __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB565ToRGBA8     )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*BGR565ToRGBA8     )(uint32_t* __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
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

    forceinline void GrayAToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        GA8ToRGB8(dest, src, count);
    }

    forceinline void GrayAToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        GA8ToRGBA8(dest, src, count);
    }

    forceinline void GrayAToGray(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        CopyEx.TruncCopy(dest, src, count);
    }

    forceinline void RGBToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        RGB8ToRGBA8(dest, src, count, alpha);
    }

    forceinline void BGRToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        BGR8ToRGBA8(dest, src, count, alpha);
    }

    forceinline void RGBAToRGB(uint8_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToRGB8(dest, src, count);
    }

    forceinline void RGBAToBGR(uint8_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToBGR8(dest, src, count);
    }

    forceinline void RGBToBGR(uint8_t* const dest, const uint8_t* src, const size_t count) const noexcept
    {
        RGB8ToBGR8(dest, src, count);
    }

    forceinline void RGBAToBGRA(uint32_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToBGRA8(dest, src, count);
    }

    forceinline void RGBAGetChannel(uint8_t* const dest, const uint32_t* src, const size_t count, const uint8_t channel) const noexcept
    {
        switch (channel)
        {
        case 0: return RGBA8ToR8(dest, src, count);
        case 1: return RGBA8ToG8(dest, src, count);
        case 2: return RGBA8ToB8(dest, src, count);
        case 3: return RGBA8ToA8(dest, src, count);
        default: return;
        }
    }

    forceinline void RGBGetChannel(uint8_t* const dest, const uint8_t* src, const size_t count, const uint8_t channel) const noexcept
    {
        switch (channel)
        {
        case 0: return RGB8ToR8(dest, src, count);
        case 1: return RGB8ToG8(dest, src, count);
        case 2: return RGB8ToB8(dest, src, count);
        default: return;
        }
    }

    forceinline void RGBAGetChannelAlpha(uint16_t* const dest, const uint32_t* src, const size_t count, const uint8_t channel) const noexcept
    {
        switch (channel)
        {
        case 0: return RGBA8ToRA8(dest, src, count);
        case 1: return RGBA8ToGA8(dest, src, count);
        case 2: return RGBA8ToBA8(dest, src, count);
        default: return;
        }
    }

    forceinline void RGB555ToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        RGB555ToRGB8(dest, src, count);
    }
    forceinline void BGR555ToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        BGR555ToRGB8(dest, src, count);
    }

    forceinline void RGB555ToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count, const bool hasAlpha = false) const noexcept
    {
        (hasAlpha ? RGB5551ToRGBA8 : RGB555ToRGBA8)(dest, src, count);
    }
    forceinline void BGR555ToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count, const bool hasAlpha = false) const noexcept
    {
        (hasAlpha ? BGR5551ToRGBA8 : BGR555ToRGBA8)(dest, src, count);
    }

    forceinline void RGB565ToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        RGB565ToRGB8(dest, src, count);
    }
    forceinline void BGR565ToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        BGR565ToRGB8(dest, src, count);
    }

    forceinline void RGB565ToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        RGB565ToRGBA8(dest, src, count);
    }
    forceinline void BGR565ToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        BGR565ToRGBA8(dest, src, count);
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