#include "rely.h"
#include "ImageUtil/ImageUtilRely.h"
#include "ImageUtil/ColorConvert.h"
#include "SystemCommon/Format.h"
#include <algorithm>
#include <random>


INTRIN_TESTSUITE(ColorCvt, xziar::img::ColorConvertor, xziar::img::ColorConvertor::Get());

template<typename Src, typename Dst, size_t M, size_t N, typename F, typename R, typename E>
static void VarLenTest(const Src* src, size_t total, F&& func, R&& reff, E&& explainer)
{
    std::vector<Dst> ref;
    ref.resize(total * N, static_cast<Dst>(0xcc));
    reff(ref.data(), src, total);
    constexpr size_t sizes[] = { 0,1,2,3,4,7,8,9,15,16,17,30,32,34,61,64,67,252,256,260,509,512,517,1001,1024,1031,2098 };
    for (const auto size : sizes)
    {
        const auto count = std::min(total, size);
        std::vector<Dst> dst;
        dst.resize(count * N, static_cast<Dst>(0xcc));
        func(dst.data(), src, count);
        for (size_t i = 0; i < count; ++i)
        {
            if constexpr (M == 1 && N == 1)
                EXPECT_EQ(dst[i], ref[i]) << explainer(count, i, src[i], dst[i], ref[i]);
            else if constexpr (N > M)
            {
                const common::span<const Dst> dstspan{ dst.data() + i * N, N };
                const common::span<const Dst> refspan{ ref.data() + i * N, N };
                EXPECT_THAT(dstspan, testing::ElementsAreArray(refspan)) << explainer(count, i, src[i], dstspan, refspan);
            }
            else
            {
                const common::span<const Src> srcspan{ src + i * M, M };
                EXPECT_EQ(dst[i], ref[i]) << explainer(count, i, srcspan, dst[i], ref[i]);
            }
        }
        if (total <= size) break;
    }
}


INTRIN_TEST(ColorCvt, G8ToGA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0xa3);
    VarLenTest<uint8_t, uint16_t, 1, 1>(reinterpret_cast<const uint8_t*>(src.data()), src.size(), [&](uint16_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint8_t, uint8_t, 1, 3>(reinterpret_cast<const uint8_t*>(src.data()), src.size(), [&](uint8_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint8_t, uint32_t, 1, 1>(reinterpret_cast<const uint8_t*>(src.data()), src.size(), [&](uint32_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint16_t, uint32_t, 1, 1>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint32_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGB8ToRGBA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0x39);
    VarLenTest<uint8_t, uint32_t, 3, 1>(reinterpret_cast<const uint8_t*>(src.data()), src.size() / 3, [&](uint32_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->RGBToRGBA(dst, src, count, RefAlpha);
    }, [](uint32_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const uint32_t r_ = *src++;
            const uint32_t g_ = *src++;
            const uint32_t b_ = *src++;
            const uint32_t r = r_, g = g_ << 8, b = b_ << 16, a = static_cast<uint32_t>(static_cast<uint8_t>(RefAlpha) << 24);
            *dst++ = r | g | b | a;
            count--;
        }
    },
    [](size_t count, size_t idx, common::span<const uint8_t> src, uint32_t dst, uint32_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x} {:02x} {:02x}] get[{:08x}] ref[{:08x}]"),
            count, idx, src[0], src[1], src[2], dst, ref);
    });
}

INTRIN_TEST(ColorCvt, RGBA8ToRGB8)
{
    const auto src = GetRandVals();
    VarLenTest<uint32_t, uint8_t, 1, 3>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](uint8_t* dst, const uint32_t* src, size_t count)
    {
        Intrin->RGBAToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint32_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(0xff & (val >> 0));
            const auto g = static_cast<uint8_t>(0xff & (val >> 8));
            const auto b = static_cast<uint8_t>(0xff & (val >> 16));
            *dst++ = r;
            *dst++ = g;
            *dst++ = b;
            count--;
        }
    },
    [](size_t count, size_t idx, uint32_t src, common::span<const uint8_t> dst, common::span<const uint8_t> ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:08x}] get[{:02x} {:02x} {:02x}] ref[{:02x} {:02x} {:02x}]"), 
            count, idx, src, dst[0], dst[1], dst[2], ref[0], ref[1], ref[2]);
    });
}


#if CM_DEBUG == 0

TEST(IntrinPerf, G8ToGA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint16_t> dst(Size);
    PerfTester tester("G8ToGA8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.GrayToGrayA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, G8ToRGB8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint8_t> dst(Size * 3);
    PerfTester tester("G8ToRGB8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.GrayToRGB(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, G8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint32_t> dst(Size);
    PerfTester tester("G8ToRGBA8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.GrayToRGBA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, GA8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dst(Size);
    PerfTester tester("GA8ToRGBA8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.GrayAToRGBA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGB8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint32_t> dst(Size);
    PerfTester tester("RGB8ToRGBA8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToRGBA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBA8ToRGB8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dst(Size * 3);
    PerfTester tester("RGBA8ToRGB8", Size);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToRGB(dst.data(), src.data(), Size);
    });
}

#endif