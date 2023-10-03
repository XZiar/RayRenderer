#include "rely.h"
#include "SystemCommon/ThreadEx.h"
#include <random>


void TestIntrinComplete(common::span<const common::FastPathBase::PathInfo> supports, common::span<const common::FastPathBase::VarItem> intrinMap, bool isComplete)
{
    for (const auto& [inst, choice] : intrinMap)
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
    EXPECT_TRUE(isComplete);
}

common::span<const std::byte> GetRandVals() noexcept
{
    static const std::array<uint8_t, 2048> RandVals = []()
    {
        std::mt19937 gen(std::random_device{}());
        std::array<uint8_t, 2048> vals = {};
        for (auto& val : vals)
            val = static_cast<uint8_t>(gen());
        return vals;
    }();
    return { reinterpret_cast<const std::byte*>(RandVals.data()), RandVals.size() };
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
        common::SetThreadQoS(common::ThreadQoS::High);
    }
};
int main(int argc, char **argv)                  
{
    printf("Running main() from %s\n", __FILE__);
    common::PrintSystemVersion();
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new CPUEnvironment());
    return RUN_ALL_TESTS();
}
