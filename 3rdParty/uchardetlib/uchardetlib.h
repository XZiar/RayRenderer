#pragma once

#ifndef UCHARDETLIB_H_
#define UCHARDETLIB_H_

#include "common\StrCharset.hpp"
#include <vector>
#include <string>

namespace uchdet
{
using common::str::Charset;

Charset detectEncoding(const std::vector<uint8_t>& str);
Charset detectEncoding(const std::string& str);
Charset detectEncoding(const std::wstring& str);
std::string getEncoding(const void *data, const size_t len);


}

#endif