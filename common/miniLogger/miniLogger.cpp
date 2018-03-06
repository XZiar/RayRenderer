#include "MiniLoggerRely.h"
#include "miniLogger.h"

namespace common::mlog
{


namespace detail
{

MLoggerCallback MiniLoggerBase::OnLog = nullptr;
void MiniLoggerBase::SetGlobalCallback(const MLoggerCallback& cb)
{
    OnLog = cb;
}

}


}
