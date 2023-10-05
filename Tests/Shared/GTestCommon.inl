#include "GTestCommon.h"
#include "SystemCommon/ThreadEx.h"
#include "common/CharConvs.hpp"
#include <variant>


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


void ProcessTestArg(common::span<char* const> args)
{
    common::PrintSystemVersion();
    const auto& info = common::TopologyInfo::Get();
    const auto& partitions = common::CPUPartition::GetPartitions();
    for (const auto& part : partitions)
        printf("[%s] %s %s\n", part.PackageName.data(), part.PartitionName.c_str(), part.Affinity.ToString().c_str());
    std::variant<std::monostate, const common::CPUPartition*, uint32_t> bindOption;
    for (std::string_view arg : args)
    {
        if (arg.starts_with("-cpupart="))
        {
            arg.remove_prefix(9);
            uint32_t partidx = 0;
            if (common::StrToInt(arg, partidx).first)
            {
                if (partidx < partitions.size())
                    bindOption = &partitions[partidx];
            }
        }
        else if (arg.starts_with("-cpuidx="))
        {
            arg.remove_prefix(8);
            uint32_t cpuidx = 0;
            if (common::StrToInt(arg, cpuidx).first)
            {
                if (cpuidx < info.GetTotalProcessorCount())
                    bindOption = cpuidx;
            }
        }
        else if (arg == "-printcpu")
        {
            common::TopologyInfo::PrintTopology();
        }
    }
    const auto thisThread = common::ThreadObject::GetCurrentThreadObject();
    thisThread.SetQoS(common::ThreadQoS::High);
    
    switch (bindOption.index())
    {
    case 1:
    {
        const auto partition = std::get<1>(bindOption);
        printf("Use cpu partition [%zu] [%s]\n", partition - partitions.data(), partition->PartitionName.c_str());
        thisThread.SetAffinity(partition->Affinity);
    } break;
    case 2:
    {
        const auto cpuidx = std::get<2>(bindOption);
        common::ThreadAffinity affinity;
        affinity.Set(cpuidx, true);
        printf("Use cpu [%u]\n", cpuidx);
        thisThread.SetAffinity(affinity);
    } break;
    default:
        break;
    }
    if (bindOption.index() != 0)
    {
        if (const auto affinity = thisThread.GetAffinity(); affinity)
            printf("Current affinity: %s\n", affinity->ToString().c_str());
    }
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
