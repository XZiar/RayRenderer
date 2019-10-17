#pragma once
#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "EnumEx.hpp"
#include <string_view>

namespace common::file
{

enum class OpenFlag : uint16_t
{
    MASK_BASIC = 0xff00, MASK_EXTEND = 0xff00,
    FLAG_READ = 0x1, FLAG_WRITE = 0x2, FLAG_READWRITE = FLAG_READ | FLAG_WRITE, FLAG_CREATE = 0x4, FLAG_TRUNC = 0x8,
    FLAG_APPEND = 0x10, FLAG_TEXT = 0x20,

    CreatNewBinary = FLAG_CREATE | FLAG_TRUNC | FLAG_WRITE, 
    CreatNewText   = FLAG_CREATE | FLAG_TRUNC | FLAG_WRITE  | FLAG_TEXT,
    ReadBinary     =                            FLAG_READ, 
    ReadText       =                            FLAG_READ   | FLAG_TEXT,
    Append         = FLAG_CREATE | FLAG_WRITE | FLAG_APPEND,
    AppendText     = FLAG_CREATE | FLAG_WRITE | FLAG_APPEND | FLAG_TEXT,
    Dummy_WX       = FLAG_CREATE              | FLAG_WRITE,
    Dummy_RWX      = FLAG_CREATE              | FLAG_READWRITE,
    Dummy_RW       = CreatNewBinary | FLAG_READ,
    Dummy_AR       = Append         | FLAG_READ,

    FLAG_SHARE_READ = 0x0100, FLAG_SHARE_WRITE  = 0x0200, FLAG_ASYNC = 0x0400,
    FLAG_DontBuffer = 0x1000, FLAG_WriteThrough = 0x2000, FLAG_DeleteOnClose = 0x4000,
};
MAKE_ENUM_BITFIELD(OpenFlag)


enum class FileErrReason : uint8_t 
{ 
    EMPTY = 0,
    MASK_OP   = 0xf0, MASK_REASON  = 0x0f,
    OpenFail  = 0x10, ReadFail     = 0x20, WriteFail      = 0x40, CloseFail   = 0x80,
    UnknowErr = 0, 
    WrongParam, OpMismatch, IsDir, NotExist, AlreadyExist, PermissionDeny, WrongFormat, EndOfFile,
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
public:
    fs::path filepath;
public:
    EXCEPTION_CLONE_EX(FileException);
    const FileErrReason reason;
    FileException(const FileErrReason why, const fs::path& file, const std::u16string_view& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_), filepath(file), reason(why)
    { }
    ~FileException() override {}
};


}
