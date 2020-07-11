#include "Format.h"
#include <ctime>

#include "3rdParty/fmt/format.cc"

using common::str::Charset;

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