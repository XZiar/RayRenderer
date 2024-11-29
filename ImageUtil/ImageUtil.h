#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"
#include "common/FileBase.hpp"


namespace xziar::img
{

[[nodiscard]] IMGUTILAPI Image ReadImage(const common::fs::path& path, const ImgDType dataType = ImageDataType::RGBA);
[[nodiscard]] IMGUTILAPI Image ReadImage(common::io::RandomInputStream& stream, const std::u16string& ext, const ImgDType dataType = ImageDataType::RGBA);
template<typename T>
[[nodiscard]] Image ReadImage(const std::vector<T>& data, const std::u16string& ext, const ImgDType dataType = ImageDataType::RGBA)
{
    common::io::ContainerInputStream<std::vector<T>> stream(data);
    return ReadImage(stream, ext, dataType);
}

IMGUTILAPI void WriteImage(const Image& image, const common::fs::path& path, const uint8_t quality = 90);
IMGUTILAPI void WriteImage(const Image& image, common::io::RandomOutputStream& stream, const std::u16string& ext, const uint8_t quality = 90);
template<typename T>
[[nodiscard]] std::vector<T> WriteImage(const Image& image, const std::u16string& ext, const uint8_t quality = 90)
{
    static_assert(sizeof(T) == 1, "only accept 1 byte element type");
    std::vector<T> output;
    common::io::ContainerOutputStream<std::vector<T>> stream(output);
    WriteImage(image, stream, ext, quality);
    return output;
}

}