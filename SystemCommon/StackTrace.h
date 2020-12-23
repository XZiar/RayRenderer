#pragma once
#include "SystemCommonRely.h"
#include "common/Exceptions.hpp"
#include <vector>


namespace common
{

std::vector<StackTraceItem> SYSCOMMONAPI GetStack();
#define CREATE_EXCEPTIONEX(ex, ...) ::common::BaseException::CreateWithStacks<ex>(::common::GetStack(), __VA_ARGS__)
#define COMMON_THROWEX(ex, ...) throw CREATE_EXCEPTIONEX(ex, __VA_ARGS__)

}