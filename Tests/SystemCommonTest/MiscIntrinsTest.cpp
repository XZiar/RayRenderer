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

template<typename Src, typename Dst, size_t Max, typename F>
static void CastTest(const Src* src, const std::array<Dst, Max>& ref, F&& func)
{
    constexpr size_t sizes[] = { 0,1,2,3,4,7,8,9,15,16,17,30,32,34,61,64,67,252,256,260,509,512,517,1001,1024,1031,2098 };
    for (const auto size : sizes)
    {
        const auto count = std::min(Max, size);
        std::vector<Dst> dst;
        dst.resize(count);
        func(dst.data(), src, count);
        EXPECT_THAT(dst, testing::ElementsAreArray(ref.data(), count)) << "when test on [" << count << "] elements";
        if (size >= Max) break;
    }
}
template<typename Dst, typename Src>
static auto CastRef(const Src* src)
{
    std::array<Dst, std::tuple_size_v<decltype(RandVals)> / sizeof(Src)> vals = {};
    for (size_t i = 0; i < vals.size(); ++i)
        vals[i] = static_cast<Dst>(src[i]);
    return vals;
}
template<typename Dst, size_t N, typename Src>
static auto CastRef(const Src* src)
{
    std::array<Dst, N> vals = {};
    for (size_t i = 0; i < vals.size(); ++i)
        vals[i] = static_cast<Dst>(src[i]);
    return vals;
}
template<typename Dst, typename Src, typename F>
static auto CastRef(const Src* src, F&& func)
{
    std::array<Dst, std::tuple_size_v<decltype(RandVals)> / sizeof(Src)> vals = {};
    for (size_t i = 0; i < vals.size(); ++i)
        vals[i] = func(src[i]);
    return vals;
}
INTRIN_TEST(CopyEx, ZExtCopy12)
{
    static const auto ref = CastRef<uint16_t>(RandVals.data());
    CastTest(RandVals.data(), ref, [&](auto dst, auto src, auto cnt) 
        {
            Intrin->ZExtCopy(dst, src, cnt);
        });
}
INTRIN_TEST(CopyEx, ZExtCopy14)
{
    static const auto ref = CastRef<uint32_t>(RandVals.data());
    CastTest(RandVals.data(), ref, [&](auto dst, auto src, auto cnt)
        {
            Intrin->ZExtCopy(dst, src, cnt);
        });
}
INTRIN_TEST(CopyEx, ZExtCopy24)
{
    const auto ptr = reinterpret_cast<const uint16_t*>(RandVals.data());
    static const auto ref = CastRef<uint32_t>(ptr);
    CastTest(ptr, ref, [&](auto dst, auto src, auto cnt)
        {
            Intrin->ZExtCopy(dst, src, cnt);
        });
}

INTRIN_TEST(CopyEx, TruncCopy21)
{
    const auto ptr = reinterpret_cast<const uint16_t*>(RandVals.data());
    static const auto ref = CastRef<uint8_t>(ptr);
    CastTest(ptr, ref, [&](auto dst, auto src, auto cnt)
        {
            Intrin->TruncCopy(dst, src, cnt);
        });
}
INTRIN_TEST(CopyEx, TruncCopy41)
{
    const auto ptr = reinterpret_cast<const uint32_t*>(RandVals.data());
    static const auto ref = CastRef<uint8_t>(ptr);
    CastTest(ptr, ref, [&](auto dst, auto src, auto cnt)
        {
            Intrin->TruncCopy(dst, src, cnt);
        });
}
INTRIN_TEST(CopyEx, TruncCopy42)
{
    const auto ptr = reinterpret_cast<const uint32_t*>(RandVals.data());
    static const auto ref = CastRef<uint16_t>(ptr);
    CastTest(ptr, ref, [&](auto dst, auto src, auto cnt)
        {
            Intrin->TruncCopy(dst, src, cnt);
        });
}

INTRIN_TEST(CopyEx, CvtI32F32)
{
    const auto ptr = reinterpret_cast<const int32_t*>(RandVals.data());
    static const auto ref1 = CastRef<float>(ptr);
    static const auto ref2 = CastRef<float>(ptr, [](auto src) { return static_cast<float>(src) * (1 / float(INT32_MAX)); });
    CastTest(ptr, ref1, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt);
        });
    CastTest(ptr, ref2, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt, 1.f);
        });
}
INTRIN_TEST(CopyEx, CvtI16F32)
{
    const auto ptr = reinterpret_cast<const int16_t*>(RandVals.data());
    static const auto ref1 = CastRef<float>(ptr);
    static const auto ref2 = CastRef<float>(ptr, [](auto src) { return static_cast<float>(src) * (1 / float(INT16_MAX)); });
    CastTest(ptr, ref1, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt);
        });
    CastTest(ptr, ref2, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt, 1.f);
        });
}
INTRIN_TEST(CopyEx, CvtI8F32)
{
    const auto ptr = reinterpret_cast<const int8_t*>(RandVals.data());
    static const auto ref1 = CastRef<float>(ptr);
    static const auto ref2 = CastRef<float>(ptr, [](auto src) { return static_cast<float>(src) * (1 / float(INT8_MAX)); });
    CastTest(ptr, ref1, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt);
        });
    CastTest(ptr, ref2, [&](auto dst, auto src, auto cnt)
        {
            Intrin->CopyToFloat(dst, src, cnt, 1.f);
        });
}

template<typename Src, typename Dst>
static void TestCvtFP2I(const common::CopyManager& intrin)
{
    static constexpr auto IMax = std::numeric_limits<Dst>::max();
    static constexpr auto IMin = std::numeric_limits<Dst>::min();
    static constexpr Src FMax = static_cast<Src>(std::numeric_limits<Dst>::max());
    static constexpr Src FMin = static_cast<Src>(std::numeric_limits<Dst>::min());
    const auto ptr = reinterpret_cast<const Dst*>(RandVals.data());
    static const auto fp1 = CastRef<Src>(ptr);
    static const auto fp2 = CastRef<Src>(ptr, [](auto src) { return static_cast<Src>(src) * (1 / FMax); });
    static const auto ref1 = CastRef<Dst>(fp1.data());
    static const auto ref2 = CastRef<Dst>(fp2.data(), [](auto src) { return static_cast<Dst>(src * FMax); });
    static const auto ref3 = CastRef<Dst>(fp2.data(), [](auto src)
        { 
            src *= 2 * FMax;
            if (src >= FMax) return IMax;
            if (src <= FMin) return IMin;
            return static_cast<Dst>(src);
        });
    CastTest(fp1.data(), ref1, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFromFloat(dst, src, cnt);
        });
    CastTest(fp2.data(), ref2, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFromFloat(dst, src, cnt, Src(1));
        });
    CastTest(fp2.data(), ref3, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFromFloat(dst, src, cnt, Src(0.5), true);
        });
}
INTRIN_TEST(CopyEx, CvtF32I32)
{
    TestCvtFP2I<float, int32_t>(*Intrin);
}
INTRIN_TEST(CopyEx, CvtF32I16)
{
    TestCvtFP2I<float, int16_t>(*Intrin);
}
INTRIN_TEST(CopyEx, CvtF32I8)
{
    TestCvtFP2I<float, int8_t>(*Intrin);
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