#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef STKTRACE_EXPORT
#   define STKTRACEAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define STKTRACEAPI _declspec(dllimport)
# endif
#else
# ifdef STKTRACE_EXPORT
#   define STKTRACEAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define STKTRACEAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include <vector>


namespace stacktrace
{

std::vector<common::StackTraceItem> STKTRACEAPI GetStack();
#define CREATE_EXCEPTIONEX(ex, ...) ::common::BaseException::CreateWithStacks<ex>(::stacktrace::GetStack(), __VA_ARGS__)
#define COMMON_THROWEX(ex, ...) throw CREATE_EXCEPTIONEX(ex, __VA_ARGS__)


}