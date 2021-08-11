#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <optional>

namespace common::str::parsepack
{

#define STR_SWITCH_PACK_CASE(r, _, tp) HashCase(arg, BOOST_PP_TUPLE_ELEM(2, 0, tp)) return BOOST_PP_TUPLE_ELEM(2, 1, tp);
#define STR_SWITCH_PACK_CASES(...) BOOST_PP_SEQ_FOR_EACH(STR_SWITCH_PACK_CASE, x, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SWITCH_PACK(tp, ...) [](){                      \
    constexpr auto sv = ::common::str::ToStringView(BOOST_PP_TUPLE_ELEM(2, 0, tp)); \
    using K = decltype(sv);                             \
    using V = decltype(BOOST_PP_TUPLE_ELEM(2, 1, tp));  \
    return [](K arg) -> std::optional<V>                \
    {                                                   \
        switch (::common::DJBHash::HashC(arg))          \
        {                                               \
        STR_SWITCH_PACK_CASES(tp, __VA_ARGS__)          \
        default: break;                                 \
        }                                               \
        return {};                                      \
    }; }()                                              \


}