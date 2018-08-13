#include "oclRely.h"
#include "oclPromiseTask.h"
#include "oclUtil.h"


namespace oclu
{

namespace detail
{

oclPromise_::oclPromise_(const cl_event e, const cl_command_queue que) : eventPoint(e)
{
    clFlush(que);
}

oclPromise_::oclPromise_(oclPromise_ &&other)
{
    eventPoint = other.eventPoint;
    other.eventPoint = nullptr;
}

oclPromise_::~oclPromise_()
{
    if (eventPoint)
        clReleaseEvent(eventPoint);
}

common::PromiseState oclPromise_::state()
{
    using common::PromiseState;
    cl_int status;
    const auto ret = clGetEventInfo(eventPoint, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
    if (ret != CL_SUCCESS)
    {
        oclLog().warning(u"Error in reading cl_event's status: {}\n", oclUtil::getErrorString(ret));
        return PromiseState::Invalid;
    }
    if (status < 0)
    {
        oclLog().warning(u"cl_event's status shows an error: {}\n", oclUtil::getErrorString(status));
        return PromiseState::Error;
    }
    switch (status)
    {
    case CL_QUEUED: return PromiseState::Unissued;
    case CL_SUBMITTED: return PromiseState::Issued;
    case CL_RUNNING: return PromiseState::Executing;
    case CL_COMPLETE: return PromiseState::Success;
    default: return PromiseState::Invalid;
    }
}

void oclPromise_::wait()
{
    clWaitForEvents(1, &eventPoint);
}

uint64_t oclPromise_::ElapseNs()
{
    uint64_t from = 0, to = 0;
    clGetEventProfilingInfo(eventPoint, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
    clGetEventProfilingInfo(eventPoint, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &to, nullptr);
    return to - from;
}
}


}