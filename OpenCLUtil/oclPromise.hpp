#pragma once

#include "oclRely.h"

namespace oclu
{
namespace detail
{

class oclPromiseCore
{
    friend class _oclBuffer;
    friend class _oclImage;
    friend class _oclKernel;
protected:
    const cl_event Event;
    oclPromiseCore(const cl_event e, const cl_command_queue que) : Event(e) 
    { 
        clFlush(que);
    }
    ~oclPromiseCore()
    {
        if (Event)
            clReleaseEvent(Event);
    }
    common::PromiseState State()
    {
        using common::PromiseState;
        cl_int status;
        const auto ret = clGetEventInfo(Event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
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
    void Wait()
    {
        clWaitForEvents(1, &Event);
    }
    uint64_t ElapseNs()
    {
        uint64_t from = 0, to = 0;
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &to, nullptr);
        return to - from;
    }
};

template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
    friend class _oclBuffer;
    friend class _oclImage;
public:
    T Result;
    oclPromise(const cl_event e, const cl_command_queue que) : oclPromiseCore(e, que) { }
    template<typename U>
    oclPromise(const cl_event e, const cl_command_queue que, U&& data) : oclPromiseCore(e, que), Result(std::forward<U>(data)) { }
    ~oclPromise() override { }
    common::PromiseState virtual state() override { return State(); }
    T wait() override
    {
        Wait();
        return std::move(Result);
    }
    uint64_t ElapseNs() override { return oclPromiseCore::ElapseNs(); }
};

class oclPromiseVoid : public ::common::detail::PromiseResult_<void>, public oclPromiseCore
{
    friend class _oclBuffer;
    friend class _oclImage;
    friend class _oclKernel;
public:
    oclPromiseVoid(const cl_event e, const cl_command_queue que) : oclPromiseCore(e, que) { }
    ~oclPromiseVoid() override { }
    common::PromiseState virtual state() override { return oclPromiseCore::State(); }
    void wait() override { Wait(); }
    uint64_t ElapseNs() override { return oclPromiseCore::ElapseNs(); }
};

}

}
