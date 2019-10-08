//
// pch.cpp
// Include the standard header and generate the precompiled header.
//

#include "pch.h"

std::mt19937& GetRanEng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
uint32_t GetARand()
{
    return GetRanEng()();
}