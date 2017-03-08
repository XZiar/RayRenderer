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

}
