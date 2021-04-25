#pragma once

#include "StringUtilRely.h"
#include "common/StrBase.hpp"


namespace common::str
{

namespace detail
{
class Uchardet;
}

#if COMMON_COMPILER_MSVC
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
    bool HandleData(const common::span<const std::byte> data) const;
    void Reset() const;
    void EndData() const;
    std::string GetEncoding() const;
    common::str::Charset DetectEncoding() const
    {
        return common::str::toCharset(this->GetEncoding());
    }
};
STRCHSETAPI std::string GetEncoding(const common::span<const std::byte> data);

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


template<typename T>
inline common::str::Charset DetectEncoding(const T& cont)
{
    return common::str::toCharset(GetEncoding(detail::ToByteSpan(cont)));
}


}

