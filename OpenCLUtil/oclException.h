#pragma once
#include "oclRely.h"
#include "oclUtil.h"

namespace oclu
{


class OCLException : public common::BaseException
{
public:
    EXCEPTION_CLONE_EX(OCLException);
    enum class CLComponent { Compiler, Driver, Accellarator, OCLU };
    const CLComponent exceptionSource;
protected:
    OCLException(const char* const type, const CLComponent source, const std::u16string_view& msg)
        : BaseException(type, msg), exceptionSource(source) { }
public:
    OCLException(const CLComponent source, const std::u16string_view& msg)
        : OCLException(TYPENAME, source, msg)
    { }
    OCLException(const CLComponent source, cl_int errcode, std::u16string msg)
        : OCLException(TYPENAME, source, msg.append(u" --ERROR: ").append(oclUtil::GetErrorString(errcode)))
    { }
    virtual ~OCLException() {}
};




}