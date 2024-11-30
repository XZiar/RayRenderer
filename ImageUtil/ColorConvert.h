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
    void(*GfToGAf           )(float* __restrict dest, const float* __restrict src, size_t count, float alpha) noexcept = nullptr;
    void(*GfToRGBf          )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*GfToRGBAf         )(float* __restrict dest, const float* __restrict src, size_t count, float alpha) noexcept = nullptr;
    void(*GAfToRGBf         )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*GAfToRGBAf        )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToRGBA8       )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*BGR8ToRGBA8       )(uint32_t* __restrict dest, const uint8_t*  __restrict src, size_t count, std::byte alpha) noexcept = nullptr;
    void(*RGBA8ToRGB8       )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToBGR8       )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBfToRGBAf       )(float* __restrict dest, const float* __restrict src, size_t count, float alpha) noexcept = nullptr;
    void(*BGRfToRGBAf       )(float* __restrict dest, const float* __restrict src, size_t count, float alpha) noexcept = nullptr;
    void(*RGBAfToRGBf       )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToBGRf       )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToBGR8        )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToBGRA8      )(uint32_t* __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBfToBGRf        )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToBGRAf      )(float* __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToR8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToG8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToB8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBA8ToA8         )(uint8_t*  __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToR8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToG8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGB8ToB8          )(uint8_t*  __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToRf         )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToGf         )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToBf         )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBAfToAf         )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBfToRf          )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBfToGf          )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*RGBfToBf          )(float*  __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*Extract8x2        )(uint8_t* const * __restrict dest, const uint16_t* __restrict src, size_t count) noexcept = nullptr;
    void(*Extract8x3        )(uint8_t* const * __restrict dest, const uint8_t*  __restrict src, size_t count) noexcept = nullptr;
    void(*Extract8x4        )(uint8_t* const * __restrict dest, const uint32_t* __restrict src, size_t count) noexcept = nullptr;
    void(*Extract32x2       )(float* const * __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*Extract32x3       )(float* const * __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*Extract32x4       )(float* const * __restrict dest, const float* __restrict src, size_t count) noexcept = nullptr;
    void(*Combine8x2        )(uint16_t* __restrict dest, const uint8_t* const * __restrict src, size_t count) noexcept = nullptr;
    void(*Combine8x3        )(uint8_t*  __restrict dest, const uint8_t* const * __restrict src, size_t count) noexcept = nullptr;
    void(*Combine8x4        )(uint32_t* __restrict dest, const uint8_t* const * __restrict src, size_t count) noexcept = nullptr;
    void(*Combine32x2       )(float* __restrict dest, const float* const * __restrict src, size_t count) noexcept = nullptr;
    void(*Combine32x3       )(float* __restrict dest, const float* const * __restrict src, size_t count) noexcept = nullptr;
    void(*Combine32x4       )(float* __restrict dest, const float* const * __restrict src, size_t count) noexcept = nullptr;
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
    void(*RGB10ToRGBf       )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
    void(*BGR10ToRGBf       )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
    void(*RGB10ToRGBAf      )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
    void(*BGR10ToRGBAf      )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
    void(*RGB10A2ToRGBAf    )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
    void(*BGR10A2ToRGBAf    )(float*    __restrict dest, const uint32_t* __restrict src, size_t count, float mulVal) noexcept = nullptr;
public:
    IMGUTILAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    IMGUTILAPI ColorConvertor(common::span<const VarItem> requests = {}) noexcept;
    IMGUTILAPI ~ColorConvertor();
    IMGUTILAPI [[nodiscard]] bool IsComplete() const noexcept final;
    constexpr const common::CopyManager& GetCopy() const noexcept { return CopyEx; }

    forceinline void GrayToGrayA(uint16_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        G8ToGA8(dest, src, count, alpha);
    }
    forceinline void GrayToGrayA(float* const dest, const float* src, const size_t count, const float alpha = 1.0f) const noexcept
    {
        GfToGAf(dest, src, count, alpha);
    }

    forceinline void GrayToRGB(uint8_t* const dest, const uint8_t* src, const size_t count) const noexcept
    {
        G8ToRGB8(dest, src, count);
    }
    forceinline void GrayToRGB(float* const dest, const float* src, const size_t count) const noexcept
    {
        GfToRGBf(dest, src, count);
    }

    forceinline void GrayToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        G8ToRGBA8(dest, src, count, alpha);
    }
    forceinline void GrayToRGBA(float* const dest, const float* src, const size_t count, const float alpha = 1.0f) const noexcept
    {
        GfToRGBAf(dest, src, count, alpha);
    }

    forceinline void GrayAToRGB(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        GA8ToRGB8(dest, src, count);
    }
    forceinline void GrayAToRGB(float* const dest, const float* src, const size_t count) const noexcept
    {
        GAfToRGBf(dest, src, count);
    }

    forceinline void GrayAToRGBA(uint32_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        GA8ToRGBA8(dest, src, count);
    }
    forceinline void GrayAToRGBA(float* const dest, const float* src, const size_t count) const noexcept
    {
        GAfToRGBAf(dest, src, count);
    }

    forceinline void GrayAToGray(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        CopyEx.TruncCopy(dest, src, count);
    }
    forceinline void GrayAToGray(float* const dest, const float* src, const size_t count) const noexcept
    {
        CopyEx.TruncCopy(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint64_t*>(src), count);
    }

    forceinline void GrayAToAlpha(uint8_t* const dest, const uint16_t* src, const size_t count) const noexcept
    {
        if (count > 0)
        {
            const auto src_ = reinterpret_cast<const uint8_t*>(src);
            CopyEx.TruncCopy(dest, reinterpret_cast<const uint16_t*>(src_ + 1), count - 1);
            dest[count - 1] = src_[count * 2 - 1];
        }
    }
    forceinline void GrayAToAlpha(float* const dest, const float* src, const size_t count) const noexcept
    {
        if (count > 0)
        {
            CopyEx.TruncCopy(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint64_t*>(src + 1), count);
            dest[count - 1] = src[count * 2 - 1];
        }
    }

    forceinline void RGBToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        RGB8ToRGBA8(dest, src, count, alpha);
    }
    forceinline void BGRToRGBA(uint32_t* const dest, const uint8_t* src, const size_t count, const std::byte alpha = std::byte(0xff)) const noexcept
    {
        BGR8ToRGBA8(dest, src, count, alpha);
    }
    forceinline void RGBToRGBA(float* const dest, const float* src, const size_t count, const float alpha = 1.0f) const noexcept
    {
        RGBfToRGBAf(dest, src, count, alpha);
    }
    forceinline void BGRToRGBA(float* const dest, const float* src, const size_t count, const float alpha = 1.0f) const noexcept
    {
        BGRfToRGBAf(dest, src, count, alpha);
    }

    forceinline void RGBAToRGB(uint8_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToRGB8(dest, src, count);
    }
    forceinline void RGBAToBGR(uint8_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToBGR8(dest, src, count);
    }
    forceinline void RGBAToRGB(float* const dest, const float* src, const size_t count) const noexcept
    {
        RGBAfToRGBf(dest, src, count);
    }
    forceinline void RGBAToBGR(float* const dest, const float* src, const size_t count) const noexcept
    {
        RGBAfToBGRf(dest, src, count);
    }

    forceinline void RGBToBGR(uint8_t* const dest, const uint8_t* src, const size_t count) const noexcept
    {
        RGB8ToBGR8(dest, src, count);
    }
    forceinline void RGBToBGR(float* const dest, const float* src, const size_t count) const noexcept
    {
        RGBfToBGRf(dest, src, count);
    }

    forceinline void RGBAToBGRA(uint32_t* const dest, const uint32_t* src, const size_t count) const noexcept
    {
        RGBA8ToBGRA8(dest, src, count);
    }
    forceinline void RGBAToBGRA(float* const dest, const float* src, const size_t count) const noexcept
    {
        RGBAfToBGRAf(dest, src, count);
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
    forceinline void RGBAGetChannel(float* const dest, const float* src, const size_t count, const uint8_t channel) const noexcept
    {
        switch (channel)
        {
        case 0: return RGBAfToRf(dest, src, count);
        case 1: return RGBAfToGf(dest, src, count);
        case 2: return RGBAfToBf(dest, src, count);
        case 3: return RGBAfToAf(dest, src, count);
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
    forceinline void RGBGetChannel(float* const dest, const float* src, const size_t count, const uint8_t channel) const noexcept
    {
        switch (channel)
        {
        case 0: return RGBfToRf(dest, src, count);
        case 1: return RGBfToGf(dest, src, count);
        case 2: return RGBfToBf(dest, src, count);
        default: return;
        }
    }

    forceinline void RAToPlanar(common::span<uint8_t* const, 2> dest, const uint16_t* src, const size_t count) const noexcept
    {
        Extract8x2(dest.data(), src, count);
    }
    forceinline void RGBToPlanar(common::span<uint8_t* const, 3> dest, const uint8_t* src, const size_t count) const noexcept
    {
        Extract8x3(dest.data(), src, count);
    }
    forceinline void RGBAToPlanar(common::span<uint8_t* const, 4> dest, const uint32_t* src, const size_t count) const noexcept
    {
        Extract8x4(dest.data(), src, count);
    }
    forceinline void RAToPlanar(common::span<float* const, 2> dest, const float* src, const size_t count) const noexcept
    {
        Extract32x2(dest.data(), src, count);
    }
    forceinline void RGBToPlanar(common::span<float* const, 3> dest, const float* src, const size_t count) const noexcept
    {
        Extract32x3(dest.data(), src, count);
    }
    forceinline void RGBAToPlanar(common::span<float* const, 4> dest, const float* src, const size_t count) const noexcept
    {
        Extract32x4(dest.data(), src, count);
    }

    forceinline void PlanarToRA(uint16_t* dest, common::span<const uint8_t* const, 2> src, const size_t count) const noexcept
    {
        Combine8x2(dest, src.data(), count);
    }
    forceinline void PlanarToRGB(uint8_t* dest, common::span<const uint8_t* const, 3> src, const size_t count) const noexcept
    {
        Combine8x3(dest, src.data(), count);
    }
    forceinline void PlanarToRGBA(uint32_t* dest, common::span<const uint8_t* const, 4> src, const size_t count) const noexcept
    {
        Combine8x4(dest, src.data(), count);
    }
    forceinline void PlanarToRA(float* dest, common::span<const float* const, 2> src, const size_t count) const noexcept
    {
        Combine32x2(dest, src.data(), count);
    }
    forceinline void PlanarToRGB(float* dest, common::span<const float* const, 3> src, const size_t count) const noexcept
    {
        Combine32x3(dest, src.data(), count);
    }
    forceinline void PlanarToRGBA(float* dest, common::span<const float* const, 4> src, const size_t count) const noexcept
    {
        Combine32x4(dest, src.data(), count);
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

    forceinline void RGB10A2ToRGB(float* const dest, const uint32_t* src, const size_t count, const float range = 0) const noexcept
    {
        const float mulVal = range == 0 ? 0 : range / 1023.f;
        RGB10ToRGBf(dest, src, count, mulVal);
    }
    forceinline void BGR10A2ToRGB(float* const dest, const uint32_t* src, const size_t count, const float range = 0) const noexcept
    {
        const float mulVal = range == 0 ? 0 : range / 1023.f;
        BGR10ToRGBf(dest, src, count, mulVal);
    }
    forceinline void RGB10A2ToRGBA(float* const dest, const uint32_t* src, const size_t count, const float range = 0, const bool hasAlpha = false) const noexcept
    {
        const float mulVal = range == 0 ? 0 : range / 1023.f;
        (hasAlpha ? RGB10A2ToRGBAf : RGB10ToRGBAf)(dest, src, count, mulVal);
    }
    forceinline void BGR10A2ToRGBA(float* const dest, const uint32_t* src, const size_t count, const float range = 0, const bool hasAlpha = false) const noexcept
    {
        const float mulVal = range == 0 ? 0 : range / 1023.f;
        (hasAlpha ? BGR10A2ToRGBAf : BGR10ToRGBAf)(dest, src, count, mulVal);
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