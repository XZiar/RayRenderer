#include "GTestCommon.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/Format.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/ThreadEx.h"
#include "common/EasierJson.hpp"
#include "common/CharConvs.hpp"
#include <variant>
#include <random>

#if COMMON_COMPILER_CLANG && __clang_major__ >= 13 && __clang_major__ <= 14 && COMMON_LIBSTDCPP_VER >= 12 && COMMON_CPP_20
// try to fix https://github.com/llvm/llvm-project/issues/55560
namespace fix::detail
{
[[maybe_unused]] static std::string    clang_string_workaround   (const char    * a, const char*     b) { return { a, b }; }
[[maybe_unused]] static std::wstring   clang_string_workaround   (const wchar_t * a, const wchar_t * b) { return { a, b }; }
[[maybe_unused]] static std::u16string clang_u16string_workaround(const char16_t* a, const char16_t* b) { return { a, b }; }
[[maybe_unused]] static std::u32string clang_u32string_workaround(const char32_t* a, const char32_t* b) { return { a, b }; }
[[maybe_unused]] static std::u8string  clang_u8string_workaround (const char8_t * a, const char8_t * b) { return { a, b }; }
}
#endif


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


struct PerfReport
{
    const common::span<const common::CPUPartition> Partitions;
    const std::vector<const common::CPUPartition*> TestTargets;
    const std::string TestName;
    std::vector<std::map<std::string, std::vector<std::pair<std::string, double>>, std::less<>>> PerfData;

    PerfReport(std::vector<const common::CPUPartition*>&& targets, std::string testName) : Partitions(common::CPUPartition::GetPartitions()),
        TestTargets(std::move(targets)), TestName(std::move(testName))
    {
        Ensures(!TestTargets.empty());
        PerfData.resize(TestTargets.size());
    }

    void Save(common::fs::path folder)
    {
        const auto sysName = common::GetSystemName();
        auto hostName = common::GetEnvVar("perfhost");
        if (hostName.empty() && !sysName.empty())
        {
            hostName = common::str::to_string(sysName, common::str::Encoding::ASCII);
        }
        if (hostName.empty())
        {
            std::mt19937 gen(std::random_device{}());
            char tmp[16] = { '\0' };
            snprintf(tmp, sizeof(tmp), "%08X", static_cast<uint32_t>(gen()));
            hostName = tmp;
        }

        auto filename = TestName + "-perf-" + hostName + ".json";
        using namespace common::file;
        FileOutputStream docWriter(FileObject::OpenThrow(folder / filename, OpenFlag::CreateNewBinary));
        xziar::ejson::JObject docRoot;
        
        docRoot.Add("name", TestName);
        docRoot.Add("host", common::str::to_string(sysName, common::str::Encoding::UTF8));
        // log time
        {
            const auto time = common::SimpleTimer::getCurTime();
            docRoot.Add("time", time);
        }
        // log cpu info
        {
            auto cpu = docRoot.NewObject();
            std::string_view cpuName;
            auto cpuParts = cpu.NewArray();
            for (const auto& part : Partitions)
            {
                if (cpuName.empty())
                    cpuName = part.PackageName;
                auto cpuPart = cpuParts.NewObject();
                cpuPart.Add("name", part.PartitionName);
                cpuPart.Add("mask", part.Affinity.ToString());
                cpuParts.Push(cpuPart);
            }
            cpu.Add("name", cpuName);
            cpu.Add("partitions", cpuParts);
            docRoot.Add("cpu", cpu);
        }
        // log perf data
        {
            auto perfRoot = docRoot.NewArray();
            size_t idx = 0;
            for (const auto& testSets : PerfData)
            {
                auto perfItem = perfRoot.NewObject();

                const auto& partition = TestTargets[idx++];
                if (partition)
                {
                    const size_t partIdx = std::distance(Partitions.data(), partition);
                    perfItem.Add("partition", partIdx);
                    perfItem.Add("name", partition->PartitionName);
                }
                else
                {
                    perfItem.Add("partition", -1);
                    perfItem.Add("name", "");
                }

                auto tests = perfItem.NewObject();
                for (const auto& [testName, perfs] : testSets)
                {
                    auto testItem = tests.NewObject();
                    for (const auto& [name, time] : perfs)
                    {
                        testItem.Add(name, time);
                    }
                    tests.Add(testName, testItem);
                }
                perfItem.Add("tests", tests);

                perfRoot.Push(perfItem);
            }
            docRoot.Add("perf", perfRoot);
        }

        docRoot.Stringify(docWriter, true);
        docWriter.Flush();
    }
};


class GTestEnvironment : public ::testing::Environment
{
private:
    static inline std::unique_ptr<PerfReport> Report = {};
    std::vector<const common::CPUPartition*> TestTargets;
    std::vector<std::string_view> TestArgs;
    bool EnableReport = false;
public:
    common::fs::path ExePath;
    GTestEnvironment()
    {
        common::PrintSystemVersion();
    }
    ~GTestEnvironment() override {}

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
        // report
        if (EnableReport)
        {
            std::string testName;
            if (!ExePath.empty() && !common::fs::is_directory(ExePath))
                testName = ExePath.stem().string();

            if (testName.empty())
            {
                std::mt19937 gen(std::random_device{}());
                char tmp[16] = { '\0' };
                snprintf(tmp, sizeof(tmp), "%08XTest", static_cast<uint32_t>(gen()));
                testName = tmp;
            }
            Report = std::make_unique<PerfReport>(std::move(TestTargets), testName);
        }
    }
    void TearDown() override
    {
        if (Report)
        {
            common::fs::path folder = ExePath;
            if (!folder.empty())
            {
                if (!common::fs::is_directory(folder))
                    folder = folder.parent_path();
            }
            else
                folder = common::fs::current_path();
            Report->Save(folder);
            Report.release();
        }
    }
    void ProcessTestArg(common::span<char* const> args)
    {
        ExePath = common::file::LocateCurrentExecutable();
        if (ExePath.empty() && !args.empty())
        {
            try
            {
                ExePath = common::fs::weakly_canonical(args[0]).make_preferred();
            } 
            catch (...) { }
        }

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
            else if (arg == "-perfreport")
            {
                EnableReport = true;
            }
            else
                TestArgs.emplace_back(arg);
        }
        const auto thisThread = common::ThreadObject::GetCurrentThreadObject();
        thisThread.SetQoS(common::ThreadQoS::High);

        const common::CPUPartition* targetPartition = nullptr;
        switch (bindOption.index())
        {
        case 1:
        {
            const auto partition = std::get<1>(bindOption);
            targetPartition = partition;
            printf("Use cpu partition [%zu] [%s]\n", partition - partitions.data(), partition->PartitionName.c_str());
            thisThread.SetAffinity(partition->Affinity);
        } break;
        case 2:
        {
            const auto cpuidx = std::get<2>(bindOption);
            for (const auto& part : partitions)
            {
                if (part.Affinity.Get(cpuidx))
                {
                    targetPartition = &part;
                    break;
                }
            }
            common::ThreadAffinity affinity;
            affinity.Set(cpuidx, true);
            printf("Use cpu [%u]\n", cpuidx);
            thisThread.SetAffinity(affinity);
        } break;
        default:
            break;
        }

        if (EnableReport)
        {
            if (targetPartition)
                TestTargets.assign(1, targetPartition);
            else
            {
                TestTargets.clear();
                for (const auto& part : partitions)
                    TestTargets.emplace_back(&part);
            }
        }

        if (bindOption.index() != 0)
        {
            if (const auto affinity = thisThread.GetAffinity(); affinity)
                printf("Current affinity: %s\n", affinity->ToString().c_str());
        }
    }

    common::span<const std::string_view> GetTestArgs() const noexcept { return TestArgs; }

    static PerfReport* GetReport()
    {
        return Report.get();
    }
};


PerfTester::~PerfTester()
{
    if (const auto report = GTestEnvironment::GetReport(); report && report->TestTargets.size() > 1)
    {
        try
        {
            // reset to all cpu
            common::ThreadAffinity affinity;
            affinity.SetAll(true);
            common::ThreadObject::GetCurrentThreadObject().SetAffinity(affinity);
        }
        catch (...) { }
    }
}

size_t PerfTester::GetRunCount()
{
    if (const auto report = GTestEnvironment::GetReport(); report)
        return report->TestTargets.size();
    return 1;
}

void PerfTester::PreSelectIndex(size_t idx)
{
    const auto report = GTestEnvironment::GetReport();
    if (!report || report->TestTargets.size() <= 1) return;

    const auto& partition = *report->TestTargets[idx];
    TestCout() << "Run[" << TestName << "] on [" << partition.PartitionName << "]\n";
    common::ThreadObject::GetCurrentThreadObject().SetAffinity(partition.Affinity);
}

void PerfTester::PostProcess(size_t partIdx, std::string_view itemVar)
{
    std::sort(TimeRecords.begin(), TimeRecords.end());
    const auto keepCount = std::max<size_t>(static_cast<size_t>(TimeRecords.size() * 0.9), 1u);
    TimeRecords.resize(keepCount);
    const auto totalNs = static_cast<double>(std::accumulate(TimeRecords.begin(), TimeRecords.end(), uint64_t(0)));
    const auto nsPerRun = totalNs / keepCount;
    const auto nsPerOp = nsPerRun / OpPerRun;
    TestCout() << "[" << TestName << "]: [" << itemVar << "] takes avg[" << nsPerOp << "]ns per operation\n";

    if (const auto report = GTestEnvironment::GetReport(); report)
    {
        auto& perfPart = report->PerfData[partIdx];
        perfPart[TestName].emplace_back(itemVar, nsPerOp);
    }
}


