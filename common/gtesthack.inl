#include "gtesthack.h"
#include "3rdParty/gtest/googletest/src/gtest-all.cc"


TestCout::~TestCout()
{
    testing::internal::ColoredPrintf(testing::internal::GTestColor::kGreen, "[          ] ");
    testing::internal::ColoredPrintf(testing::internal::GTestColor::kYellow, "%s", str().c_str());
}
