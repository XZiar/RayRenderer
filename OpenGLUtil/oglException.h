#pragma once
#include "oglRely.h"

namespace oglu
{


class OGLException : public BaseException
{
public:
	enum class GLComponent { Compiler, Driver, GPU };
	const GLComponent exceptionSource;
	OGLException(const GLComponent source, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(msg, data), exceptionSource(source)
	{ }
	virtual ~OGLException() {}
	virtual Wrapper<BaseException> clone() const override
	{
		return Wrapper<OGLException>(*this);
	}
};

//class GL


}