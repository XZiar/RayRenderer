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
    std::shared_ptr<oclPromiseCore> Prev;
    cl_event Event;
    std::exception_ptr Exception;
    const cl_command_queue Queue;
    oclPromiseCore(const cl_event e, const cl_command_queue que, const std::shared_ptr<oclPromiseCore>& prev = {})
        : Prev(prev), Event(e), Queue(que)
    {
        //clFlush(que);
    }
    ~oclPromiseCore()
    {
        if (Event)
            clReleaseEvent(Event);
    }
    void Flush()
    {
        clFlush(Queue);
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
    cl_event& GetEvent() { return Event; }
    void SetException(const std::exception_ptr ex) { Exception = ex; }
};

template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
    friend class _oclBuffer;
    friend class _oclImage;
private:
    virtual void PreparePms() override 
    { 
        Flush();
    }
    common::PromiseState virtual State() override
    {
        return oclPromiseCore::State();
    }
    T WaitPms() override
    {
        oclPromiseCore::Wait();
        return std::move(Result);
    }
public:
    T Result;
    oclPromise(const cl_event e, const cl_command_queue que) : oclPromiseCore(e, que) { }
    template<typename U>
    oclPromise(const cl_event e, const cl_command_queue que, U&& data) : oclPromiseCore(e, que), Result(std::forward<U>(data)) { }
    ~oclPromise() override { }
    uint64_t ElapseNs() override 
    { 
        return oclPromiseCore::ElapseNs();
    }
};

class oclPromiseVoid : public ::common::detail::PromiseResult_<void>, public oclPromiseCore
{
    friend class _oclBuffer;
    friend class _oclImage;
    friend class _oclKernel;
private:
    virtual void PreparePms() override
    {
        Flush();
    }
    common::PromiseState virtual State() override
    {
        return oclPromiseCore::State();
    }
    void WaitPms() override
    {
        oclPromiseCore::Wait();
    }
public:
    oclPromiseVoid(const cl_event e, const cl_command_queue que, const std::shared_ptr<oclPromiseVoid>& pms = {})
        : oclPromiseCore(e, que, pms) { }
    ~oclPromiseVoid() override { }
    uint64_t ElapseNs() override 
    { 
        return oclPromiseCore::ElapseNs();
    }
};

}

}
