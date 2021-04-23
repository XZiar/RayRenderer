#include "gtest-enhanced.h"
#include "src/gtest-all.cc"
#include "src/gmock-all.cc"


TestCout::~TestCout()
{
    testing::internal::ColoredPrintf(testing::internal::GTestColor::kGreen, "[          ] ");
    testing::internal::ColoredPrintf(testing::internal::GTestColor::kYellow, "%s", Source.str().c_str());
}
