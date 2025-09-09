#include "rely.h"
#include "../Shared/GTestCommon.inl"
#include "common/ResourceHelper.inl"
#include <random>


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

static bool SlimTest = true;

bool IsSlimTest() noexcept
{
    return SlimTest;
}


void TestResizeImage(std::string filepath, std::string_view reader, std::string_view writer);

int main(int argc, char **argv)                  
{
    common::ResourceHelper::Init(nullptr);
    const auto env = new GTestEnvironment();
    testing::InitGoogleTest(&argc, argv);
    env->ProcessTestArg({ argv, static_cast<size_t>(argc) });
    printf("Running main() from %s\n", env->ExePath.empty() ? __FILE__ : env->ExePath.string().c_str());
    testing::AddGlobalTestEnvironment(env);
    std::optional<std::string_view> testimg;
    std::string_view reader, writer;
    for (const auto arg : env->GetTestArgs())
    {
        if (arg.starts_with("-testimg="))
            testimg = arg.substr(9);
        else if (arg.starts_with("-testimgwriter="))
            writer = arg.substr(15);
        else if (arg.starts_with("-testimgreader="))
            reader = arg.substr(15);
        else if (arg == "-fulltest")
            SlimTest = false;
    }
    if (testimg)
    {
        TestResizeImage(std::string(*testimg), reader, writer);
        return 0;
    }
    return RUN_ALL_TESTS();
}
