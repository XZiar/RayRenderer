#include "ImageUtilRely.h"

namespace xziar::img
{
using namespace common::mlog;
logger& ImgLog()
{
	static logger imglog(L"ImageUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return imglog;
}
}