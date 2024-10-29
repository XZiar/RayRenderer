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
            else if constexpr (M > N)
            {
                const common::span<const Src> srcspan{ src + i * M, M };
                EXPECT_EQ(dst[i], ref[i]) << explainer(count, i, srcspan, dst[i], ref[i]);
            }
            else
            {
                const common::span<const Src> srcspan{ src + i * M, M };
                const common::span<const Dst> dstspan{ dst.data() + i * N, N };
                const common::span<const Dst> refspan{ ref.data() + i * N, N };
                EXPECT_THAT(dstspan, testing::ElementsAreArray(refspan)) << explainer(count, i, srcspan, dstspan, refspan);
            }
        }
        if (total <= size) break;
    }
}


INTRIN_TEST(ColorCvt, G8ToGA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0xa3);
    SCOPED_TRACE("ColorCvt::G8ToGA8");
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
    SCOPED_TRACE("ColorCvt::G8ToRGB8");
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
    SCOPED_TRACE("ColorCvt::G8ToRGBA8");
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
    SCOPED_TRACE("ColorCvt::GA8ToRGBA8");
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
    SCOPED_TRACE("ColorCvt::RGB8ToRGBA8");
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

INTRIN_TEST(ColorCvt, BGR8ToRGBA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0x5b);
    SCOPED_TRACE("ColorCvt::BGR8ToRGBA8");
    VarLenTest<uint8_t, uint32_t, 3, 1>(reinterpret_cast<const uint8_t*>(src.data()), src.size() / 3, [&](uint32_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->BGRToRGBA(dst, src, count, RefAlpha);
    }, [](uint32_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const uint32_t b_ = *src++;
            const uint32_t g_ = *src++;
            const uint32_t r_ = *src++;
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
    SCOPED_TRACE("ColorCvt::RGBA8ToRGB8");
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

INTRIN_TEST(ColorCvt, RGBA8ToBGR8)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBA8ToBGR8");
    VarLenTest<uint32_t, uint8_t, 1, 3>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](uint8_t* dst, const uint32_t* src, size_t count)
    {
        Intrin->RGBAToBGR(dst, src, count);
    }, [](uint8_t* dst, const uint32_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto b = static_cast<uint8_t>(0xff & (val >> 0));
            const auto g = static_cast<uint8_t>(0xff & (val >> 8));
            const auto r = static_cast<uint8_t>(0xff & (val >> 16));
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

INTRIN_TEST(ColorCvt, RGB8ToBGR8)
{
    std::vector<uint8_t> tmp;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        tmp.push_back(static_cast<uint8_t>((i << 4) | 0));
        tmp.push_back(static_cast<uint8_t>((i << 4) | 1));
        tmp.push_back(static_cast<uint8_t>((i << 4) | 2));
    }
    //const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGB8ToBGR8");
    VarLenTest<uint8_t, uint8_t, 3, 3>(reinterpret_cast<const uint8_t*>(tmp.data()), tmp.size() / 3, [&](uint8_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->RGBToBGR(dst, src, count);
    }, [](uint8_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const auto r = *src++;
            const auto g = *src++;
            const auto b = *src++;
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
            count--;
        }
    },
    [](size_t count, size_t idx, common::span<const uint8_t> src, common::span<const uint8_t> dst, common::span<const uint8_t> ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x} {:02x} {:02x}] get[{:02x} {:02x} {:02x}] ref[{:02x} {:02x} {:02x}]"),
            count, idx, src[0], src[1], src[2], dst[0], dst[1], dst[2], ref[0], ref[1], ref[2]);
    });
}

INTRIN_TEST(ColorCvt, RGBA8ToBGRA8)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBA8ToBGRA8");
    VarLenTest<uint32_t, uint32_t, 1, 1>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](uint32_t* dst, const uint32_t* src, size_t count)
    {
        Intrin->RGBAToBGRA(dst, src, count);
    }, [](uint32_t* dst_, const uint32_t* src_, size_t count)
    {
        auto dst = reinterpret_cast<uint8_t*>(dst_);
        auto src = reinterpret_cast<const uint8_t*>(src_);
        while (count)
        {
            const auto r = *src++;
            const auto g = *src++;
            const auto b = *src++;
            const auto a = *src++;
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
            *dst++ = a;
            count--;
        }
    },
    [](size_t count, size_t idx, uint32_t src, uint32_t dst, uint32_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:08x}] get[{:08x}] ref[{:08x}]"), 
            count, idx, src, dst, ref);
    });
}

template<uint8_t Ch>
void TestGetChannel4(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint32_t, uint8_t, 1, 1>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](uint8_t* dst, const uint32_t* src, size_t count)
    {
        intrin->RGBAGetChannel(dst, src, count, Ch);
    }, [](uint8_t* dst, const uint32_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(0xff & (val >> 0));
            const auto g = static_cast<uint8_t>(0xff & (val >> 8));
            const auto b = static_cast<uint8_t>(0xff & (val >> 16));
            const auto a = static_cast<uint8_t>(0xff & (val >> 24));
            const uint8_t rgba[4] = { r,g,b,a };
            *dst++ = rgba[Ch];
            count--;
        }
    },
    [](size_t count, size_t idx, uint32_t src, uint8_t dst, uint8_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:08x}] get[{:02x}] ref[{:02x}]"),
            count, idx, src, dst, ref);
    });
}
INTRIN_TEST(ColorCvt, RGBA8ToR8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToR8");
    TestGetChannel4<0>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBA8ToG8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToG8");
    TestGetChannel4<1>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBA8ToB8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToB8");
    TestGetChannel4<2>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBA8ToA8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToA8");
    TestGetChannel4<3>(Intrin);
}

template<uint8_t Ch>
void TestGetChannel3(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint8_t, uint8_t, 3, 1>(reinterpret_cast<const uint8_t*>(src.data()), src.size() / 3, [&](uint8_t* dst, const uint8_t* src, size_t count)
    {
        intrin->RGBGetChannel(dst, src, count, Ch);
    }, [](uint8_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const auto r = *src++;
            const auto g = *src++;
            const auto b = *src++;
            const uint8_t rgb[3] = { r,g,b };
            *dst++ = rgb[Ch];
            count--;
        }
    },
    [](size_t count, size_t idx, common::span<const uint8_t> src, uint8_t dst, uint8_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:02x} {:02x} {:02x}] get[{:02x}] ref[{:02x}]"),
            count, idx, src[0], src[1], src[2], dst, ref);
    });
}
INTRIN_TEST(ColorCvt, RGB8ToR8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToR8");
    TestGetChannel3<0>(Intrin);
}

INTRIN_TEST(ColorCvt, RGB8ToG8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToG8");
    TestGetChannel3<1>(Intrin);
}

INTRIN_TEST(ColorCvt, RGB8ToB8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToB8");
    TestGetChannel3<2>(Intrin);
}

template<uint8_t Ch>
void TestGetChannelA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint32_t, uint16_t, 1, 1>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](uint16_t* dst, const uint32_t* src, size_t count)
    {
        intrin->RGBAGetChannelAlpha(dst, src, count, Ch);
    }, [](uint16_t* dst, const uint32_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(0xff & (val >> 0));
            const auto g = static_cast<uint8_t>(0xff & (val >> 8));
            const auto b = static_cast<uint8_t>(0xff & (val >> 16));
            const auto a = static_cast<uint8_t>(0xff & (val >> 24));
            const uint8_t rgb[3] = { r,g,b };
            reinterpret_cast<uint8_t*>(dst)[0] = rgb[Ch];
            reinterpret_cast<uint8_t*>(dst)[1] = a;
            dst++;
            count--;
        }
    },
    [](size_t count, size_t idx, uint32_t src, uint16_t dst, uint16_t ref)
    {
        common::str::Formatter<char> fmter{};
        return fmter.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{:08x}] get[{:04x}] ref[{:04x}]"),
            count, idx, src, dst, ref);
    });
}
INTRIN_TEST(ColorCvt, RGBA8ToRA8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToRA8");
    TestGetChannelA<0>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBA8ToGA8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToGA8");
    TestGetChannelA<1>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBA8ToBA8)
{
    SCOPED_TRACE("ColorCvt::RGBA8ToBA8");
    TestGetChannelA<2>(Intrin);
}


#if CM_DEBUG == 0

TEST(IntrinPerf, G8ToGA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint16_t> dst(Size);
    PerfTester tester("G8ToGA8", Size, 150);
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
    PerfTester tester("G8ToRGB8", Size, 150);
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
    PerfTester tester("G8ToRGBA8", Size, 150);
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
    PerfTester tester("GA8ToRGBA8", Size, 150);
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
    PerfTester tester1("RGB8ToRGBA8", Size, 150);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToRGBA(dst.data(), src.data(), Size);
    });
    PerfTester tester2("BGR8ToRGBA8", Size, 150);
    tester2.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.BGRToRGBA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBA8ToRGB8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dst(Size * 3);
    PerfTester tester1("RGBA8ToRGB8", Size, 150);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToRGB(dst.data(), src.data(), Size);
    });
    PerfTester tester2("RGBA8ToBGR8", Size, 150);
    tester2.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToBGR(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGB8ToBGR8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint8_t> dst(Size * 3);
    PerfTester tester("RGB8ToBGR8", Size, 150);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToBGR(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBA8ToBGRA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint32_t> dst(Size);
    PerfTester tester("RGBA8ToBGRA8", Size, 150);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToBGRA(dst.data(), src.data(), Size);
    });
}

template<uint8_t Idx>
void TestGetChannel4Perf(std::string_view name)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dst(Size);
    PerfTester tester(name, Size, 150);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAGetChannel(dst.data(), src.data(), Size, Idx);
    });
}
TEST(IntrinPerf, RGBAGetChannel)
{
    TestGetChannel4Perf<0>("RGBA8ToR8");
    TestGetChannel4Perf<1>("RGBA8ToG8");
    TestGetChannel4Perf<2>("RGBA8ToB8");
    TestGetChannel4Perf<3>("RGBA8ToA8");
}

template<uint8_t Idx>
void TestGetChannel3Perf(std::string_view name)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint8_t> dst(Size);
    PerfTester tester(name, Size, 150);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBGetChannel(dst.data(), src.data(), Size, Idx);
    });
}
TEST(IntrinPerf, RGB8ToR8)
{
    TestGetChannel3Perf<0>("RGB8ToR8");
    TestGetChannel3Perf<1>("RGB8ToG8");
    TestGetChannel3Perf<2>("RGB8ToB8");
}

template<uint8_t Idx>
void TestGetChannel4APerf(std::string_view name)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint16_t> dst(Size);
    PerfTester tester(name, Size, 150);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAGetChannelAlpha(dst.data(), src.data(), Size, Idx);
    });
}
TEST(IntrinPerf, RGBAGetChannelAlpha)
{
    TestGetChannel4APerf<0>("RGBA8ToRA8");
    TestGetChannel4APerf<1>("RGBA8ToGA8");
    TestGetChannel4APerf<2>("RGBA8ToBA8");
}

#endif