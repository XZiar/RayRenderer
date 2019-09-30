#include "CommonRely.hpp"
#include "Exceptions.hpp"


namespace common::file
{


class FileException : public BaseException
{
public:
    enum class Reason { NotExist, WrongFormat, OpenFail, ReadFail, WriteFail, CloseFail };
public:
    fs::path filepath;
public:
    EXCEPTION_CLONE_EX(FileException);
    const Reason reason;
    FileException(const Reason why, const fs::path& file, const std::u16string_view& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_), filepath(file), reason(why)
    { }
    ~FileException() override {}
};


}
