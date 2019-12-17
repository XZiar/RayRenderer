#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef WDHOST_EXPORT
#   define WDHOSTAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define WDHOSTAPI _declspec(dllimport)
# endif
#else
# ifdef WDHOST_EXPORT
#   define WDHOSTAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define WDHOSTAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/Delegate.hpp"
#include <string>
#include <functional>
#include <memory>
#include <atomic>

