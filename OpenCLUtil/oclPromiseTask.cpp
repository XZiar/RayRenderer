#include "oclRely.h"
#include "oclPromiseTask.h"


namespace oclu
{


oclPromise::oclPromise(oclPromise &&other)
{
    eventPoint = other.eventPoint;
    other.eventPoint = nullptr;
}

oclPromise::~oclPromise()
{
    clReleaseEvent(eventPoint);
}

void oclPromise::wait()
{
    clWaitForEvents(1, &eventPoint);
}

uint64_t oclPromise::ElapseNs()
{
    uint64_t from = 0, to = 0;
    clGetEventProfilingInfo(eventPoint, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
    clGetEventProfilingInfo(eventPoint, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &to, nullptr);
    return to - from;
}


}