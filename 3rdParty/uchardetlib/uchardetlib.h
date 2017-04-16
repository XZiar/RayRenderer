#pragma once

#include "../../common/StringEx.hpp"
#include <vector>
#include <string>

namespace uchdet
{


common::Charset detectEncoding(const std::vector<uint8_t>& str);
common::Charset detectEncoding(const std::string& str);
common::Charset detectEncoding(const std::wstring& str);
std::string getEncoding(const void *data, const size_t len);


}