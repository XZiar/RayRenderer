#include "FontRely.h"

namespace oglu
{

using namespace common::mlog;
logger& fntLog()
{
	static logger fntlog(L"FontHelper", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return fntlog;
}



}