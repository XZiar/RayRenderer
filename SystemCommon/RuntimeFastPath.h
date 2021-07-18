#include "SystemCommonRely.h"
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
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


template <typename T> struct PathInfo;
template <typename R, typename... A>
struct PathInfo<R(A...) noexcept>
{
    using Ret = R;
    template<size_t I> using Arg = std::tuple_element_t<I, std::tuple<A...>>;
};
}


#define GET_FASTPATH_FUNC(func) PPCAT(func, FastPath)


#define DEFINE_FASTPATH(clz, func) struct GET_FASTPATH_FUNC(func) :                 \
    ::common::fastpath::PathInfo<clz::T##func>                                      \
{                                                                                   \
    template<typename Method> static constexpr bool MethodExist() { return false; } \
    template<typename Method> static typename clz::T##func Func;                    \
}


#define TYPE_NAME_ARG(r, host, i, name) BOOST_PP_COMMA_IF(i) typename host::Arg<i> name
#define DEFINE_FASTPATH_METHOD(func, var)                                                       \
template<> inline constexpr bool GET_FASTPATH_FUNC(func)::MethodExist<var>() { return true; }   \
template<> inline typename GET_FASTPATH_FUNC(func)::Ret GET_FASTPATH_FUNC(func)::Func<var>      \
(BOOST_PP_SEQ_FOR_EACH_I(TYPE_NAME_ARG, GET_FASTPATH_FUNC(func), func##Args)) noexcept


#define RegistFuncVar(r, func, var)                                         \
if constexpr (GET_FASTPATH_FUNC(func)::MethodExist<var>())                  \
{                                                                           \
    if (var::RuntimeCheck())                                                \
        path.Variants.emplace_back(STRINGIZE(var),                          \
            reinterpret_cast<void*>(&GET_FASTPATH_FUNC(func)::Func<var>));  \
}
#define RegistFuncVars(clz, func, ...) do                                               \
{                                                                                       \
    auto& path = ret.emplace_back(STRINGIZE(func), &Access<&clz::func>);                \
    BOOST_PP_SEQ_FOR_EACH(RegistFuncVar, func, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
} while(0)

