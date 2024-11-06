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

void TestResizeImage(std::string filepath);

int main(int argc, char **argv)                  
{
    common::ResourceHelper::Init(nullptr);
    const auto env = new GTestEnvironment();
    testing::InitGoogleTest(&argc, argv);
    env->ProcessTestArg({ argv, static_cast<size_t>(argc) });
    printf("Running main() from %s\n", env->ExePath.empty() ? __FILE__ : env->ExePath.string().c_str());
    testing::AddGlobalTestEnvironment(env);
    for (const auto arg : env->GetTestArgs())
    {
        if (arg.starts_with("-testimg="))
        {
            TestResizeImage(std::string(arg.substr(9)));
            return 0;
        }
    }
    return RUN_ALL_TESTS();
}
