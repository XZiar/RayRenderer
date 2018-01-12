#pragma once

#ifndef _M_CEE
#error("CLIAsync should only be used with /clr")
#endif

#include "Exceptions.hpp"

#include <msclr\marshal_cppstd.h>
using namespace System;
using namespace msclr::interop;

namespace common
{

public ref class CPPException : public Exception
{
private:
	String^ stacktrace;
	static Exception^ formInnerException(const BaseException& be)
	{
		if (const auto& inner = be.nestedException())
		{
			if (const auto *fex = dynamic_cast<FileException*>(inner.get()))
			{
				auto msg = marshal_as<String^>(fex->message);
				auto fpath = marshal_as<String^>(fex->filepath.wstring());
				auto innerEx = formInnerException(*fex);
				if (fex->reason == FileException::Reason::NotExist)
					return gcnew IO::FileNotFoundException(msg, fpath, innerEx);
				else
					return gcnew IO::FileLoadException(msg, fpath, innerEx);
			}
			return gcnew CPPException(*inner);
		}
		else
			return nullptr;
	}
public:
	property String^ StackTrace
	{
		String^ get() override { return stacktrace; }
	}
	CPPException(const BaseException& be) : Exception(marshal_as<String^>(be.message), formInnerException(be))
	{
		const auto& stack = be.stacktrace();
		stacktrace = String::Format("at [{0}] : line {1} ({2})", marshal_as<String^>(stack.func), stack.line, marshal_as<String^>(stack.file));
	}
};

}