#pragma once

#ifdef RESPAK_EXPORT
#   define RESPAKAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RESPAKAPI _declspec(dllimport)
#endif


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/FileEx.hpp"
#include "3rdParty/rapidjson/rapidjson.h"
#include "3rdParty/rapidjson/document.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <array>

namespace xziar::respak
{
namespace str = common::str;
namespace fs = common::fs;
using std::byte;
using std::wstring;
using std::string;
using std::u16string;
using std::array;
using common::BaseException;
using common::FileException;

}

#ifdef RESPAK_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace xziar::respak
{
common::mlog::MiniLogger<false>& rpakLog();
}
#endif
