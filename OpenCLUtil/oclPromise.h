#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclBuffer_;
class oclImage_;
class oclKernel_;


class OCLUAPI COMMON_EMPTY_BASES DependEvents : public common::NonCopyable
{
private:
    std::vector<cl_event> Events;
    uint32_t DirectCount;
    DependEvents(std::vector<cl_event>&& event, uint32_t direct);
public:
    ~DependEvents();
    std::pair<const cl_event*, cl_uint> GetCLWait() const noexcept
    {
        if (Events.empty())
            return { nullptr, 0 };
        else
            return { Events.data(), DirectCount };
    }
    static DependEvents Create(const common::PromiseStub& pmss) noexcept;
};

class OCLUAPI oclPromiseCore
{
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclKernel_;
    friend class DependEvents;
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
    static void CL_CALLBACK EventCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
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
    oclPromiseCore(PrevType&& prev, const cl_event e, oclCmdQue que);
    ~oclPromiseCore();
    void Flush();
    [[nodiscard]] common::PromiseState QueryState() noexcept;
    void Wait();
    bool RegisterCallback(const common::detail::PmsCore& pms);
    [[nodiscard]] uint64_t ElapseNs() noexcept;
    [[nodiscard]] uint64_t ChainedElapseNs() noexcept;
    [[nodiscard]] const cl_event& GetEvent() { return Event; }
    virtual void ExecutePmsCallbacks() = 0;
public:
    [[nodiscard]] static std::pair<std::vector<std::shared_ptr<oclPromiseCore>>, oclEvents>
        ParsePms(const common::PromiseStub& pmss) noexcept;

    enum class TimeType { Queued, Submit, Start, End };
    uint64_t QueryTime(TimeType type) const noexcept;
    std::string_view GetEventName() const noexcept;
};


template<typename T>
class COMMON_EMPTY_BASES oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore, protected common::detail::ResultExHolder<T>
{
    friend class oclPromiseCore;
private:
    using RH = common::detail::ResultExHolder<T>;
    void PreparePms() override
    {
        if (!this->IsException())
            Flush();
    }
    [[nodiscard]] common::PromiseState GetState() noexcept override
    {
        if (this->IsException())
            return common::PromiseState::Error;
        return oclPromiseCore::QueryState();
    }
    void MakeActive(common::detail::PmsCore&& pms) override
    {
        if (RegisterCallback(pms))
            return;
        common::detail::PromiseResultCore::MakeActive(std::move(pms));
    }
    void WaitPms() noexcept override
    {
        oclPromiseCore::Wait();
    }
    [[nodiscard]] T GetResult() override
    {
        if constexpr (!std::is_same_v<T, void>)
        {
            this->CheckResultExtracted();
            return this->ExtraResult();
        }
    }
    void ExecutePmsCallbacks() override 
    {
        this->ExecuteCallback();
    }
public:
    oclPromise(common::detail::ExceptionResult<std::exception_ptr> ex)
        : oclPromiseCore(std::monostate{}, nullptr, {}), RH(ex) { }
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que)
        : oclPromiseCore(std::move(prev), e, que) { }
    template<typename U>
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que, U&& data)
        : oclPromiseCore(std::move(prev), e, que)
    {
        this->Result = std::forward<U>(data);
    }

    ~oclPromise() override { }
    [[nodiscard]] uint64_t ElapseNs() noexcept override
    { 
        return oclPromiseCore::ElapseNs();
    }
    [[nodiscard]] uint64_t ChainedElapseNs() noexcept override
    { 
        return oclPromiseCore::ChainedElapseNs();
    };


    [[nodiscard]] static std::shared_ptr<oclPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<oclPromise>(common::detail::ExceptionResult<std::exception_ptr>(ex));
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

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
