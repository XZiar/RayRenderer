#pragma once

#include "DxRely.h"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{


class DXUAPI DxPromiseCore : public common::PromiseProvider
{
    friend class DependEvents;
protected:
    void* Handle;
    std::shared_ptr<common::ExceptionBasicInfo> WaitException;
    bool IsException;
    [[nodiscard]] common::PromiseState GetState() noexcept override;
    [[nodiscard]] common::PromiseState WaitPms() noexcept override;
    [[nodiscard]] uint64_t ElapseNs() noexcept override;
public:
    DxPromiseCore(void* handle, const bool isException = false);
    ~DxPromiseCore();
    std::optional<common::BaseException> GetException() const;
};

template<typename T>
class COMMON_EMPTY_BASES DxPromise : public common::detail::PromiseResult_<T>
{
    friend class DxPromiseCore;
private:
    common::CachedPromiseProvider<DxPromiseCore> Promise;
    common::detail::ResultExHolder<T> Holder;
    [[nodiscard]] T GetResult() override
    {
        this->CheckResultExtracted();
        if (auto ex = Promise.GetException())
            Holder.SetException(*ex);
        return Holder.ExtraResult();
    }
public:
    DxPromise(std::exception_ptr ex) : Promise(nullptr, true)
    {
        Holder.SetException(ex);
    }
    DxPromise(const DxException& ex) : Promise(nullptr, true)
    {
        Holder.SetException(ex);
    }
    DxPromise(void* handle) : Promise(handle)
    { }
    template<typename U>
    DxPromise(void* handle, U&& data) : Promise(handle)
    {
        Holder.Result = std::forward<U>(data);
    }
    ~DxPromise() override { }
    DxPromiseCore& GetPromise() noexcept override
    {
        return Promise;
    }

    [[nodiscard]] static std::shared_ptr<DxPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<DxPromise>(ex);
    }
    [[nodiscard]] static std::shared_ptr<DxPromise> CreateError(const DxException& ex)
    {
        return std::make_shared<DxPromise>(ex);
    }
    [[nodiscard]] static std::shared_ptr<DxPromise> Create(void* handle)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<DxPromise>(handle);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<DxPromise> Create(void* handle, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<DxPromise>(handle, std::forward<U>(data));
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
