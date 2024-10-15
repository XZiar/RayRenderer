#include "rely.h"
#include "../Shared/GTestCommon.inl"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/Format.h"
#include <csignal>


void sighandler(int)
{
    const auto stack = common::GetStack();
    common::str::Formatter<char> fmter;
    for (const auto& s : stack)
    {
        const auto txt = fmter.FormatStatic(FmtString("{}:{} {}\n"), (std::u16string_view)s.File, s.Line, (std::u16string_view)s.Func);
        printf("%s", txt.c_str());
    }
    abort();
    //raise(SIGILL);
}

int main(int argc, char **argv)                  
{
    const auto env = new GTestEnvironment();
    testing::InitGoogleTest(&argc, argv);
    env->ProcessTestArg({ argv, static_cast<size_t>(argc) });
    printf("Running main() from %s\n", env->ExePath.empty() ? __FILE__ : env->ExePath.string().c_str());
    testing::AddGlobalTestEnvironment(env);
    std::signal(SIGILL, sighandler);
    return RUN_ALL_TESTS();
}
