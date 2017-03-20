#include "RenderCoreInternal.h"

namespace rayr
{

using namespace common::mlog;
logger& basLog()
{
	static logger baslog(L"BasicTest", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return baslog;
}


}