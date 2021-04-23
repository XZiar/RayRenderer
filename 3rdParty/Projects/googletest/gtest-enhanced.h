#pragma once

#ifndef gtest_EXPORTS
#   define GTEST_LINKED_AS_SHARED_LIBRARY 1
#endif

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
#include "../../googletest/googletest/include/gtest/gtest.h"
#include "../../googletest/googlemock/include/gmock/gmock.h"
#ifdef _MSC_VER
#   pragma warning(pop)
#endif
#include <sstream>
#include <type_traits>


class TestCout 
{
    std::stringstream Source;
public:
    template<typename T>
    TestCout& operator<<(T&& arg)
    {
        Source << std::forward<T>(arg);
        return *this;
    }
    GTEST_API_ ~TestCout();
};


#define GTEST_DEFAULT_MAIN                          \
int main(int argc, char **argv)                     \
{                                                   \
    printf("Running main() from %s\n", __FILE__);   \
    testing::InitGoogleTest(&argc, argv);           \
    return RUN_ALL_TESTS();                         \
}


