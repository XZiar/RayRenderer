#pragma once

#include "StringCharsetRely.h"
#include "common/StrBase.hpp"


namespace common::strchset
{

namespace detail
{
class Uchardet;
}

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4251)
#endif

class STRCHSETAPI CharsetDetector
{
private:
    std::unique_ptr<detail::Uchardet> Tool;
public:
    CharsetDetector();
    CharsetDetector(CharsetDetector&&) noexcept = default;
    ~CharsetDetector();
    CharsetDetector& operator=(CharsetDetector&&) noexcept = default;
    bool HandleData(const void *data, const size_t len) const;
    void Reset() const;
    void EndData() const;
    std::string GetEncoding() const;
    common::str::Charset DetectEncoding() const
    {
        return common::str::toCharset(GetEncoding());
    }
};
std::string STRCHSETAPI GetEncoding(const void *data, const size_t len);

#if COMPILER_MSVC
#   pragma warning(pop)
#endif


template<typename T>
common::str::Charset DetectEncoding(const std::basic_string<T>& str)
{
    return common::str::toCharset(GetEncoding(str.data(), str.size() * sizeof(T)));
}
template<typename T>
common::str::Charset DetectEncoding(const std::basic_string_view<T>& str)
{
    return common::str::toCharset(GetEncoding(str.data(), str.size() * sizeof(T)));
}
template<typename T, typename A>
common::str::Charset DetectEncoding(const std::vector<T, A>& str)
{
    return common::str::toCharset(GetEncoding(str.data(), str.size() * sizeof(T)));
}


}

