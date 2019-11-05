#pragma once

#include "oclPch.h"
#include "oclCmdQue.h"

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
private:
    struct oclEvents
    {
    private:
        std::optional<std::vector<cl_event>> Events;
    public:
        constexpr oclEvents() {}
        constexpr oclEvents(std::vector<cl_event>&& evts) : Events(std::move(evts)) {}
        std::pair<const cl_event*, cl_uint> Get() const noexcept
        {
            if (Events.has_value())
                return { Events.value().data(), static_cast<cl_uint>(Events.value().size()) };
            else
                return { nullptr, 0 };
        }
    };
protected:
    using PrevType = std::variant<std::monostate, std::vector<std::shared_ptr<oclPromiseCore>>, std::shared_ptr<oclPromiseCore>>;
    [[nodiscard]] static PrevType TrimPms(std::vector<std::shared_ptr<oclPromiseCore>>&& pmss) noexcept
    {
        switch (pmss.size())
        {
        case 0:  return std::monostate{};
        case 1:  return pmss[0];
        default: return std::move(pmss);
        }
    }

    const cl_event Event;
    const oclCmdQue Queue;
    PrevType Prev;
    oclPromiseCore(PrevType&& prev, const cl_event e, oclCmdQue que)
        : Event(e), Queue(std::move(que)), Prev(std::move(prev)) { }
    ~oclPromiseCore()
    {
        if (Event)
            clReleaseEvent(Event);
    }
    void Flush()
    {
        if (Queue)
            clFlush(Queue->CmdQue);
    }
    [[nodiscard]] common::PromiseState State()
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
        if (Event)
            clWaitForEvents(1, &Event);
    }
    [[nodiscard]] uint64_t ElapseNs()
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
    [[nodiscard]] uint64_t ChainedElapseNs()
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
    [[nodiscard]] const cl_event& GetEvent() { return Event; }
public:
    [[nodiscard]] static std::pair<std::vector<std::shared_ptr<oclPromiseCore>>, oclEvents>
        ParsePms(const common::PromiseStub& pmss) noexcept
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
};


template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
private:
    using RDataType = std::conditional_t<std::is_same_v<T, void>, uint32_t, T>;
    std::variant<std::monostate, std::exception_ptr, RDataType> Result;
    virtual void PreparePms() override
    {
        Flush();
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
    }
    [[nodiscard]] common::PromiseState virtual State() override
    {
        if (Result.index() == 1)
            return common::PromiseState::Error;
        return oclPromiseCore::State();
    }
    [[nodiscard]] T WaitPms() override
    {
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
        oclPromiseCore::Wait();
        if constexpr(!std::is_same_v<T, void>)
            return std::move(std::get<2>(Result));
    }

public:
    oclPromise(std::exception_ptr ex)
        : oclPromiseCore(std::monostate{}, nullptr, {}), Result(ex) { }
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que)
        : oclPromiseCore(std::move(prev), e, que) { }
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que, RDataType&& data)
        : oclPromiseCore(std::move(prev), e, que), Result(std::move(data)) { }

    ~oclPromise() override { }
    [[nodiscard]] uint64_t ElapseNs() override
    { 
        return oclPromiseCore::ElapseNs();
    }
    [[nodiscard]] uint64_t ChainedElapseNs() override
    { 
        return oclPromiseCore::ChainedElapseNs();
    };


    [[nodiscard]] static std::shared_ptr<oclPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<oclPromise>(std::monostate{}, nullptr, {});
    }
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que, std::forward<U>(data));
    }
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>>&& prev, const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(TrimPms(std::move(prev)), e, que);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>>&& prev, const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(TrimPms(std::move(prev)), e, que, std::forward<U>(data));
    }
};



}
