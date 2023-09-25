#pragma once

#include "SystemCommonRely.h"
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <tuple>

#if COMMON_COMPILER_MSVC
#   pragma warning(disable: 5046)
#endif

namespace common::fastpath
{
struct FuncVarBase
{
    static bool RuntimeCheck() noexcept { return true; }
};

struct PathHack
{
    template<typename T, auto F>
    static void*& Access(FastPathBase& base) noexcept { return *reinterpret_cast<void**>(&(static_cast<T&>(base).*F)); }
    template<typename T>
    static auto HackFunc() noexcept;
    template<typename... Ts>
    forceinline static bool CheckExixts(const Ts&... args) noexcept
    {
        return (... && args);
    }
};

template <typename T> struct PathInfo;
template <typename R, typename... A>
struct PathInfo<R(*)(A...) noexcept>
{
    using TFunc = R(A...) noexcept;
    using Ret = R;
    template<size_t I> using Arg = std::tuple_element_t<I, std::tuple<A...>>;
};


}


#define GET_FASTPATH_FUNC(func) PPCAT(func, FastPath)
#define GET_FASTPATH_TYPE(func) PPCAT(func, Type)


#define DEFINE_FASTPATH(clz, func) struct GET_FASTPATH_FUNC(func);                                              \
template<> inline auto common::fastpath::PathHack::HackFunc<GET_FASTPATH_FUNC(func)>() noexcept                 \
{ return reinterpret_cast<std::decay_t<decltype(std::declval<const clz&>().func)>>(0); }                        \
using GET_FASTPATH_TYPE(func) = decltype(::common::fastpath::PathHack::HackFunc<GET_FASTPATH_FUNC(func)>());    \
struct GET_FASTPATH_FUNC(func) : ::common::fastpath::PathInfo<GET_FASTPATH_TYPE(func)>                          \
{                                                                                                               \
    template<typename Method> static constexpr bool MethodExist() { return false; }                             \
    template<typename Method> static TFunc Func;                                                                \
}


#define TYPE_NAME_ARG(r, host, i, name) BOOST_PP_COMMA_IF(i) typename host::Arg<i> name
#define DEFINE_FASTPATH_METHOD(func, var)                                                       \
template<> inline constexpr bool GET_FASTPATH_FUNC(func)::MethodExist<var>() { return true; }   \
template<> inline typename GET_FASTPATH_FUNC(func)::Ret GET_FASTPATH_FUNC(func)::Func<var>      \
(BOOST_PP_SEQ_FOR_EACH_I(TYPE_NAME_ARG, GET_FASTPATH_FUNC(func), func##Args)) noexcept


#define REGISTER_FASTPATH_VARIANT(r, func, var)                             \
if constexpr (GET_FASTPATH_FUNC(func)::MethodExist<var>())                  \
{                                                                           \
    if (var::RuntimeCheck())                                                \
        path.Variants.emplace_back(STRINGIZE(var),                          \
            reinterpret_cast<void*>(&GET_FASTPATH_FUNC(func)::Func<var>));  \
}
#define REGISTER_FASTPATH_VARIANTS(func, ...) do                                                    \
{                                                                                                   \
    auto& path = ret.emplace_back(STRINGIZE(func));                                                 \
    BOOST_PP_SEQ_FOR_EACH(REGISTER_FASTPATH_VARIANT, func, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
} while(0)


#define DEFINE_FASTPATH_PARTIAL(clz, scope)                                                     \
static void PushFuncs_##clz##_scope(std::vector<common::FastPathBase::PathInfo>& ret) noexcept; \
::common::span<const ::common::FastPathBase::PathInfo> clz##_##scope() noexcept                 \
{                                                                                               \
    static auto list = []()                                                                     \
        {                                                                                       \
            std::vector<::common::FastPathBase::PathInfo> ret;                                  \
            PushFuncs_##clz##_scope(ret);                                                       \
            return ret;                                                                         \
        }();                                                                                    \
        return list;                                                                            \
}                                                                                               \
void PushFuncs_##clz##_scope(std::vector<::common::FastPathBase::PathInfo>&ret) noexcept


#define DECLARE_FASTPATH_PARTIAL(r, clz, scope) ::common::span<const ::common::FastPathBase::PathInfo> PPCAT(PPCAT(clz,_),scope)() noexcept;
#define MERGE_FASTPATH_PARTIAL(r, clz, scope) if (PPCAT(FastPathCheck_,scope)()) ::common::FastPathBase::MergeInto(ret, PPCAT(PPCAT(clz,_),scope)());
#define DECLARE_FASTPATH_PARTIALS(clz, ...)                                                     \
BOOST_PP_SEQ_FOR_EACH(DECLARE_FASTPATH_PARTIAL, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))     \
static void clz##_GatherSupportMap(std::vector<::common::FastPathBase::PathInfo>& ret) noexcept \
{                                                                                               \
    BOOST_PP_SEQ_FOR_EACH(MERGE_FASTPATH_PARTIAL, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
}

#define DEFINE_FASTPATH_SCOPE(scope) static bool PPCAT(FastPathCheck_,scope)() noexcept
#define FASTPATH_ITEM(r, clz, func) ret.emplace_back(STRINGIZE(func), &::common::fastpath::PathHack::Access<clz, &clz::func>);
#define CHECK_FASTPATH_FUNC(r, state, func) state && func
#define DEFINE_FASTPATH_BASIC(clz, ...)                                                 \
span<const clz::PathInfo> clz::GetSupportMap() noexcept                                 \
{                                                                                       \
    static auto list = []()                                                             \
    {                                                                                   \
        std::vector<::common::FastPathBase::PathInfo> ret;                              \
        BOOST_PP_SEQ_FOR_EACH(FASTPATH_ITEM, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))\
        clz##_GatherSupportMap(ret);                                                    \
        return ret;                                                                     \
    }();                                                                                \
    return list;                                                                        \
}                                                                                       \
clz::clz(span<const VarItem> requests) noexcept { Init(requests); }                     \
clz::~clz() {}                                                                          \
bool clz::IsComplete() const noexcept                                                   \
{                                                                                       \
    return ::common::fastpath::PathHack::CheckExixts(__VA_ARGS__);                      \
}
