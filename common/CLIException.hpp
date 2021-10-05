#pragma once

#ifndef _M_CEE
#error("CLIException should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "FileBase.hpp"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/FileEx.h"

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
    CPPException(const common::ExceptionBasicInfo& info, String^ msg, Exception^ innerEx) : Exception(msg, innerEx)
    {
        using std::u16string_view;
        stacktrace = "";
        for (const auto& stack : info.StackTrace)
            stacktrace += String::Format("at [{0}] : line {1} ({2})\r\n",
                ToStr((const u16string_view&)stack.Func), stack.Line, ToStr((const u16string_view&)stack.File));
    }
    CPPException(String^ msg, Exception^ innerEx) : Exception(msg, innerEx) 
    { 
        stacktrace = ""; 
    }
    static Exception^ FromException(common::ExceptionBasicInfo& info)
    {
        Exception^ innerEx = info.InnerException ? FromException(*info.InnerException) : nullptr;
        if (const auto fex = info.Cast<common::file::FileException>(); fex)
        {
            auto msg = ToStr(fex->Message);
            auto fpath = ToStr(fex->Filepath.u16string());
            switch (fex->Reason & common::file::FileErrReason::MASK_REASON)
            {
            case common::file::FileErrReason::NotExist:
                return gcnew IO::FileNotFoundException(msg, fpath, innerEx);
            case common::file::FileErrReason::EndOfFile:
                return gcnew EndOfStreamExceptionEx(msg, fpath, innerEx);
            default:
                return gcnew FileException(msg, fpath, fex->Reason, innerEx);
            }
        }
        else if (const auto oex = info.Cast<common::OtherException>(); oex)
            return gcnew CPPException(info, ToStr(std::string_view(oex->GetInnerMessage())), innerEx);
        else
            return gcnew CPPException(info, ToStr(info.Message), innerEx);
    }
public:
    property String^ StackTrace
    {
        String^ get() override { return stacktrace; }
    }
    static Exception^ FromException(const common::BaseException& be)
    {
        return FromException(*be.InnerInfo());
    }
    static Exception^ FromException(std::exception_ptr e)
    {
        String^ msg;
        try
        {
            std::rethrow_exception(e);
        }
        catch (const common::BaseException& be)
        {
            return FromException(be);
        }
        catch (const std::runtime_error& re)
        {
            msg = ToStr(std::string_view(re.what()));
        }
        catch (...)
        {
            msg = "UNKNWON";
        }
        return gcnew CPPException(msg, nullptr);
    }
};

}