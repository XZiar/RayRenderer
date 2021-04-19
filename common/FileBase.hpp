#pragma once
#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "EnumEx.hpp"
#include <string_view>


/* filesystem compatible include */

#ifdef __has_include
#   if __has_include(<filesystem>)
#       define COMMON_FS 1
#   elif __has_include(<experimental/filesystem>)
#       define COMMON_FS 2
#   endif
#endif

#ifndef COMMON_FS
# if COMMON_COMPILER_GCC
#   if COMMON_GCC_VER >= 80000
#     define COMMON_FS 1
#   elif COMMON_GCC_VER >= 50300
#     define COMMON_FS 2
#   elif !defined(COMMON_FS_NO_GHC)
#     error GCC version too low to use this header, at least gcc 5.3.0 for filesystem support
#   endif
# elif COMMON_COMPILER_CLANG
#   if COMMON_CLANG_VER >= 70000
#     define COMMON_FS 1
#   elif COMMON_CLANG_VER >= 30901
#     define COMMON_FS 2
#   elif !defined(COMMON_FS_NO_GHC)
#     error clang version too low to use this header, at least clang 3.9.1 for filesystem support
#   endif
# elif COMMON_COMPILER_MSVC
#   if COMMON_MSVC_VER >= 191400
#     define COMMON_FS 1
#   elif COMMON_MSVC_VER >= 190000
#     define COMMON_FS 2
#   elif !defined(COMMON_FS_NO_GHC)
#     error clang version too low to use this header, at least gcc 3.9.1 for filesystem support
#   endif
# elif !defined(COMMON_FS_NO_GHC)
#   error Unknown compiler, not supported by this header
# endif
#endif

// use ghc::filesystem if needed, boost::fs does not support char16_t & char32_t
#if defined(COMMON_FS_USE_GHC) || (!defined(COMMON_FS) && !defined(COMMON_FS_NO_GHC))
#   undef COMMON_FS
#   define COMMON_FS 3
#endif

#if COMMON_FS == 1
#   include <filesystem>
namespace common
{
namespace fs = std::filesystem;
}
#elif COMMON_FS == 2
#   include <experimental/filesystem>
namespace common
{
namespace fs = std::experimental::filesystem;
}
#elif COMMON_FS == 3
#   include "3rdParty/ghc_filesystem/include/ghc/filesystem.hpp"
namespace common
{
namespace fs = ghc::filesystem;
}
#endif


namespace common::file
{

enum class OpenFlag : uint16_t
{
    MASK_BASIC = 0xff00, MASK_EXTEND = 0xff00,
    FLAG_READ = 0x1, FLAG_WRITE = 0x2, FLAG_READWRITE = FLAG_READ | FLAG_WRITE, FLAG_CREATE = 0x4, FLAG_TRUNC = 0x8,
    FLAG_APPEND = 0x10, FLAG_TEXT = 0x20,

    FLAG_SHARE_READ = 0x0100, FLAG_SHARE_WRITE = 0x0200, FLAG_ASYNC = 0x0400,
    FLAG_DontBuffer = 0x1000, FLAG_WriteThrough = 0x2000, FLAG_DeleteOnClose = 0x4000,

    CreateNewBinary = FLAG_CREATE | FLAG_TRUNC | FLAG_WRITE, 
    CreateNewText   = FLAG_CREATE | FLAG_TRUNC | FLAG_WRITE  | FLAG_TEXT,
    ReadBinary      =                            FLAG_READ               | FLAG_SHARE_READ, 
    ReadText        =                            FLAG_READ   | FLAG_TEXT | FLAG_SHARE_READ,
    Append          = FLAG_CREATE | FLAG_WRITE | FLAG_APPEND,
    AppendText      = FLAG_CREATE | FLAG_WRITE | FLAG_APPEND | FLAG_TEXT,
    Dummy_WX        = FLAG_CREATE              | FLAG_WRITE,
    Dummy_RWX       = FLAG_CREATE              | FLAG_READWRITE,
    Dummy_RW        = CreateNewBinary | FLAG_READ,
    Dummy_AR        = Append         | FLAG_READ,
};
MAKE_ENUM_BITFIELD(OpenFlag)

enum class SeekWhere : int { Beginning = 0, Current = 1, End = 2 };

enum class FileErrReason : uint8_t 
{ 
    EMPTY = 0,
    MASK_OP   = 0xf0, MASK_REASON  = 0x0f,
    OpenFail  = 0x10, ReadFail     = 0x20, WriteFail      = 0x40, CloseFail   = 0x80,
    UnknowErr = 0, 
    WrongParam, OpMismatch, IsDir, NotExist, AlreadyExist, PermissionDeny, WrongFormat, EndOfFile, 
    SharingViolate
};
MAKE_ENUM_BITFIELD(FileErrReason)

constexpr inline std::u16string_view GetReasonOp(const FileErrReason reason)
{
    switch (reason & FileErrReason::MASK_OP)
    {
    case FileErrReason::OpenFail:       return u"Open";
    case FileErrReason::ReadFail:       return u"Read";
    case FileErrReason::WriteFail:      return u"Write";
    case FileErrReason::CloseFail:      return u"Close";
    default:                            return u"Unknown";
    }
}


class FileException : public BaseException
{
    PREPARE_EXCEPTION(FileException, BaseException,
        fs::path Filepath;
        FileErrReason Reason;
        ExceptionInfo(const std::u16string_view msg, const fs::path& filepath, const FileErrReason reason)
            : TPInfo(TYPENAME, msg), Filepath(filepath), Reason(reason)
        { }
    );
    FileException(const FileErrReason why, const fs::path& file, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, file, why)
    { }
    FileErrReason Reason() const noexcept { return GetInfo().Reason; }
    const fs::path& FilePath() const noexcept { return GetInfo().Filepath; }
};


}
