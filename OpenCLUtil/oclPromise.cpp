#include "oclPch.h"
#include "oclPromise.h"
#include "oclUtil.h"
#include "common/Linq2.hpp"

namespace oclu
{

oclPromiseCore::oclPromiseCore(PrevType&& prev, const cl_event e, oclCmdQue que)
    : Event(e), Queue(std::move(que)), Prev(std::move(prev)) { }
oclPromiseCore::~oclPromiseCore()
{
    if (Event)
        clReleaseEvent(Event);
}
void oclPromiseCore::Flush()
{
    if (Queue)
        clFlush(Queue->CmdQue);
}
[[nodiscard]] common::PromiseState oclPromiseCore::State()
{
    using common::PromiseState;
    cl_int status;
    const auto ret = clGetEventInfo(Event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
    if (ret != CL_SUCCESS)
    {
        oclLog().warning(u"Error in reading cl_event's status: {}\n", oclUtil::GetErrorString(ret));
        return PromiseState::Invalid;
    }
    if (status < 0)
    {
        oclLog().warning(u"cl_event's status shows an error: {}\n", oclUtil::GetErrorString(status));
        return PromiseState::Error;
    }
    switch (status)
    {
    case CL_QUEUED:     return PromiseState::Unissued;
    case CL_SUBMITTED:  return PromiseState::Issued;
    case CL_RUNNING:    return PromiseState::Executing;
    case CL_COMPLETE:   return PromiseState::Success;
    default:            return PromiseState::Invalid;
    }
}
void oclPromiseCore::Wait()
{
    if (Event)
        clWaitForEvents(1, &Event);
}
[[nodiscard]] uint64_t oclPromiseCore::ElapseNs()
{
    if (Event)
    {
        uint64_t from = 0, to = 0;
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &to, nullptr);
        return to - from;
    }
    return 0;
}
[[nodiscard]] uint64_t oclPromiseCore::ChainedElapseNs()
{
    auto time = ElapseNs();
    switch (Prev.index())
    {
    case 2: return time + std::get<2>(Prev)->ChainedElapseNs();
    case 1:
        for (const auto& prev : std::get<1>(Prev))
        {
            time += prev->ChainedElapseNs();
        }
        return time;
    default:
    case 0: return time;
    }
}
[[nodiscard]] std::pair<std::vector<std::shared_ptr<oclPromiseCore>>, oclPromiseCore::oclEvents>
oclPromiseCore::ParsePms(const common::PromiseStub& pmss) noexcept
{
    auto clpmss = pmss.FilterOut<oclPromiseCore>();
    auto evts = common::linq::FromIterable(clpmss)
        .Select([](const auto& clpms) { return clpms->GetEvent(); })
        .Where([](const auto& evt) { return evt != nullptr; })
        .ToVector();
    if (evts.size() > 0)
        return { std::move(clpmss), std::move(evts) };
    else
        return { std::move(clpmss), {} };
}

uint64_t oclPromiseCore::QueryTime(oclPromiseCore::TimeType type) const noexcept
{
    if (Event)
    {
        cl_profiling_info info = UINT32_MAX;
        switch (type)
        {
        case TimeType::Queued: info = CL_PROFILING_COMMAND_QUEUED; break;
        case TimeType::Submit: info = CL_PROFILING_COMMAND_SUBMIT; break;
        case TimeType::Start:  info = CL_PROFILING_COMMAND_START;  break;
        case TimeType::End:    info = CL_PROFILING_COMMAND_END;    break;
        default: break;
        }
        if (info != UINT32_MAX)
        {
            uint64_t time;
            clGetEventProfilingInfo(Event, info, sizeof(cl_ulong), &time, nullptr);
            return time;
        }
    }
    return 0;
}


}
