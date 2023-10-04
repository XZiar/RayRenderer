#include "rely.h"
#include "SystemCommon/ThreadEx.h"
#include "common/CharConvs.hpp"


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
    common::PrintSystemVersion();
    const auto& partitions = common::CPUPartition::GetPartitions();
    for (const auto& part : partitions)
        printf("[%s] %s %s\n", part.PackageName.data(), part.PartitionName.c_str(), part.Affinity.ToString().c_str());
    uint32_t partidx = UINT32_MAX;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        if (arg.starts_with("-cpupart="))
        {
            arg.remove_prefix(9);
            common::StrToInt(arg, partidx);
        }
        else if (arg == "-printcpu")
        {
            common::TopologyInfo::PrintTopology();
        }
    }
    const auto thisThread = common::ThreadObject::GetCurrentThreadObject();
    thisThread.SetQoS(common::ThreadQoS::High);
    if (partidx < partitions.size())
    {
        printf("Use cpu partition [%u]\n", partidx);
        thisThread.SetAffinity(partitions[partidx].Affinity);
    }

    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new CPUEnvironment());
    return RUN_ALL_TESTS();
}
