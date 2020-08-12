#pragma once
#include "oclRely.h"
#include "oclUtil.h"

namespace oclu
{



class OCLException : public common::BaseException
{
public:
    enum class CLComponent { Compiler, Driver, Accellarator, OCLU };
private:
    PREPARE_EXCEPTION(OCLException, BaseException,
        const CLComponent Component;
        template<typename T>
        ExceptionInfo(T&& msg, const CLComponent source)
            : ExceptionInfo(TYPENAME, std::forward<T>(msg), source)
        { }
    protected:
        template<typename T>
        ExceptionInfo(const char* type, T&& msg, const CLComponent source)
            : TPInfo(type, std::forward<T>(msg)), Component(source)
        { }
    );
    OCLException(const CLComponent source, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, source)
    { }
    OCLException(const CLComponent source, cl_int errcode, std::u16string msg)
        : BaseException(T_<ExceptionInfo>{}, msg.append(u" --ERROR: ").append(oclUtil::GetErrorString(errcode)), source)
    { }
};



}