#pragma once

#include <cstdint>
#include <cstdio>
#include <ios>
#include <vector>
#include <string>
#include <tuple>


namespace stb
{

std::tuple<int32_t, int32_t> loadImage(const std::wstring& fname, std::vector<uint32_t>& data);

std::vector<uint32_t> resizeImage(const std::vector<uint32_t>& input, const uint32_t inw, const uint32_t inh, const uint32_t width, const uint32_t height);

void saveImage(const std::wstring& fname, const std::vector<uint32_t>& data, const uint32_t width, const uint32_t height);

}
