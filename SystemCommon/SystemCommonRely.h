#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef SYSCOMMON_EXPORT
#   define SYSCOMMONAPI _declspec(dllexport)
# else
#   define SYSCOMMONAPI _declspec(dllimport)
# endif
#else
# define SYSCOMMONAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <string_view>


namespace common
{
[[nodiscard]] SYSCOMMONAPI bool CheckCPUFeature(str::HashedStrView<char> feature) noexcept;
[[nodiscard]] SYSCOMMONAPI span<const std::string_view> GetCPUFeatures() noexcept;
}