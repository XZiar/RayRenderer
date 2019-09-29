#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef SYSCOMMON_EXPORT
#   define SYSCOMMONAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define SYSCOMMONAPI _declspec(dllimport)
# endif
#else
# ifdef SYSCOMMON_EXPORT
#   define SYSCOMMONAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define SYSCOMMONAPI
# endif
#endif


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/EnumEx.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <string_view>

