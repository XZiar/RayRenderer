#pragma once
#include "../Shared/GTestCommon.h"
#include "common/ResourceHelper.h"
#include "resource.h"

common::span<const std::byte> GetRandVals() noexcept;
bool IsSlimTest() noexcept;
