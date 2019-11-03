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
    const cl_event Event;
    const oclCmdQue Queue;
    std::variant<std::monostate, std::vector<std::shared_ptr<oclPromiseCore>>, std::shared_ptr<oclPromiseCore>> Prev;
    template<typename T>
    oclPromiseCore(T&& prev, const cl_event e, oclCmdQue que)
        : Event(e), Queue(std::move(que)), Prev(std::move(prev)) { }
    ~oclPromiseCore()
    {
        if (Event)
            clReleaseEvent(Event);
    }
    void Flush()
    {
        clFlush(Queue->CmdQue);
    }
    common::PromiseState State()
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
    void Wait()
    {
        clWaitForEvents(1, &Event);
    }
    uint64_t ElapseNs()
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
    uint64_t ChainedElapseNs()
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
    const cl_event& GetEvent() { return Event; }
};

template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
    //friend class oclBuffer_;
    //friend class oclImage_;
private:
    using RDataType = std::conditional_t<std::is_same_v<T, void>, uint32_t, T>;
    std::variant<std::monostate, std::exception_ptr, RDataType> Result;
    virtual void PreparePms() override
    {
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
        Flush();
    }
    common::PromiseState virtual State() override
    {
        if (Result.index() == 1)
            return common::PromiseState::Error;
        return oclPromiseCore::State();
    }
    T WaitPms() override
    {
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
        oclPromiseCore::Wait();
        if constexpr(!std::is_same_v<T, void>)
            return std::move(std::get<2>(Result));
    }

public:
    template<typename P>
    oclPromise(P&& prev, const cl_event e, oclCmdQue que)
        : oclPromiseCore(std::forward<P>(prev), e, std::move(que)) { }
    template<typename P>
    oclPromise(P&& prev, const cl_event e, oclCmdQue que, RDataType data)
        : oclPromiseCore(std::forward<P>(prev), e, std::move(que)), Result(std::move(data)) { }

    ~oclPromise() override { }
    uint64_t ElapseNs() override 
    { 
        return oclPromiseCore::ElapseNs();
    }
    uint64_t ChainedElapseNs() override 
    { 
        return oclPromiseCore::ChainedElapseNs();
    };


    static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que);
    }
    template<typename U>
    static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que, std::forward<U>(data));
    }
    static std::shared_ptr<oclPromise> Create(std::shared_ptr<oclPromiseCore> prev, const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::move(prev), e, que);
    }
    template<typename U>
    static std::shared_ptr<oclPromise> Create(std::shared_ptr<oclPromiseCore> prev, const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::move(prev), e, que, std::forward<U>(data));
    }
    static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>> prev, const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::move(prev), e, que);
    }
    template<typename U>
    static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>> prev, const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::move(prev), e, que, std::forward<U>(data));
    }
};



}
