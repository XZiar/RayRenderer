#pragma once
#include "common/CopyEx.hpp"
#pragma message("Compiling CopyTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace copytest
{
using namespace common::copy;


struct CopyTestHelper
{
    static const std::vector<uint32_t>& GetCopyRanges();
    static const std::vector<uint32_t>& GetMemSetRanges();
    template<typename T>
    static const std::vector<T>& GetRandomSources();
    template<typename Src, typename Dst>
    static const std::vector<Dst>& GetConvertData();
};


template<typename From, typename To>
class CopyTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "Copy";
    void TestBody() override 
    {
        const auto& orig = CopyTestHelper::GetRandomSources<From>();                      
        const auto& real = CopyTestHelper::GetConvertData<From, To>();               
        const auto total = orig.size();                                                
        for (const auto len : CopyTestHelper::GetCopyRanges())                         
        {                                                                              
            for (uint8_t i = 4; i--;)
            {
                const auto start = GetARand() % (total - len);
                std::vector<To> dest(len);
                CopyLittleEndian(dest.data(), dest.size(), orig.data() + start, len);
                std::vector<To> truth(real.data() + start, real.data() + start + len);
                EXPECT_THAT(dest, testing::ContainerEq(truth));
            } 
        }
    }
};


template<typename T>
class BroadcastTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "Broadcast";
    void TestBody() override
    {
        for (const auto len : CopyTestHelper::GetMemSetRanges())
        {
            for (uint8_t i = 4; i--;)
            {
                const T val = static_cast<T>(GetARand());
                const std::vector<T> truth(len, val);
                std::vector<T> raw(len, 0);
                BroadcastMany(raw.data(), raw.size(), val, len);
                EXPECT_THAT(raw, testing::ContainerEq(truth));
            }
        }
    }
};


}
