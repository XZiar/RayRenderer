#pragma once

#ifndef _M_CEE
#error("CLIException should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "Exceptions.hpp"
#include "FileExceptions.hpp"

using namespace System;

namespace Common
{

public ref class EndOfStreamExceptionEx : public IO::EndOfStreamException
{
private:
    initonly String^ fileName;
public:
    EndOfStreamExceptionEx(String^ msg, String^ fpath, Exception^ innEx) :
        EndOfStreamException(msg, innEx), fileName(fpath) { }
    property String^ FileName
    {
        String^ get() { return fileName; }
    }
};

public ref class FileException : public IO::IOException
{
public:
    enum class FileErrReasons : std::underlying_type_t<common::file::FileErrReason>
    {
        Open  = common::enum_cast(common::file::FileErrReason::OpenFail),  Read  = common::enum_cast(common::file::FileErrReason::ReadFail),
        Write = common::enum_cast(common::file::FileErrReason::WriteFail), Close = common::enum_cast(common::file::FileErrReason::CloseFail),
        UnknowErr = common::enum_cast(common::file::FileErrReason::UnknowErr),
        WrongParam, OpMismatch, IsDir, NotExist, AlreadyExist, PermissionDeny, WrongFormat, EndOfFile,
    };
private:
    initonly String^ fileName;
    initonly FileErrReasons reason;
public:

    FileException(String^ msg, String^ fpath, const common::file::FileErrReason errReason, Exception^ innEx) :
        IOException(msg, innEx), fileName(fpath), reason(static_cast<FileErrReasons>(errReason))
    { }
    property String^ FileName
    {
        String^ get() { return fileName; }
    }
    property FileErrReasons Operation
    {
        FileErrReasons get() 
        { return static_cast<FileErrReasons>(static_cast<common::file::FileErrReason>(reason) & common::file::FileErrReason::MASK_OP); }
    }
    property FileErrReasons Reason
    {
        FileErrReasons get()
        { return static_cast<FileErrReasons>(static_cast<common::file::FileErrReason>(reason) & common::file::FileErrReason::MASK_REASON); }
    }
};

public ref class CPPException : public Exception
{
private:
    initonly String^ stacktrace;
    static Exception^ FormInnerException(const common::BaseException& be)
    {
        if (const auto& inner = be.NestedException())
        {
            if (const auto fex = std::dynamic_pointer_cast<common::file::FileException>(inner))
            {
                auto msg = ToStr((const std::u16string_view&)fex->message);
                auto fpath = ToStr(fex->filepath.u16string());
                auto innerEx = FormInnerException(*fex);
                switch (fex->reason & common::file::FileErrReason::MASK_REASON)
                {
                case common::file::FileErrReason::NotExist:
                    return gcnew IO::FileNotFoundException(msg, fpath, innerEx);
                case common::file::FileErrReason::EndOfFile:
                    return gcnew EndOfStreamExceptionEx(msg, fpath, innerEx);
                default:
                    return gcnew FileException(msg, fpath, fex->reason, innerEx);
                }
            }
            else if (const auto bex = std::dynamic_pointer_cast<common::BaseException>(inner))
                return gcnew CPPException(*bex);
            else if (const auto oex = std::dynamic_pointer_cast<common::detail::OtherException>(inner))
                return gcnew CPPException(*oex);
        }
        return nullptr;
    }
    CPPException(const common::detail::OtherException& oe) : Exception(ToStr(std::string_view(oe.What()))) {}
public:
    property String^ StackTrace
    {
        String^ get() override { return stacktrace; }
    }
    CPPException(const common::BaseException& be) : Exception(ToStr(be.message), FormInnerException(be))
    {
        using std::u16string_view;
        stacktrace = "";
        for(const auto& stack : be.Stack())
            stacktrace += String::Format("at [{0}] : line {1} ({2})", 
                ToStr((const u16string_view&)stack.Func), stack.Line, ToStr((const u16string_view&)stack.File));
    }
};

}