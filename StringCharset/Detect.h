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
inline common::str::Charset DetectEncoding(const T& cont)
{
    using Helper = common::container::ContiguousHelper<T>;
    static_assert(Helper::IsContiguous, "only accept contiguous container as input");
    return common::str::toCharset(GetEncoding(Helper::Data(cont), Helper::Count(cont) * Helper::EleSize));
}


}

