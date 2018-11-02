#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef STRCHSET_EXPORT
#   define STRCHSETAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define STRCHSETAPI _declspec(dllimport)
# endif
#else
# ifdef STRCHSET_EXPORT
#   define STRCHSETAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define STRCHSETAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/AlignedContainer.hpp"
#include "common/StrBase.hpp"
#include "common/Wrapper.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <string_view>
