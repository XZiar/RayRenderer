#pragma once
#include "oglRely.h"

namespace oglu
{


class OGLException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(OGLException);
	enum class GLComponent { Compiler, Driver, GPU, OGLU, Tex };
	const GLComponent exceptionSource;
protected:
	OGLException(const char* const type, const GLComponent source, const std::u16string_view msg)
		: common::BaseException(type, msg), exceptionSource(source) { }
public:
	OGLException(const GLComponent source, const std::u16string_view msg)
		: OGLException(TYPENAME, source, msg)
	{ }
	virtual ~OGLException() {}
};

//class GL


}