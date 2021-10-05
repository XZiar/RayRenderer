#include "SystemCommonPch.h"
#include "StringFormat.h"
#include <ctime>

#include "fmt/src/format.cc"
#pragma message("Compiling StringUtil with fmt[" STRINGIZE(FMT_VERSION) "]" )

using common::str::Encoding;

FMT_BEGIN_NAMESPACE

namespace detail::temp
{

std::string& GetLocalString()
{
    static thread_local std::string buffer;
    return buffer;
}

}


FMT_END_NAMESPACE