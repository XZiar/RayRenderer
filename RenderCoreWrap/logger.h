#pragma once

#include "RenderCoreRely.h"

using namespace System;

#pragma unmanaged
void __cdecl onLog(common::mlog::LogLevel lv, const std::wstring& from, const std::wstring& content);
void setLogger();
#pragma managed

public ref class ManagedLogger
{
public:
	void log(common::mlog::LogLevel lv, String^ from, String^ content);
};

