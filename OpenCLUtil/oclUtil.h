#pragma once

#include "oclRely.h"


namespace oclu
{


class oclUtil
{
    friend class GLInterop;
private:
    OCLUAPI [[nodiscard]] static common::mlog::MiniLogger<false>& GetOCLLog();
    OCLUAPI static bool InsertInitFilter_(std::function<bool(const detail::oclCommon&)> filter) noexcept;
public:
    OCLUAPI static void LogCLInfo();
    OCLUAPI [[nodiscard]] static std::u16string_view GetErrorString(const int32_t err);
    OCLUAPI static void WaitMany(common::PromiseStub promises);
    template<typename F>
    static bool InsertInitFilter(F&& filter) noexcept
    {
        static_assert(std::is_invocable_r_v<bool, F, const detail::oclCommon&>, "filter need to accepe const detail::oclCommon& and return bool");
        static_assert(noexcept(filter(std::declval<const detail::oclCommon&>())), "filter need to be noexcept");
        return InsertInitFilter_(std::forward<F>(filter));
    }
};

}
