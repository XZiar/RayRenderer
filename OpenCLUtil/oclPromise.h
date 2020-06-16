#pragma once

#include "oclRely.h"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclContext_;
class oclBuffer_;
class oclImage_;
class oclKernel_;
template<typename T>
class oclPromise;


class OCLUAPI COMMON_EMPTY_BASES DependEvents : public common::NonCopyable
{
    friend class oclPromiseCore;
private:
    std::vector<cl_event> Events;
    std::vector<oclCmdQue> Queues;
public:
    DependEvents(const common::PromiseStub& pmss) noexcept;
    DependEvents() noexcept;

    [[nodiscard]] std::pair<const cl_event*, cl_uint> GetWaitList() const noexcept
    {
        return { Events.data(), gsl::narrow_cast<uint32_t>(Events.size()) };
    }
    void FlushAllQueues() const;
};

class OCLUAPI oclPromiseCore
{
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclKernel_;
    friend class DependEvents;
private:
    static void CL_CALLBACK EventCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
protected:
    DependEvents Depends;
    const cl_event Event;
    const oclCmdQue Queue;
    oclPromiseCore(DependEvents&& depend, const cl_event e, oclCmdQue que);
    ~oclPromiseCore();
    void Flush();
    [[nodiscard]] common::PromiseState QueryState() noexcept;
    [[nodiscard]] std::optional<common::BaseException> Wait() noexcept;
    bool RegisterCallback(const common::PmsCore& pms);
    [[nodiscard]] uint64_t ElapseNs() noexcept;
    [[nodiscard]] const cl_event& GetEvent() { return Event; }
    virtual void ExecutePmsCallbacks() = 0;
public:
    enum class TimeType { Queued, Submit, Start, End };
    uint64_t QueryTime(TimeType type) const noexcept;
    std::string_view GetEventName() const noexcept;
};

class oclCustomEvent : public ::common::detail::PromiseResult_<void>, public oclPromiseCore
{
    friend class oclContext_;
private:
    MAKE_ENABLER();
    common::PmsCore Pms;
    void PreparePms() override
    {
        Pms->Prepare();
    }
    [[nodiscard]] common::PromiseState GetState() noexcept override
    {
        return Pms->State();
    }
    void MakeActive(common::PmsCore&&) override
    { }
    common::PromiseState WaitPms() noexcept override
    {
        Pms->WaitFinish();
        return GetState();
    }
    void GetResult() override
    { }
    void ExecutePmsCallbacks() override
    {
        this->ExecuteCallback();
    }
    oclCustomEvent(common::PmsCore&& pms, cl_event evt);
    void Init();
public:
    ~oclCustomEvent() override;
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
        return this->QueryState();
    }
    void MakeActive(common::PmsCore&& pms) override
    {
        if (this->RegisterCallback(pms))
            return;
        common::detail::PromiseResultCore::MakeActive(std::move(pms));
    }
    common::PromiseState WaitPms() noexcept override
    {
        auto ret = oclPromiseCore::Wait();
        if (ret.has_value())
            this->Result = common::detail::ExceptionResult<std::shared_ptr<common::BaseException>>{ ret.value().Share() };
        return this->QueryState();
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
    oclPromise(std::exception_ptr ex) : 
        oclPromiseCore({}, nullptr, {})
    {
        this->Result = common::detail::ExceptionResult<std::exception_ptr>{ ex };
    }
    oclPromise(DependEvents&& depend, const cl_event e, const oclCmdQue& que) : 
        oclPromiseCore(std::move(depend), e, que) 
    { }
    template<typename U>
    oclPromise(DependEvents&& depend, const cl_event e, const oclCmdQue& que, U&& data) : 
        oclPromiseCore(std::move(depend), e, que)
    {
        this->Result = std::forward<U>(data);
    }
    ~oclPromise() override { }

    [[nodiscard]] uint64_t ElapseNs() noexcept override
    { 
        return oclPromiseCore::ElapseNs();
    }


    [[nodiscard]] static std::shared_ptr<oclPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<oclPromise>(ex);
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
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(DependEvents&& depend, const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::move(depend), e, que);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(DependEvents&& depend, const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::move(depend), e, que, std::forward<U>(data));
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
