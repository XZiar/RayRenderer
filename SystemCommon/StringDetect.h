#pragma once

#include "SystemCommonRely.h"


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

class EncodingDetector
{
private:
    std::unique_ptr<detail::Uchardet> Tool;
public:
    SYSCOMMONAPI EncodingDetector();
    EncodingDetector(EncodingDetector&&) noexcept = default;
    SYSCOMMONAPI ~EncodingDetector();
    EncodingDetector& operator=(EncodingDetector&&) noexcept = default;
    SYSCOMMONAPI bool HandleData(const common::span<const std::byte> data) const;
    SYSCOMMONAPI void Reset() const;
    SYSCOMMONAPI void EndData() const;
    SYSCOMMONAPI std::string GetEncoding() const;
    common::str::Encoding DetectEncoding() const
    {
        return common::str::EncodingFromName(this->GetEncoding());
    }
};
SYSCOMMONAPI std::string GetEncoding(const common::span<const std::byte> data);

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


template<typename T>
inline common::str::Encoding DetectEncoding(const T& cont)
{
    return common::str::EncodingFromName(GetEncoding(detail::ToByteSpan(cont)));
}


}

