#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <tuple>


namespace stb
{

std::tuple<int32_t, int32_t> loadImage(const std::wstring& fname, std::vector<uint32_t>& data);

std::vector<uint32_t> resizeImage(const std::vector<uint32_t>& input, const uint32_t inw, const uint32_t inh, const uint32_t width, const uint32_t height);

void saveImage(const std::wstring& fname, const void *data, const uint32_t width, const uint32_t height, const uint8_t compCount = 4);
inline void saveImage(const std::wstring& fname, const std::vector<uint32_t>& data, const uint32_t width, const uint32_t height)
{
	saveImage(fname, data.data(), width, height, 4);
}

std::vector<uint32_t> compressTextureBC1(const std::vector<uint32_t>& input, uint32_t w, uint32_t h);
std::vector<uint32_t> compressTextureBC2(const std::vector<uint32_t>& input, uint32_t w, uint32_t h);

}
