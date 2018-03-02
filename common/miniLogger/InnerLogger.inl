#pragma once
#include "miniLogger.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "common/SpinLock.hpp"

namespace common::mlog
{


class ConsoleLogger
{
private:
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO csbInfo;
	std::atomic_flag flagWrite = ATOMIC_FLAG_INIT;
	std::atomic_uint_least8_t curlevel;
	void changeState(const LogLevel lv)
	{
		constexpr WORD BLACK = 0;
		constexpr WORD BLUE = 1;
		constexpr WORD GREEN = 2;
		constexpr WORD CYAN = 3;
		constexpr WORD RED = 4;
		constexpr WORD MAGENTA = 5;
		constexpr WORD BROWN = 6;
		constexpr WORD LIGHTGRAY = 7;
		constexpr WORD DARKGRAY = 8;
		constexpr WORD LIGHTBLUE = 9;
		constexpr WORD LIGHTGREEN = 10;
		constexpr WORD LIGHTCYAN = 11;
		constexpr WORD LIGHTRED = 12;
		constexpr WORD LIGHTMAGENTA = 13;
		constexpr WORD YELLOW = 14;
		constexpr WORD WHITE = 15;
		if (curlevel.exchange((uint8_t)lv) != (uint8_t)lv)
		{
			WORD attrb = BLACK;
			switch (lv)
			{
			case LogLevel::Error:
				attrb = LIGHTRED; break;
			case LogLevel::Warning:
				attrb = YELLOW; break;
			case LogLevel::Sucess:
				attrb = LIGHTGREEN; break;
			case LogLevel::Info:
				attrb = WHITE; break;
			case LogLevel::Verbose:
				attrb = LIGHTMAGENTA; break;
			case LogLevel::Debug:
				attrb = LIGHTCYAN; break;
			}
			SetConsoleTextAttribute(hConsole, attrb);
		}
	}
public:
	ConsoleLogger()
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		curlevel = (uint8_t)LogLevel::None;
	}
	void print(const LogLevel lv, const std::wstring& content)
	{
		if (hConsole == nullptr)
			return;
        common::SpinLocker locker(flagWrite);
		changeState(lv);
		DWORD outlen;
		WriteConsole(hConsole, content.c_str(), (DWORD)content.length(), &outlen, NULL);
	}
};


class DebuggerLogger
{
    std::atomic_flag flagWrite = ATOMIC_FLAG_INIT;
public:
    void print(const LogLevel lv, const std::wstring& content)
    {
        common::SpinLocker locker(flagWrite);
        OutputDebugString(content.c_str());
    }
};


}