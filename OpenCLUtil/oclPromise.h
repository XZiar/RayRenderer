#pragma once

#include "oclRely.h"



#if COMMON_COMPILER_MSVC
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

class OCLUAPI oclPromiseCore : public common::PromiseProvider
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
    std::shared_ptr<common::ExceptionBasicInfo> WaitException;
    bool IsException;
    [[nodiscard]] common::PromiseState GetState() noexcept override;
    void PreparePms() override;
    void MakeActive(common::PmsCore&& pms) override;
    [[nodiscard]] common::PromiseState WaitPms() noexcept override;
    [[nodiscard]] uint64_t ElapseNs() noexcept override;
public:
    oclPromiseCore(DependEvents&& depend, const cl_event e, oclCmdQue que, const bool isException = false);
    ~oclPromiseCore();
    enum class TimeType { Queued, Submit, Start, End };
    uint64_t QueryTime(TimeType type) const noexcept;
    bool RegisterCallback(const common::PmsCore& pms);
    std::string_view GetEventName() const noexcept;
    std::optional<common::BaseException> GetException() const;
};

class oclCustomEvent : public ::common::detail::PromiseResult_<void>, public oclPromiseCore
{
    friend class oclContext_;
private:
    MAKE_ENABLER();
    common::PmsCore Pms;
    [[nodiscard]] common::PromiseState GetState() noexcept override
    {
        return Pms->State();
    }
    void PreparePms() override
    {
        Pms->Prepare();
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
    oclCustomEvent(common::PmsCore&& pms, cl_event evt);
    void Init();
public:
    ~oclCustomEvent() override;
    common::PromiseProvider& GetPromise() noexcept override
    {
        return *this;
    }
};


template<typename T>
class COMMON_EMPTY_BASES oclPromise : public common::detail::PromiseResult_<T>
{
    friend class oclPromiseCore;
private:
    common::CachedPromiseProvider<oclPromiseCore> Promise;
    common::detail::ResultExHolder<T> Holder;
    [[nodiscard]] T GetResult() override
    {
        this->CheckResultExtracted();
        if (auto ex = Promise.GetException())
            Holder.SetException(*ex);
        return Holder.ExtraResult();
    }
public:
    oclPromise(std::exception_ptr ex) :
        Promise({}, nullptr, {}, true)
    {
        Holder.SetException(ex);
    }
    oclPromise(DependEvents&& depend, const cl_event e, const oclCmdQue& que) : 
        Promise(std::move(depend), e, que)
    { }
    template<typename U>
    oclPromise(DependEvents&& depend, const cl_event e, const oclCmdQue& que, U&& data) : 
        Promise(std::move(depend), e, que)
    {
        Holder.Result = std::forward<U>(data);
    }
    ~oclPromise() override { }
    oclPromiseCore& GetPromise() noexcept override
    {
        return Promise;
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

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
