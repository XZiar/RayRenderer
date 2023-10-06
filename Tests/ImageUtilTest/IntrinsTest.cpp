#include "rely.h"
#include "ImageUtil/ImageUtilRely.h"
#include "ImageUtil/ColorConvert.h"
#include "SystemCommon/Format.h"
#include <algorithm>
#include <random>


INTRIN_TESTSUITE(ColorCvt, xziar::img::ColorConvertor, xziar::img::ColorConvertor::Get());

template<typename Src, typename Dst, size_t Multiplier, typename F, typename R, typename E>
static void VarLenTest(common::span<const Src> src, F&& func, R&& reff, E&& explainer)
{
    std::vector<Dst> ref;
    ref.resize(src.size() * Multiplier, static_cast<Dst>(0xcc));
    reff(ref.data(), src.data(), src.size());
    constexpr size_t sizes[] = { 0,1,2,3,4,7,8,9,15,16,17,30,32,34,61,64,67,252,256,260,509,512,517,1001,1024,1031,2098 };
    for (const auto size : sizes)
    {
        const auto count = std::min(src.size(), size);
        const auto tmp = src.subspan(0, count);
        std::vector<Dst> dst;
        dst.resize(count * Multiplier, static_cast<Dst>(0xcc));
        func(dst.data(), tmp.data(), count);
        for (size_t i = 0; i < count; ++i)
        {
            if constexpr (Multiplier == 1)
                EXPECT_EQ(dst[i], ref[i]) << explainer(count, i, tmp[i], dst[i], ref[i]);
            else
            {
                const common::span<const Dst> dstspan{ dst.data() + i * Multiplier, Multiplier };
                const common::span<const Dst> refspan{ ref.data() + i * Multiplier, Multiplier };
                EXPECT_THAT(dstspan, testing::ElementsAreArray(refspan)) << explainer(count, i, tmp[i], dstspan, refspan);
            }
        }
        if (src.size() <= size) break;
    }
}


INTRIN_TEST(ColorCvt, G8ToGA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0xa3);
    VarLenTest<uint8_t, uint16_t, 1>({ reinterpret_cast<const uint8_t*>(src.data()), src.size() }, [&](uint16_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->GrayToGrayA(dst, src, count, RefAlpha);
    }, [](uint16_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            *dst++ = static_cast<uint16_t>((static_cast<uint16_t>(RefAlpha) << 8) | (*src++));
            count--;
        }
    },
    [](size_t count, size_t idx, uint8_t src, uint16_t dst, uint16_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x}] get[{:04x}] ref[{:04x}]"), count, idx, src, dst, ref);
    });
}

INTRIN_TEST(ColorCvt, G8ToRGB8)
{
    const auto src = GetRandVals();
    VarLenTest<uint8_t, uint8_t, 3>({ reinterpret_cast<const uint8_t*>(src.data()), src.size() }, [&](uint8_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->GrayToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src;
            *dst++ = *src;
            *dst++ = *src++;
            count--;
        }
    },
    [](size_t count, size_t idx, uint8_t src, common::span<const uint8_t> dst, common::span<const uint8_t> ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x}] get[{:02x} {:02x} {:02x}] ref[{:02x} {:02x} {:02x}]"), 
            count, idx, src, dst[0], dst[1], dst[2], ref[0], ref[1], ref[2]);
    });
}

INTRIN_TEST(ColorCvt, G8ToRGBA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0xb7);
    VarLenTest<uint8_t, uint32_t, 1>({ reinterpret_cast<const uint8_t*>(src.data()), src.size() }, [&](uint32_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->GrayToRGBA(dst, src, count, RefAlpha);
    }, [](uint32_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const uint32_t r = val, g = val << 8, b = val << 16, a = static_cast<uint32_t>(static_cast<uint8_t>(RefAlpha) << 24);
            *dst++ = r | g | b | a;
            count--;
        }
    },
    [](size_t count, size_t idx, uint8_t src, uint32_t dst, uint32_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x}] get[{:08x}] ref[{:08x}]"), count, idx, src, dst, ref);
    });
}

INTRIN_TEST(ColorCvt, GA8ToRGBA8)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint32_t, 1>({ reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2 }, [&](uint32_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayAToRGBA(dst, src, count);
    }, [](uint32_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const uint32_t gray = val & 0xffu, alpha = val >> 8;
            const uint32_t r = gray, g = gray << 8, b = gray << 16, a = alpha << 24;
            *dst++ = r | g | b | a;
            count--;
        }
    },
    [](size_t count, size_t idx, uint16_t src, uint32_t dst, uint32_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:04x}] get[{:08x}] ref[{:08x}]"), count, idx, src, dst, ref);
    });
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
static void RunPerfTestAll(std::string_view funcName, F&& func, size_t opPerRun, uint32_t limitUs = 300000/*0.3s*/)
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

TEST(IntrinPerf, G8ToGA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint16_t> dst(Size);
    RunPerfTestAll<xziar::img::ColorConvertor>("G8ToGA8", [&](const xziar::img::ColorConvertor& host)
    {
        host.GrayToGrayA(dst.data(), src.data(), src.size());
    }, Size);
}

TEST(IntrinPerf, G8ToRGB8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint8_t> dst(Size * 3);
    RunPerfTestAll<xziar::img::ColorConvertor>("G8ToRGB8", [&](const xziar::img::ColorConvertor& host)
        {
            host.GrayToRGB(dst.data(), src.data(), src.size());
        }, Size);
}

TEST(IntrinPerf, G8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint32_t> dst(Size);
    RunPerfTestAll<xziar::img::ColorConvertor>("G8ToRGBA8", [&](const xziar::img::ColorConvertor& host)
        {
            host.GrayToRGBA(dst.data(), src.data(), src.size());
        }, Size);
}

TEST(IntrinPerf, GA8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dst(Size);
    RunPerfTestAll<xziar::img::ColorConvertor>("GA8ToRGBA8", [&](const xziar::img::ColorConvertor& host)
    {
        host.GrayAToRGBA(dst.data(), src.data(), src.size());
    }, Size);
}

#endif