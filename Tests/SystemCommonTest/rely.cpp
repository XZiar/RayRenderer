#include "rely.h"
#include "../Shared/GTestCommon.inl"


int main(int argc, char **argv)                  
{
    printf("Running main() from %s\n", __FILE__);
    ProcessTestArg({ argv, static_cast<size_t>(argc) });
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new CPUEnvironment());
    return RUN_ALL_TESTS();
}
