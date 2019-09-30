#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "EnumEx.hpp"


namespace common::file
{


enum class FileErrReason : uint8_t 
{ 
    EMPTY = 0,
    MASK_OP   = 0xf0, MASK_REASON  = 0x0f,
    OpenFail  = 0x10, ReadFail     = 0x20, WriteFail      = 0x40, CloseFail   = 0x80,
    UnknowErr = 0, 
    WrongParam, OpMismatch, IsDir, NotExist, AlreadyExist, PermissionDeny, WrongFormat, EndOfFile,
};
MAKE_ENUM_BITFIELD(FileErrReason)

class FileException : public BaseException
{
public:
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
