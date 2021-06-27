#pragma once
#include "common/Exceptions.hpp"

namespace ft
{

class [[nodiscard]] FTException : public common::BaseException
{
    PREPARE_EXCEPTION(FTException, BaseException,
        ExceptionInfo(const std::u16string_view msg) : TPInfo(TYPENAME, msg) { }
    );
    FTException(const std::u16string_view msg) : BaseException(T_<ExceptionInfo>{}, msg) {}
};


}