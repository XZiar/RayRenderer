#include "rely.h"
#include <algorithm>
#include <random>
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/MiscIntrins.h"


using namespace std::string_view_literals;

INTRIN_TESTSUITE(CopyEx, common::CopyManager);
TEST_F(CopyEx, Complete)
{
    for (const auto& [inst, var] : common::CopyEx.GetIntrinMap())
    {
        TestCout() << "intrin [" << inst << "] use [" << var << "]\n";
    }
    ASSERT_TRUE(common::CopyEx.IsComplete());
}

template<typename Src>
static void BroadcastTest(const common::CopyManager& intrin, const Src src, size_t count)
{
    std::vector<Src> dst;
    dst.resize(count);
    intrin.BroadcastMany(dst.data(), src, count);
    EXPECT_THAT(dst, testing::Each(src)) << "when test on [" << count << "] elements";
}
INTRIN_TEST(CopyEx, Broadcast2)
{
    constexpr uint16_t val = 0x8023;
    BroadcastTest(*Intrin, val, 3);
    BroadcastTest(*Intrin, val, 17);
    BroadcastTest(*Intrin, val, 35);
    BroadcastTest(*Intrin, val, 68);
    BroadcastTest(*Intrin, val, 139);
}

INTRIN_TEST(CopyEx, Broadcast4)
{
    constexpr uint32_t val = 0xdeafbeefu;
    BroadcastTest(*Intrin, val, 3);
    BroadcastTest(*Intrin, val, 17);
    BroadcastTest(*Intrin, val, 35);
    BroadcastTest(*Intrin, val, 68);
    BroadcastTest(*Intrin, val, 139);
}

std::mt19937& GetRanEng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
alignas(32) const std::array<uint8_t, 2048> RandVals = []()
{
    auto& gen = GetRanEng();
    std::array<uint8_t, 2048> vals = {};
    for (auto& val : vals)
        val = static_cast<uint8_t>(gen());
    return vals;
}();

template<typename Src, typename Dst>
static void ZExtTest(const common::CopyManager* intrin, const Src* src, const Dst* ref, size_t count)
{
    std::vector<Dst> dst; 
    dst.resize(count);
    intrin->ZExtCopy(dst.data(), src, count);
    EXPECT_THAT(dst, testing::ElementsAreArray(ref, count)) << "when test on [" << count << "] elements";
}
INTRIN_TEST(CopyEx, ZExtCopy12)
{
    static const auto ref = []() 
    {
        std::array<uint16_t, 2048> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = RandVals[i];
        return vals;
    }();
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 7);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 27);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 97);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 197);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 2048);
}
INTRIN_TEST(CopyEx, ZExtCopy14)
{
    static const auto ref = []() 
    {
        std::array<uint32_t, 2048> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = RandVals[i];
        return vals;
    }();
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 7);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 27);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 97);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 197);
    ZExtTest(Intrin.get(), RandVals.data(), ref.data(), 2048);
}
INTRIN_TEST(CopyEx, ZExtCopy24)
{
    const auto ptr = reinterpret_cast<const uint16_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<uint32_t, 1024> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = ptr[i];
        return vals;
    }();
    ZExtTest(Intrin.get(), ptr, ref.data(), 7);
    ZExtTest(Intrin.get(), ptr, ref.data(), 27);
    ZExtTest(Intrin.get(), ptr, ref.data(), 97);
    ZExtTest(Intrin.get(), ptr, ref.data(), 197);
    ZExtTest(Intrin.get(), ptr, ref.data(), 1024);
}

template<typename Src, typename Dst>
static void NarrowTest(const common::CopyManager* intrin, const Src* src, const Dst* ref, size_t count)
{
    std::vector<Dst> dst;
    dst.resize(count);
    intrin->NarrowCopy(dst.data(), src, count);
    EXPECT_THAT(dst, testing::ElementsAreArray(ref, count)) << "when test on [" << count << "] elements";
}
INTRIN_TEST(CopyEx, NarrowCopy21)
{
    const auto ptr = reinterpret_cast<const uint16_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<uint8_t, 1024> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<uint8_t>(ptr[i]);
        return vals;
    }();
    NarrowTest(Intrin.get(), ptr, ref.data(), 7);
    NarrowTest(Intrin.get(), ptr, ref.data(), 27);
    NarrowTest(Intrin.get(), ptr, ref.data(), 97);
    NarrowTest(Intrin.get(), ptr, ref.data(), 197);
    NarrowTest(Intrin.get(), ptr, ref.data(), 1024);
}
INTRIN_TEST(CopyEx, NarrowCopy41)
{
    const auto ptr = reinterpret_cast<const uint32_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<uint8_t, 512> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<uint8_t>(ptr[i]);
        return vals;
    }();
    NarrowTest(Intrin.get(), ptr, ref.data(), 7);
    NarrowTest(Intrin.get(), ptr, ref.data(), 27);
    NarrowTest(Intrin.get(), ptr, ref.data(), 97);
    NarrowTest(Intrin.get(), ptr, ref.data(), 197);
    NarrowTest(Intrin.get(), ptr, ref.data(), 512);
}
INTRIN_TEST(CopyEx, NarrowCopy42)
{
    const auto ptr = reinterpret_cast<const uint32_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<uint16_t, 512> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<uint16_t>(ptr[i]);
        return vals;
    }();
    NarrowTest(Intrin.get(), ptr, ref.data(), 7);
    NarrowTest(Intrin.get(), ptr, ref.data(), 27);
    NarrowTest(Intrin.get(), ptr, ref.data(), 97);
    NarrowTest(Intrin.get(), ptr, ref.data(), 197);
    NarrowTest(Intrin.get(), ptr, ref.data(), 512);
}
template<typename Src, typename Dst>
static void I2FTest(const common::CopyManager* intrin, const Src* src, const Dst* ref, const Dst* ref2, size_t count)
{
    std::vector<Dst> dst;
    dst.resize(count);
    intrin->ToFloatCopy(dst.data(), src, count);
    EXPECT_THAT(dst, testing::ElementsAreArray(ref, count)) << "when test on [" << count << "] elements";
    intrin->ToFloatCopy(dst.data(), src, count, Dst(1));
    EXPECT_THAT(dst, testing::ElementsAreArray(ref2, count)) << "when test on [" << count << "] elements";
}
INTRIN_TEST(CopyEx, CvtI32F32)
{
    const auto ptr = reinterpret_cast<const int32_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<float, 256> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<float>(ptr[i]);
        return vals;
    }();
    static const auto ref2 = [&]()
    {
        constexpr float muler = 1 / float(INT32_MAX);
        std::array<float, 256> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = ref[i] * muler;
        return vals;
    }();
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 7);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 27);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 97);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 197);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 256);
}
INTRIN_TEST(CopyEx, CvtI16F32)
{
    const auto ptr = reinterpret_cast<const int16_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<float, 512> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<float>(ptr[i]);
        return vals;
    }();
    static const auto ref2 = [&]()
    {
        constexpr float muler = 1 / float(INT16_MAX);
        std::array<float, 512> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = ref[i] * muler;
        return vals;
    }();
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 7);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 27);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 97);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 197);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 512);
}
INTRIN_TEST(CopyEx, CvtI8F32)
{
    const auto ptr = reinterpret_cast<const int8_t*>(RandVals.data());
    static const auto ref = [&]()
    {
        std::array<float, 1024> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = static_cast<float>(ptr[i]);
        return vals;
    }();
    static const auto ref2 = [&]()
    {
        constexpr float muler = 1 / float(INT8_MAX);
        std::array<float, 1024> vals = {};
        for (size_t i = 0; i < vals.size(); ++i)
            vals[i] = ref[i] * muler;
        return vals;
    }();
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 7);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 27);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 97);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 197);
    I2FTest(Intrin.get(), ptr, ref.data(), ref2.data(), 1024);
}


INTRIN_TESTSUITE(MiscIntrins, common::MiscIntrins);
TEST_F(MiscIntrins, Complete)
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


INTRIN_TESTSUITE(DigestFuncs, common::DigestFuncs);
TEST_F(DigestFuncs, Complete)
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