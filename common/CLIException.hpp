#pragma once

#ifndef _M_CEE
#error("CLIException should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "Exceptions.hpp"

using namespace System;

namespace Common
{

public ref class CPPException : public Exception
{
private:
    initonly String^ stacktrace;
    static Exception^ formInnerException(const common::BaseException& be)
    {
        if (const auto& inner = be.NestedException())
        {
            if (const auto fex = std::dynamic_pointer_cast<common::FileException>(inner))
            {
                auto msg = ToStr((const std::u16string_view&)fex->message);
                auto fpath = ToStr(fex->filepath.u16string());
                auto innerEx = formInnerException(*fex);
                if (fex->reason == common::FileException::Reason::NotExist)
                    return gcnew IO::FileNotFoundException(msg, fpath, innerEx);
                else
                    return gcnew IO::FileLoadException(msg, fpath, innerEx);
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
    CPPException(const common::BaseException& be) : Exception(ToStr(be.message), formInnerException(be))
    {
        using std::u16string_view;
        stacktrace = "";
        for(const auto& stack : be.Stack())
            stacktrace += String::Format("at [{0}] : line {1} ({2})", 
                ToStr((const u16string_view&)stack.Func), stack.Line, ToStr((const u16string_view&)stack.File));
    }
};

}