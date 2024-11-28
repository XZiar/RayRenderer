#pragma once

#include "SystemCommonRely.h"
#if !COMMON_COMPILER_MSVC || (defined(_MSVC_TRADITIONAL) && !_MSVC_TRADITIONAL)
#   define BOOST_PP_LIMIT_TUPLE 128
#endif
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/seq.hpp>
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


#define FASTPATH_INFO_HEAD(x) FASTPATH_INFO_1 x
#define FASTPATH_INFO_TAIL(x) FASTPATH_INFO_2 x
#define FASTPATH_INFO_ALL(...) __VA_ARGS__ 
#define FASTPATH_INFO_NIL(...) 
#define FASTPATH_INFO_1(...) __VA_ARGS__ FASTPATH_INFO_NIL
#define FASTPATH_INFO_2(...) FASTPATH_INFO_ALL


#define GET_FASTPATH_FUNC(func) PPCAT(func, FastPath)


#define DEFINE_FASTPATHS_EACH(r, clz, func) struct GET_FASTPATH_FUNC(func)      \
{                                                                               \
    template<typename Method> static inline constexpr bool MethodExist = false; \
    template<typename Method> static FASTPATH_INFO_HEAD(PPCAT(func, Info))      \
        Func(FASTPATH_INFO_TAIL(PPCAT(func, Info))) noexcept;                   \
};
#define DEFINE_FASTPATHS(clz, ...) BOOST_PP_SEQ_FOR_EACH(DEFINE_FASTPATHS_EACH, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))


#define DEFINE_FASTPATH_METHOD(func, var)                                           \
template<> inline constexpr bool GET_FASTPATH_FUNC(func)::MethodExist<var> = true;  \
template<> inline FASTPATH_INFO_HEAD(PPCAT(func, Info)) GET_FASTPATH_FUNC(func)::   \
    Func<var>(FASTPATH_INFO_TAIL(PPCAT(func, Info))) noexcept


#define REGISTER_FASTPATH_VARIANT(r, func, var)                             \
if constexpr (GET_FASTPATH_FUNC(func)::MethodExist<var>)                    \
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


#define DEFINE_FASTPATH_PARTIAL(clz, scope)                                                         \
static void PushFuncs_##clz##_scope(::std::vector<::common::FastPathBase::PathInfo>& ret) noexcept; \
::common::span<const ::common::FastPathBase::PathInfo> clz##_##scope() noexcept                     \
{                                                                                                   \
    static auto list = []()                                                                         \
        {                                                                                           \
            ::std::vector<::common::FastPathBase::PathInfo> ret;                                    \
            PushFuncs_##clz##_scope(ret);                                                           \
            return ret;                                                                             \
        }();                                                                                        \
        return list;                                                                                \
}                                                                                                   \
void PushFuncs_##clz##_scope(::std::vector<::common::FastPathBase::PathInfo>&ret) noexcept


#define DECLARE_FASTPATH_PARTIAL(r, clz, scope) ::common::span<const ::common::FastPathBase::PathInfo> PPCAT(PPCAT(clz,_),scope)() noexcept;
#define MERGE_FASTPATH_PARTIAL(r, clz, scope) if (PPCAT(FastPathCheck_,scope)()) ::common::FastPathBase::MergeInto(ret, PPCAT(PPCAT(clz,_),scope)());
#define DECLARE_FASTPATH_PARTIALS(clz, ...)                                                         \
BOOST_PP_SEQ_FOR_EACH(DECLARE_FASTPATH_PARTIAL, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))         \
static void clz##_GatherSupportMap(::std::vector<::common::FastPathBase::PathInfo>& ret) noexcept   \
{                                                                                                   \
    BOOST_PP_SEQ_FOR_EACH(MERGE_FASTPATH_PARTIAL, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))       \
}

#define DEFINE_FASTPATH_SCOPE(scope) static bool PPCAT(FastPathCheck_,scope)() noexcept
#define FASTPATH_ITEM(r, clz, func) ret.emplace_back(STRINGIZE(func), &::common::fastpath::PathHack::Access<clz, &clz::func>);
#define CHECK_FASTPATH_FUNC(r, state, func) state && func
#define DEFINE_FASTPATH_BASIC(clz, ...)                                                 \
::common::span<const clz::PathInfo> clz::GetSupportMap() noexcept                       \
{                                                                                       \
    static auto list = []()                                                             \
    {                                                                                   \
        ::std::vector<::common::FastPathBase::PathInfo> ret;                            \
        BOOST_PP_SEQ_FOR_EACH(FASTPATH_ITEM, clz, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))\
        clz##_GatherSupportMap(ret);                                                    \
        return ret;                                                                     \
    }();                                                                                \
    return list;                                                                        \
}                                                                                       \
clz::clz(::common::span<const VarItem> requests) noexcept { Init(requests); }           \
clz::~clz() {}                                                                          \
bool clz::IsComplete() const noexcept                                                   \
{                                                                                       \
    return ::common::fastpath::PathHack::CheckExixts(__VA_ARGS__);                      \
}
