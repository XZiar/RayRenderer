#include "rely.h"
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/MiscIntrins.h"
#include "3rdParty/half/half.hpp"
#include <algorithm>
#include <random>


using namespace std::string_view_literals;

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

INTRIN_TESTSUITE(CopyEx, common::CopyManager, common::CopyEx);


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


template<typename Src, typename Dst>
void DirectCastTest(const common::CopyManager& intrin, void(common::CopyManager::* func)(Dst*, const Src*, size_t) const noexcept)
{
    const auto ptr = reinterpret_cast<const Src*>(RandVals.data());
    static const auto ref = CastRef<Dst>(ptr);
    CastTest(ptr, ref, [&](auto dst, auto src, auto cnt)
        {
            (intrin.*func)(dst, src, cnt);
        });
}
#define DCAST_TEST(func, type, tin, tout) INTRIN_TEST(CopyEx, func##type)   \
{                                                                           \
    DirectCastTest<tin, tout>(*Intrin, &common::CopyManager::func);         \
}
DCAST_TEST(ZExtCopy,  12, uint8_t,  uint16_t)
DCAST_TEST(ZExtCopy,  14, uint8_t,  uint32_t)
DCAST_TEST(ZExtCopy,  24, uint16_t, uint32_t)
DCAST_TEST(ZExtCopy,  28, uint16_t, uint64_t)
DCAST_TEST(ZExtCopy,  48, uint32_t, uint64_t)
DCAST_TEST(SExtCopy,  12,  int8_t,   int16_t)
DCAST_TEST(SExtCopy,  14,  int8_t,   int32_t)
DCAST_TEST(SExtCopy,  24,  int16_t,  int32_t)
DCAST_TEST(SExtCopy,  28,  int16_t,  int64_t)
DCAST_TEST(SExtCopy,  48,  int32_t,  int64_t)
DCAST_TEST(TruncCopy, 21, uint16_t, uint8_t )
DCAST_TEST(TruncCopy, 41, uint32_t, uint8_t )
DCAST_TEST(TruncCopy, 42, uint32_t, uint16_t)
DCAST_TEST(TruncCopy, 81, uint64_t, uint8_t )
DCAST_TEST(TruncCopy, 82, uint64_t, uint16_t)
DCAST_TEST(TruncCopy, 84, uint64_t, uint32_t)


template<typename Src, typename Dst>
void I2FTest(const common::CopyManager& intrin)
{
    const auto ptr = reinterpret_cast<const Src*>(RandVals.data());
    static const auto ref1 = CastRef<Dst>(ptr);
    static const auto ref2 = CastRef<Dst>(ptr, [](auto src) 
        { 
            return static_cast<Dst>(src) * (1 / static_cast<Dst>(std::numeric_limits<Src>::max()));
        });
    CastTest(ptr, ref1, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyToFloat(dst, src, cnt);
        });
    CastTest(ptr, ref2, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyToFloat(dst, src, cnt, Dst(1));
        });
}
#define I2F_TEST(type, tin, tout) INTRIN_TEST(CopyEx, type) { I2FTest<tin, tout>(*Intrin); }
I2F_TEST(CvtI32F32,  int32_t, float)
I2F_TEST(CvtI16F32,  int16_t, float)
I2F_TEST(CvtI8F32,   int8_t,  float)
I2F_TEST(CvtU32F32, uint32_t, float)
I2F_TEST(CvtU16F32, uint16_t, float)
I2F_TEST(CvtU8F32,  uint8_t,  float)


template<typename Src, typename Dst>
static void F2ITest(const common::CopyManager& intrin)
{
    static constexpr auto IMax = std::numeric_limits<Dst>::max();
    static constexpr auto IMin = std::numeric_limits<Dst>::min();
    static constexpr auto FMax = static_cast<Src>(IMax);
    static constexpr auto FMin = static_cast<Src>(IMin);
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
#define F2I_TEST(type, tin, tout) INTRIN_TEST(CopyEx, type) { F2ITest<tin, tout>(*Intrin); }
F2I_TEST(CvtF32I32, float,  int32_t)
F2I_TEST(CvtF32I16, float,  int16_t)
F2I_TEST(CvtF32I8,  float,  int8_t )
F2I_TEST(CvtF32U16, float, uint16_t)
F2I_TEST(CvtF32U8,  float, uint8_t )


template<typename Src, typename Dst, typename TIn, typename TOut, typename Val>
void F2FTest(const common::CopyManager& intrin)
{
    const auto ptr = reinterpret_cast<const Val*>(RandVals.data());
    static const auto src1 = CastRef<Src>(ptr);

    static const auto src2 = CastRef<Src>(ptr, [](auto src) 
        { 
            using T = std::conditional_t<(sizeof(Src) > 4), double, float>;
            return static_cast<T>(src * 2.0 / std::numeric_limits<Val>::max()); 
        }); // fraction
    static const auto src3 = CastRef<Src>(ptr, [](auto src) 
        {
            constexpr auto MantissaBits = std::numeric_limits<Src>::digits - 1;
            constexpr auto MantissaMask = static_cast<Val>(UINT64_MAX >> (64 - MantissaBits));
            src &= MantissaMask;
            uint8_t dummy[sizeof(Src)] = { 0 };
            memcpy_s(dummy, sizeof(Src), &src, sizeof(src));
            Src ret{};
            memcpy_s(&ret, sizeof(Src), dummy, sizeof(Src));
            return ret; 
        }); // denorm
    static const auto ref1 = CastRef<Dst>(src1.data());
    static const auto ref2 = CastRef<Dst>(src2.data());
    static const auto ref3 = CastRef<Dst>(src3.data());
    CastTest(src1.data(), ref1, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFloat(reinterpret_cast<TOut*>(dst), reinterpret_cast<const TIn*>(src), cnt);
        });
    CastTest(src2.data(), ref2, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFloat(reinterpret_cast<TOut*>(dst), reinterpret_cast<const TIn*>(src), cnt);
        });
    CastTest(src3.data(), ref3, [&](auto dst, auto src, auto cnt)
        {
            intrin.CopyFloat(reinterpret_cast<TOut*>(dst), reinterpret_cast<const TIn*>(src), cnt);
        });
}
#define F2F_TEST(func, tsrc, tdst, tin, tout, tval) \
INTRIN_TEST(CopyEx, func)                           \
{                                                   \
    F2FTest<tsrc, tdst, tin, tout, tval>(*Intrin);  \
}
using half = half_float::half;
F2F_TEST(CvtF16F32, half,   float,  uint16_t, float,    uint16_t)
F2F_TEST(CvtF32F16, float,  half,   float,    uint16_t, uint16_t)
F2F_TEST(CvtF32F64, float,  double, float,    double,   uint32_t)
F2F_TEST(CvtF64F32, double, float,  double,   float,    uint32_t)


INTRIN_TESTSUITE(MiscIntrins, common::MiscIntrins, common::MiscIntrin);


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

INTRIN_TEST(MiscIntrins, Hex2Str)
{
    constexpr auto data = []() 
    {
        std::array<std::byte, 267> data = {};
        uint8_t i = 0;
        for (auto& dat : data)
            dat = std::byte(i++);
        return data;
    }();
    EXPECT_EQ(Intrin->HexToStr(data), Hex2Str(data));
}


INTRIN_TESTSUITE(DigestFuncs, common::DigestFuncs, common::DigestFunc);


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
        // 1000000
        std::string txt(1000000, 'a');
        EXPECT_EQ(SHA256(txt),
            "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    }
}


#if CM_DEBUG == 0

template<typename T>
static std::vector<std::unique_ptr<T>> GenerateIntrinHost(std::string_view funcName)
{
    std::vector<std::unique_ptr<T>> tests;
    for (const auto& path : T::GetSupportMap())
    {
        if (path.FuncName == funcName)
        {
            for (const auto& var : path.Variants)
            {
                std::pair<std::string_view, std::string_view> info{ path.FuncName, var.MethodName };
                tests.emplace_back(std::make_unique<T>(common::span<decltype(info)>{ &info, 1 }));
            }
        }
    }
    return tests;
}

template<typename T, typename F>
static void RunPerfTestAll(std::string_view funcName, F&& func, size_t opPerRun, uint32_t limitUs = 400000/*0.4s*/)
{
    const auto tests = GenerateIntrinHost<T>(funcName);
    for (const auto& test : tests)
    {
        const auto nsPerRun = RunPerfTest([&]() { func(*test); }, limitUs);
        const auto nsPerOp = static_cast<double>(nsPerRun) / opPerRun;
        const auto intrinMap = test->GetIntrinMap();
        Ensures(intrinMap.size() == 1);
        Ensures(intrinMap[0].first == funcName);
        TestCout() << "[" << funcName << "]: [" << intrinMap[0].second << "] takes avg[" << nsPerOp << "]ns per operation\n";
    }
}

TEST(IntrinPerf, F2F)
{
    std::vector<half_float::half> inputs;
    inputs.reserve(512 * 512 * 4);
    // 512*512*4 = 1M test cases
    for (uint32_t i = 0; i < 512; ++i)
    {
        const auto srca = RandVals[i];
        for (uint32_t j = 0; j < 512; ++j)
        {
            const auto srcb = RandVals[j];
            const auto base = static_cast<float>(srca * srcb);
            inputs.push_back(static_cast<half_float::half>(base));
            const auto frac = base / (UINT16_MAX / 2);
            inputs.push_back(static_cast<half_float::half>(frac));
            const auto small1 = frac / UINT8_MAX;
            inputs.push_back(static_cast<half_float::half>(small1));
            const auto small2 = small1 / UINT8_MAX;
            inputs.push_back(static_cast<half_float::half>(small2));
        }
    }
    std::vector<float> outputs;
    outputs.resize(inputs.size());
    RunPerfTestAll<common::CopyManager>("CvtF16F32", [&](const common::CopyManager& host)
        {
            host.CopyFloat(outputs.data(), reinterpret_cast<const uint16_t*>(inputs.data()), inputs.size());
        }, static_cast<uint32_t>(inputs.size()));
}


TEST(IntrinPerf, Hex2Str)
{
    RunPerfTestAll<common::MiscIntrins>("Hex2Str", [&](const common::MiscIntrins& host)
        {
            [[maybe_unused]] const auto ret = host.HexToStr(RandVals.data(), sizeof(RandVals));
        }, static_cast<uint32_t>(sizeof(RandVals)));
}


TEST(IntrinPerf, Sha256)
{
    std::vector<std::byte> inputs(1024 * 1024);
    RunPerfTestAll<common::DigestFuncs>("Sha256", [&](const common::DigestFuncs& host)
        {
            host.SHA256(common::to_span(inputs));
        }, static_cast<uint32_t>(inputs.size()));
}

#endif