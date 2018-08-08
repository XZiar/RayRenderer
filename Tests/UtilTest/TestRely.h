#pragma once

#include "common/FileEx.hpp"
#include "common/miniLogger/miniLogger.h"
#include "common/TimeUtil.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <cinttypes>
#include <iostream>


common::fs::path FindPath();
uint32_t RegistTest(const char *name, void(*func)());
