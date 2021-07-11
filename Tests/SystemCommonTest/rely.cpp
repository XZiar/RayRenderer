#include "rely.h"
#include "SystemCommon/SystemCommonRely.h"

class CPUEnvironment : public ::testing::Environment 
{
public:
    ~CPUEnvironment() override {}
    void SetUp() override 
    {
        std::string str;
        for (const auto feat : common::GetCPUFeatures())
        {
            if (!str.empty())
                str.append(", ");
            str.append(feat);
        }
        TestCout() << "CPU Feature: [" << str << "]\n";
    }
};
int main(int argc, char **argv)                  
{
    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new CPUEnvironment());
    return RUN_ALL_TESTS();
}
