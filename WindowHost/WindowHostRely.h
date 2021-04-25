#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef WDHOST_EXPORT
#   define WDHOSTAPI _declspec(dllexport)
# else
#   define WDHOSTAPI _declspec(dllimport)
# endif
#else
# define WDHOSTAPI [[gnu::visibility("default")]]
#endif

#include "common/CommonRely.hpp"
#include "common/Delegate.hpp"
#include "common/EnumEx.hpp"
#include <string>
#include <functional>
#include <memory>
#include <atomic>

