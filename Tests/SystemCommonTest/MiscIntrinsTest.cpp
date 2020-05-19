#include "rely.h"
#include <algorithm>
#include "SystemCommon/MiscIntrins.h"


using namespace std::string_view_literals;
using common::MiscIntrins;
using common::DigestFuncs;


TEST(MiscIntrins, Complete)
{

    for (const auto& [inst, var] : common::MiscIntrin.GetIntrinMap())
    {
        TestCout() << "intrin [" << inst << "] use [" << var << "]\n";
    }
    ASSERT_TRUE(common::MiscIntrin.IsComplete());
}

INTRIN_TEST(MiscIntrins, LeadZero32)
{
    EXPECT_EQ(Intrin->LeadZero<uint32_t>(0u),           32u);
    EXPECT_EQ(Intrin->LeadZero<uint32_t>(UINT32_MAX),   0u);
    EXPECT_EQ(Intrin->LeadZero<uint32_t>(0b0001111u),   28u);
    EXPECT_EQ(Intrin->LeadZero<uint32_t>(0b11001100u),  24u);
    EXPECT_EQ(Intrin->LeadZero<int32_t> (INT32_MIN),    0u);
}

INTRIN_TEST(MiscIntrins, LeadZero64)
{
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(0ull),             64u);
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(UINT64_MAX),       0u);
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(0b0001111ull),     60u);
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(0b11001100ull),    56u);
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(0xffffffffffull),  24u);
    EXPECT_EQ(Intrin->LeadZero<uint64_t>(0xf00000000ull),   28u);
    EXPECT_EQ(Intrin->LeadZero<int64_t> (INT64_MIN),        0u);
}

INTRIN_TEST(MiscIntrins, TailZero32)
{
    EXPECT_EQ(Intrin->TailZero<uint32_t>(0u),           32u);
    EXPECT_EQ(Intrin->TailZero<uint32_t>(UINT32_MAX),   0u);
    EXPECT_EQ(Intrin->TailZero<uint32_t>(0b0001111u),   0u);
    EXPECT_EQ(Intrin->TailZero<uint32_t>(0b11001100u),  2u);
    EXPECT_EQ(Intrin->TailZero<int32_t> (INT32_MIN),    31u);
}

INTRIN_TEST(MiscIntrins, TailZero64)
{
    EXPECT_EQ(Intrin->TailZero<uint64_t>(0ull),             64u);
    EXPECT_EQ(Intrin->TailZero<uint64_t>(UINT64_MAX),       0u);
    EXPECT_EQ(Intrin->TailZero<uint64_t>(0b0001111ull),     0u);
    EXPECT_EQ(Intrin->TailZero<uint64_t>(0b11001100ull),    2u);
    EXPECT_EQ(Intrin->TailZero<uint64_t>(0xf0000fffffull),  0u);
    EXPECT_EQ(Intrin->TailZero<uint64_t>(0xf000000000ull),  36u);
    EXPECT_EQ(Intrin->TailZero<int64_t> (INT64_MIN),        63u);
}

INTRIN_TEST(MiscIntrins, PopCount32)
{
    EXPECT_EQ(Intrin->PopCount<uint32_t>(0u),           0u);
    EXPECT_EQ(Intrin->PopCount<uint32_t>(UINT32_MAX),   32u);
    EXPECT_EQ(Intrin->PopCount<uint32_t>(0b0001111u),   4u);
    EXPECT_EQ(Intrin->PopCount<uint32_t>(0b11001100u),  4u);
    EXPECT_EQ(Intrin->PopCount<int32_t> (INT32_MIN),    1u);
}

INTRIN_TEST(MiscIntrins, PopCount64)
{
    EXPECT_EQ(Intrin->PopCount<uint64_t>(0ull),             0u);
    EXPECT_EQ(Intrin->PopCount<uint64_t>(UINT64_MAX),       64u);
    EXPECT_EQ(Intrin->PopCount<uint64_t>(0b0001111ull),     4u);
    EXPECT_EQ(Intrin->PopCount<uint64_t>(0b11001100ull),    4u);
    EXPECT_EQ(Intrin->PopCount<uint64_t>(0xf0000fffffull),  24u);
    EXPECT_EQ(Intrin->PopCount<uint64_t>(0xf000000000ull),  4u);
    EXPECT_EQ(Intrin->PopCount<int64_t> (INT64_MIN),        1u);
}


TEST(DigestFuncs, Complete)
{

    for (const auto& [inst, var] : common::DigestFunc.GetIntrinMap())
    {
        TestCout() << "intrin [" << inst << "] use [" << var << "]\n";
    }
    ASSERT_TRUE(common::DigestFunc.IsComplete());
}

template<size_t N>
std::string Hex2Str(const std::array<std::byte, N>& data)
{
    constexpr auto ch = "0123456789abcdef";
    std::string ret;
    ret.reserve(N * 2);
    for (size_t i = 0; i < N; ++i)
    {
        const uint8_t dat = static_cast<uint8_t>(data[i]);
        ret.push_back(ch[dat / 16]);
        ret.push_back(ch[dat % 16]);
    }
    return ret;
}

// Test cases come from https://www.di-mgt.com.au/sha_testvectors.html

INTRIN_TEST(DigestFuncs, Sha256)
{
    const auto SHA256 = [&](std::string_view dat) 
    {
        return Hex2Str(Intrin->SHA256(common::to_span(dat)));
    };
    {
        // 0
        EXPECT_EQ(SHA256(""),
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        // 64
        EXPECT_EQ(SHA256("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-"),
            "346ed961649e04951caf255f18214542cc33a81c2af7e00bf56bb1f9b8f0119e");
        // 3
        EXPECT_EQ(SHA256("abc"),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        // 56
        EXPECT_EQ(SHA256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
        // 112
        EXPECT_EQ(SHA256("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
            "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
    }
}