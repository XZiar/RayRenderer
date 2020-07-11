#pragma once
#include <sstream>


class TestCout : public std::stringstream
{
public:
    ~TestCout();
};