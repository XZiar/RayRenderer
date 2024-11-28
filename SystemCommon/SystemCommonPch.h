#pragma once
#include "SystemCommonRely.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/Exceptions.h"
#include "common/FileBase.hpp"
#include "common/MemoryStream.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"
#if !COMMON_COMPILER_MSVC || (defined(_MSVC_TRADITIONAL) && !_MSVC_TRADITIONAL)
#   define BOOST_PP_LIMIT_TUPLE 128
#endif
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/container/small_vector.hpp>

#include <cassert>
#include <thread>
#include <variant>
#include <tuple>
#include <optional>
#include <algorithm>
#include <stdexcept>

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <conio.h>
#   include <Windows.h>
#else
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#   endif
#   if COMMON_OS_ANDROID
#       define __USE_GNU 1
#   endif
#   include <cerrno>
#   include <errno.h>
#   include <dlfcn.h>
#   include <fcntl.h>
#   include <termios.h>
#   include <unistd.h>
#   include <sys/ioctl.h>
#   include <sys/mman.h>
#   include <sys/resource.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   if COMMON_OS_DARWIN
#       include <sys/sysctl.h>
#       include <mach/mach_types.h>
#       include <mach/thread_act.h>
#       if __DARWIN_64_BIT_INO_T
            namespace common::file
            {
            struct stat64 : public stat {};
            inline int fstat64(int fd, struct stat64* buf) { return fstat(fd, buf); }
            }
            using off64_t = off_t;
            inline off64_t lseek64(int fd, off64_t offset, int whence) { return lseek(fd, offset, whence); }
            inline int fseeko64(FILE* stream, off64_t offset, int whence) { return fseeko(stream, offset, whence); }
            inline off64_t ftello64(FILE* stream) { return ftello(stream); }
#       endif
#   else
#       include <sys/syscall.h>
#   endif
#endif


namespace common
{

namespace detail
{

inline constexpr CommonColor GetLogColor(const mlog::LogLevel lv) noexcept
{
    switch (lv)
    {
    case mlog::LogLevel::Error:     return CommonColor::BrightRed;
    case mlog::LogLevel::Warning:   return CommonColor::BrightYellow;
    case mlog::LogLevel::Success:   return CommonColor::BrightGreen;
    case mlog::LogLevel::Info:      return CommonColor::BrightWhite;
    case mlog::LogLevel::Verbose:   return CommonColor::BrightMagenta;
    case mlog::LogLevel::Debug:     return CommonColor::BrightCyan;
    default:                        return CommonColor::White;
    }
}

struct InitMessage
{
    struct Handler
    {
        virtual void Handle(mlog::LogLevel level, std::string_view host, std::string_view msg) noexcept = 0;
        virtual ~Handler();
    };

    std::string Host;
    std::string Message;
    mlog::LogLevel Level = mlog::LogLevel::None;
    static void Enqueue(mlog::LogLevel level, std::string_view host, std::string_view msg) noexcept;
    static void Consume(std::unique_ptr<Handler> handler) noexcept;
};

void DebugErrorOutput(std::string_view str) noexcept;
}

}