#pragma once

#ifndef _M_CEE
#error("CLIException should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "Exceptions.hpp"

using namespace System;

namespace common
{

public ref class CPPException : public Exception
{
private:
    initonly String^ stacktrace;
    static Exception^ formInnerException(const BaseException& be)
    {
        if (const auto& inner = be.NestedException())
        {
            if (const auto fex = inner.cast_dynamic<FileException>())
            {
                auto msg = ToStr(fex->message);
                auto fpath = ToStr(fex->filepath.u16string());
                auto innerEx = formInnerException(*fex);
                if (fex->reason == FileException::Reason::NotExist)
                    return gcnew IO::FileNotFoundException(msg, fpath, innerEx);
                else
                    return gcnew IO::FileLoadException(msg, fpath, innerEx);
            }
            else if (const auto bex = inner.cast_dynamic<BaseException>())
                return gcnew CPPException(*bex);
            else if (const auto oex = inner.cast_dynamic<detail::OtherException>())
                return gcnew CPPException(*oex);
        }
        return nullptr;
    }
    CPPException(const detail::OtherException& oe) : Exception(ToStr(std::string_view(oe.What()))) {}
public:
    property String^ StackTrace
    {
        String^ get() override { return stacktrace; }
    }
    CPPException(const BaseException& be) : Exception(ToStr(be.message), formInnerException(be))
    {
        const auto& stack = be.Stacktrace();
        stacktrace = String::Format("at [{0}] : line {1} ({2})", ToStr(stack.Func), stack.Line, ToStr(stack.File));
    }
};

}