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
	String^ stacktrace;
	static Exception^ formInnerException(const BaseException& be)
	{
		if (const auto& inner = be.nestedException())
		{
			if (const auto *fex = dynamic_cast<FileException*>(inner.get()))
			{
				auto msg = ToStr(fex->message);
				auto fpath = ToStr(fex->filepath.u16string());
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
	CPPException(const BaseException& be) : Exception(ToStr(be.message), formInnerException(be))
	{
		const auto& stack = be.stacktrace();
		stacktrace = String::Format("at [{0}] : line {1} ({2})", ToStr(stack.func), stack.line, ToStr(stack.file));
	}
};

}