#pragma once
#include "miniLogger.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

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
		if (curlevel.exchange((uint8_t)lv) != (uint8_t)lv)
		{
			WORD attrb = FOREGROUND_INTENSITY;
			switch (lv)
			{
			case LogLevel::Error:
				attrb |= FOREGROUND_RED;//red
				break;
			case LogLevel::Sucess:
				attrb |= FOREGROUND_GREEN;//green
				break;
			case LogLevel::Info:
				attrb |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;//white
				break;
			case LogLevel::Verbose:
				attrb |= FOREGROUND_BLUE;//blue
				break;
			case LogLevel::Debug:
				attrb |= FOREGROUND_GREEN | FOREGROUND_BLUE;//cyan
				break;
			}
			SetConsoleTextAttribute(hConsole, attrb);
		}
	}
public:
	ConsoleLogger()
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		curlevel = (uint8_t)LogLevel::Info;
	}
	void print(const LogLevel lv, const std::wstring& content)
	{
		if (hConsole == nullptr)
			return;
		while (!flagWrite.test_and_set())
			;//spin lock
		changeState(lv);
		DWORD outlen;
		WriteConsole(hConsole, content.c_str(), content.length(), &outlen, NULL);
		flagWrite.clear();
	}
};


}