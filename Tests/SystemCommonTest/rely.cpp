#include "rely.h"
#include "SystemCommon/SystemCommonRely.h"


void TestIntrinComplete(common::span<const common::FastPathBase::PathInfo> supports, const common::FastPathBase& host)
{
    for (const auto& [inst, choice] : host.GetIntrinMap())
    {
        std::string allvar = "";
        for (const auto& path : supports)
        {
            if (path.FuncName == inst)
            {
                for (const auto& var : path.Variants)
                {
                    if (!allvar.empty()) allvar.append(", ");
                    allvar.append(var.MethodName);
                }
                break;
            }
        }
        TestCout() << "intrin [" << inst << "] use [" << choice << "] within [" << allvar << "]\n";
    }
    EXPECT_TRUE(host.IsComplete());
}

class CPUEnvironment : public ::testing::Environment 
{
public:
    ~CPUEnvironment() override {}
    void SetUp() override 
    {
        std::string str;
        for (const auto& feat : common::GetCPUFeatures())
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
