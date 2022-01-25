#pragma once
#include "oglRely.h"

namespace oglu
{


class OGLException : public common::BaseException
{
public:
    enum class GLComponent { Compiler, Driver, GPU, OGLU, Loader, Tex };
private:
    PREPARE_EXCEPTION(OGLException, BaseException,
        GLComponent Component;
        ExceptionInfo(const std::u16string_view msg, const GLComponent source)
            : ExceptionInfo(TYPENAME, msg, source)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u16string_view msg, const GLComponent source)
            : TPInfo(type, msg), Component(source)
        { }
    );
    OGLException(const GLComponent source, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, source)
    { }
};


}