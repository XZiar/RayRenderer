#pragma once

#include "oclPch.h"

namespace oclu
{
class oclBuffer_;
class oclImage_;
class oclKernel_;


class oclPromiseCore
{
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclKernel_;
protected:
    const std::shared_ptr<oclPromiseCore> Prev;
    const cl_event Event;
    const std::exception_ptr Exception;
    const oclCmdQue Queue;
    oclPromiseCore(const cl_event e, oclCmdQue que, std::shared_ptr<oclPromiseCore> prev)
        : Prev(std::move(prev)), Event(e), Queue(std::move(que)) { }
    oclPromiseCore(const std::exception_ptr ex, const cl_event e, oclCmdQue que, std::shared_ptr<oclPromiseCore> prev)
        : Prev(std::move(prev)), Event(e), Exception(ex), Queue(std::move(que)) { }
    ~oclPromiseCore()
    {
        if (Event)
            clReleaseEvent(Event);
    }
    void Flush()
    {
        if (Exception)
            std::rethrow_exception(Exception);
        clFlush(Queue->CmdQue);
    }
    common::PromiseState State()
    {
        using common::PromiseState;
        if (Exception)
            return PromiseState::Error;
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
        case CL_QUEUED: return PromiseState::Unissued;
        case CL_SUBMITTED: return PromiseState::Issued;
        case CL_RUNNING: return PromiseState::Executing;
        case CL_COMPLETE: return PromiseState::Success;
        default: return PromiseState::Invalid;
        }
    }
    void Wait()
    {
        if (Exception)
            std::rethrow_exception(Exception);
        clWaitForEvents(1, &Event);
    }
    uint64_t ElapseNs()
    {
        uint64_t from = 0, to = 0;
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
        clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &to, nullptr);
        return to - from;
    }
    uint64_t ChainedElapseNs()
    {
        auto time = ElapseNs();
        for (std::shared_ptr<oclPromiseCore> prev = Prev; prev; prev = prev->Prev)
            time += prev->ElapseNs();
        return time;
    }
    const cl_event& GetEvent() { return Event; }
};

template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
    friend class oclBuffer_;
    friend class oclImage_;
private:
    std::conditional_t<std::is_same_v<T, void>, uint32_t, T> Result;
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
        if constexpr(!std::is_same_v<T, void>)
            return std::move(Result);
    }
public:
    template<typename U>
    oclPromise(const cl_event e, oclCmdQue que, U&& data, std::shared_ptr<oclPromiseCore> prev = {})
        : oclPromiseCore(e, std::move(que), std::move(prev)), Result(std::forward<U>(data)) { }
    ~oclPromise() override { }
    uint64_t ElapseNs() override 
    { 
        return oclPromiseCore::ElapseNs();
    }
    uint64_t ChainedElapseNs() override 
    { 
        return oclPromiseCore::ChainedElapseNs();
    };

};


}
