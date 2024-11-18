#include "rely.h"
#include "ImageUtil/ImageUtilRely.h"
#include "ImageUtil/ColorConvert.h"
#include "SystemCommon/Format.h"
#include <algorithm>
#include <random>


INTRIN_TESTSUITE(ColorCvt, xziar::img::ColorConvertor, xziar::img::ColorConvertor::Get());

constexpr size_t TestSizes[] = { 0,1,2,3,4,7,8,9,15,16,17,30,32,34,61,64,67,201,252,256,260,509,512,517,1001,1024,1031,2098 };

template<typename T>
struct HexVal
{
    common::span<const T> Vals;
    static constexpr common::str::FormatSpec SpecIntHex = []() 
    {
        constexpr auto type = common::str::FormatterParser::FormatSpec::TypeIdentifier{ 'x' };
        common::str::FormatSpec spec;
        spec.ZeroPad = true;
        spec.Width = sizeof(T) * 2;
        spec.TypeExtra = type.Extra;
        return spec;
    }();
    constexpr HexVal() noexcept {}
    constexpr HexVal(common::span<const T> vals) noexcept : Vals{ vals } {}

    constexpr bool operator==(const HexVal<T>& other) const noexcept 
    {
        return (Vals.size() == other.Vals.size()) && std::equal(Vals.begin(), Vals.end(), other.Vals.begin());
    }

    void FormatWith(common::str::FormatterExecutor& executor, common::str::FormatterExecutor::Context& ctx, const common::str::FormatSpec*) const noexcept
    {
        for (uint32_t i = 0; i < Vals.size(); ++i)
        {
            if (i > 0)
                executor.PutString(ctx, " ", nullptr);
            using U = std::conditional_t<(sizeof(T) > sizeof(uint32_t)), uint64_t, uint32_t>;
            if constexpr (std::is_floating_point_v<T>)
            {
                executor.PutFloat(ctx, Vals[i], nullptr);
                executor.PutString(ctx, "(", nullptr);
                U intVal;
                memcpy_s(&intVal, sizeof(U), &Vals[i], sizeof(T));
                executor.PutInteger(ctx, intVal, false, &SpecIntHex);
                executor.PutString(ctx, ")", nullptr);
            }
            else
            {
                executor.PutInteger(ctx, static_cast<U>(Vals[i]), false, &SpecIntHex);
            }
        }
    }

    friend void PrintTo(const HexVal<T>& val, std::ostream* os)
    {
        const auto str = common::str::Formatter<char>{}.FormatStatic(FmtString("[{}]"), val);
        *os << str;
    }
};

template<typename S, typename D>
struct HexTest
{
    HexVal<S> Src;
    HexVal<D> Dst;
    HexVal<D> Ref;
    size_t Count;
    size_t Index;
    constexpr HexTest(size_t count, size_t idx) noexcept : Count(count), Index(idx) {}
    constexpr void SetSrc(common::span<const S> src) noexcept { Src = src; }
    constexpr void SetSrc(const S& src) noexcept { Src.Vals = { &src, 1 }; }
    constexpr void SetOut(common::span<const D> dst, common::span<const D> ref) noexcept { Dst = dst; Ref = ref; }
    constexpr void SetOut(const D& dst, const D& ref) noexcept { Dst.Vals = { &dst, 1 }; Ref.Vals = { &ref, 1 }; }

    friend std::ostream& operator<<(std::ostream& stream, const HexTest<S, D>& test) noexcept
    {
        const auto str = common::str::Formatter<char>{}.FormatStatic(FmtString("when test on [{}] elements, idx[{}] src[{}]"), test.Count, test.Index, test.Src);
        stream << str;
        return stream;
    }
};


template<typename Src, typename Dst, size_t M, size_t N, typename F, typename R>
static void VarLenTest(const Src* src, size_t total, F&& func, R&& reff)
{
    constexpr bool TryInplace = std::is_same_v<Src, Dst> && M == N;
    std::vector<Dst> ref;
    ref.resize(total * N, static_cast<Dst>(0xcc));
    reff(ref.data(), src, total);
    for (const auto size : TestSizes)
    {
        const auto count = std::min(total, size);
        std::vector<Dst> dsts[TryInplace ? 2 : 1];
        dsts[0].resize(count * N, static_cast<Dst>(0xcc));
        func(dsts[0].data(), src, count);
        if constexpr (TryInplace)
        {
            dsts[1].assign(src, src + count * N);
            func(dsts[1].data(), dsts[1].data(), count);
        }
        bool isSuc = true;
        [[maybe_unused]] std::string_view cond = "";
        for (const auto& dst : dsts)
        {
            for (size_t i = 0; i < count; ++i)
            {
                HexTest<Src, Dst> test(count, i);
                if constexpr (M == 1)
                    test.SetSrc(src[i]);
                else
                    test.SetSrc(common::span<const Src>{ src + i * M, M });
                if constexpr (N == 1)
                    test.SetOut(dst[i], ref[i]);
                else
                    test.SetOut(common::span<const Dst>{ dst.data() + i * N, N }, common::span<const Dst>{ ref.data() + i * N, N });

                EXPECT_EQ(test.Dst, test.Ref) << test;
                isSuc = isSuc && (test.Dst == test.Ref);
            }
            cond = "[inplace]"; // only used when > 1
        }
        if (total <= size || !isSuc) break;
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
    });
}

INTRIN_TEST(ColorCvt, GA8ToRGB8)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GA8ToRGB8");
    VarLenTest<uint16_t, uint8_t, 1, 3>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayAToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const uint8_t gray = static_cast<uint8_t>(val);
            *dst++ = gray;
            *dst++ = gray;
            *dst++ = gray;
            count--;
        }
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
    });
}

INTRIN_TEST(ColorCvt, RGB8ToBGR8)
{
    //std::vector<uint8_t> src;
    //for (uint32_t i = 0; i < 4096; ++i)
    //{
    //    src.push_back(static_cast<uint8_t>((i << 4) | 0));
    //    src.push_back(static_cast<uint8_t>((i << 4) | 1));
    //    src.push_back(static_cast<uint8_t>((i << 4) | 2));
    //}
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGB8ToBGR8");
    VarLenTest<uint8_t, uint8_t, 3, 3>(reinterpret_cast<const uint8_t*>(src.data()), src.size() / 3, [&](uint8_t* dst, const uint8_t* src, size_t count)
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


template<typename Src, typename Dst, size_t M, size_t N, typename F>
static void VarLenExtractTest(common::span<const std::byte> source, F&& func)
{
    static_assert(sizeof(Src) * M == sizeof(Dst) * N);
    const auto src = reinterpret_cast<const Src*>(source.data());
    const auto total = source.size() / sizeof(Src) / M;
    for (const auto size : TestSizes)
    {
        const auto count = std::min(total, size);
        std::vector<Dst> dsts[N];
        std::array<Dst*, N> ptrs = {};
        for (uint32_t i = 0; i < N; ++i)
        {
            dsts[i].resize(count * N, static_cast<Dst>(0xcc));
            ptrs[i] = dsts[i].data();
        }
        func(ptrs, src, count);
        bool isSuc = true;
        auto ref = reinterpret_cast<const Dst*>(source.data());
        for (size_t i = 0; i < count; ++i)
        {
            std::array<Dst, N> dstval = {};
            const common::span<const Dst> refval{ ref, N };
            ref += N;
            for (uint32_t j = 0; j < N; ++j)
                dstval[j] = dsts[j][i];

            HexTest<Src, Dst> test(count, i);
            if constexpr (M == 1)
                test.SetSrc(src[i]);
            else
                test.SetSrc(common::span<const Src>{ src + i * M, M });
            test.SetOut(dstval, refval);

            EXPECT_EQ(test.Dst, test.Ref) << test;
            isSuc = isSuc && (test.Dst == test.Ref);
        }
        if (total <= size || !isSuc) break;
    }
}

INTRIN_TEST(ColorCvt, Extract8x2)
{
    SCOPED_TRACE("ColorCvt::Extract8x2");
    const auto src = GetRandVals();
    VarLenExtractTest<uint16_t, uint8_t, 1, 2>(src, [&](const std::array<uint8_t*, 2>& dsts, const uint16_t* src, size_t count)
    {
        Intrin->RAToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract8x3)
{
    SCOPED_TRACE("ColorCvt::Extract8x3");
    const auto src = GetRandVals();
    VarLenExtractTest<uint8_t, uint8_t, 3, 3>(src, [&](const std::array<uint8_t*, 3>& dsts, const uint8_t* src, size_t count)
    {
        Intrin->RGBToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract8x4)
{
    SCOPED_TRACE("ColorCvt::Extract8x4");
    const auto src = GetRandVals();
    VarLenExtractTest<uint32_t, uint8_t, 1, 4>(src, [&](const std::array<uint8_t*, 4>& dsts, const uint32_t* src, size_t count)
    {
        Intrin->RGBAToPlanar(dsts, src, count);
    });
}

template<bool isRGB>
void Test555ToRGB(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint8_t, 1, 3>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr(isRGB) intrin->RGB555ToRGB(dst, src, count);
        else intrin->BGR555ToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(((val >>  0) & 0x1fu) << 3);
            const auto g = static_cast<uint8_t>(((val >>  5) & 0x1fu) << 3);
            const auto b = static_cast<uint8_t>(((val >> 10) & 0x1fu) << 3);
            *dst++ = isRGB ? r : b;
            *dst++ = g;
            *dst++ = isRGB ? b : r;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB555ToRGB8)
{
    SCOPED_TRACE("ColorCvt::RGB555ToRGB8");
    Test555ToRGB<true>(Intrin);
}

INTRIN_TEST(ColorCvt, BGR555ToRGB8)
{
    SCOPED_TRACE("ColorCvt::BGR555ToRGB8");
    Test555ToRGB<false>(Intrin);
}

template<bool isRGB, bool hasAlpha>
void Test555ToRGBA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint32_t, 1, 1>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint32_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr(isRGB) intrin->RGB555ToRGBA(dst, src, count, hasAlpha);
        else intrin->BGR555ToRGBA(dst, src, count, hasAlpha);
    }, [](uint32_t* dst_, const uint16_t* src, size_t count)
    {
        auto dst = reinterpret_cast<uint8_t*>(dst_);
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(((val >>  0) & 0x1fu) << 3);
            const auto g = static_cast<uint8_t>(((val >>  5) & 0x1fu) << 3);
            const auto b = static_cast<uint8_t>(((val >> 10) & 0x1fu) << 3);
            *dst++ = isRGB ? r : b;
            *dst++ = g;
            *dst++ = isRGB ? b : r;
            if constexpr (hasAlpha) *dst++ = val & 0x8000u ? 0xff : 0x0;
            else *dst++ = 0xff;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB555ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::RGB555ToRGBA8");
    Test555ToRGBA<true, false>(Intrin);
}

INTRIN_TEST(ColorCvt, BGR555ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::BGR555ToRGBA8");
    Test555ToRGBA<false, false>(Intrin);
}

INTRIN_TEST(ColorCvt, RGB5551ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::RGB5551ToRGBA8");
    Test555ToRGBA<true, true>(Intrin);
}

INTRIN_TEST(ColorCvt, BGR5551ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::BGR5551ToRGBA8");
    Test555ToRGBA<false, true>(Intrin);
}


template<bool isRGB>
void Test565ToRGB(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint8_t, 1, 3>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr (isRGB) intrin->RGB565ToRGB(dst, src, count);
        else intrin->BGR565ToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(((val >>  0) & 0x1fu) << 3);
            const auto g = static_cast<uint8_t>(((val >>  5) & 0x3fu) << 2);
            const auto b = static_cast<uint8_t>(((val >> 11) & 0x1fu) << 3);
            *dst++ = isRGB ? r : b;
            *dst++ = g;
            *dst++ = isRGB ? b : r;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB565ToRGB8)
{
    SCOPED_TRACE("ColorCvt::RGB565ToRGB8");
    Test565ToRGB<true>(Intrin);
}

INTRIN_TEST(ColorCvt, BGR565ToRGB8)
{
    SCOPED_TRACE("ColorCvt::BGR565ToRGB8");
    Test565ToRGB<false>(Intrin);
}

template<bool isRGB>
void Test565ToRGBA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint32_t, 1, 1>(reinterpret_cast<const uint16_t*>(src.data()), src.size() / 2, [&](uint32_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr (isRGB) intrin->RGB565ToRGBA(dst, src, count);
        else intrin->BGR565ToRGBA(dst, src, count);
    }, [](uint32_t* dst_, const uint16_t* src, size_t count)
    {
        auto dst = reinterpret_cast<uint8_t*>(dst_);
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<uint8_t>(((val >>  0) & 0x1fu) << 3);
            const auto g = static_cast<uint8_t>(((val >>  5) & 0x3fu) << 2);
            const auto b = static_cast<uint8_t>(((val >> 11) & 0x1fu) << 3);
            *dst++ = isRGB ? r : b;
            *dst++ = g;
            *dst++ = isRGB ? b : r;
            *dst++ = 0xff;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB565ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::RGB565ToRGBA8");
    Test565ToRGBA<true>(Intrin);
}

INTRIN_TEST(ColorCvt, BGR565ToRGBA8)
{
    SCOPED_TRACE("ColorCvt::BGR565ToRGBA8");
    Test565ToRGBA<false>(Intrin);
}

template<bool IsRGB, bool NeedAlpha, bool HasAlpha>
void Test10ToRGBA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin, uint32_t range_)
{
    const auto range = range_ == 10243 ? 0.f : static_cast<float>(range_);
    const auto src = GetRandVals();
    VarLenTest<uint32_t, float, 1, NeedAlpha ? 4 : 3>(reinterpret_cast<const uint32_t*>(src.data()), src.size() / 4, [&](float* dst, const uint32_t* src, size_t count)
    {
        if constexpr (NeedAlpha)
        {
            if constexpr (IsRGB) intrin->RGB10A2ToRGBA(dst, src, count, range, HasAlpha);
            else intrin->BGR10A2ToRGBA(dst, src, count, range, HasAlpha);
        }
        else
        {
            if constexpr (IsRGB) intrin->RGB10A2ToRGB(dst, src, count, range);
            else intrin->BGR10A2ToRGB(dst, src, count, range);
        }
    }, [&](float* dst, const uint32_t* src, size_t count)
    {
        const auto mulVal = range_ / 1023.f;
        while (count)
        {
            const auto val = *src++;
            const auto r = static_cast<int32_t>((val >>  0) & 0x3ffu) * mulVal;
            const auto g = static_cast<int32_t>((val >> 10) & 0x3ffu) * mulVal;
            const auto b = static_cast<int32_t>((val >> 20) & 0x3ffu) * mulVal;
            *dst++ = IsRGB ? r : b;
            *dst++ = g;
            *dst++ = IsRGB ? b : r;
            if constexpr (NeedAlpha)
                *dst++ = HasAlpha ? static_cast<int32_t>(val >> 30) / 3.0f : 1.0f;
            count--;
        }
    });
}
template<bool IsRGB, bool NeedAlpha, bool HasAlpha>
void Test10ToRGBA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin, std::string name)
{
    {
        const ::testing::ScopedTrace scope(__FILE__, __LINE__, name + ", range=1023");
        Test10ToRGBA<IsRGB, NeedAlpha, HasAlpha>(intrin, 1023u);
    }
    {
        const ::testing::ScopedTrace scope(__FILE__, __LINE__, name + ", range=1");
        Test10ToRGBA<IsRGB, NeedAlpha, HasAlpha>(intrin, 1u);
    }
    {
        const ::testing::ScopedTrace scope(__FILE__, __LINE__, name + ", range=3");
        Test10ToRGBA<IsRGB, NeedAlpha, HasAlpha>(intrin, 3u);
    }
}

INTRIN_TEST(ColorCvt, RGB10ToRGBf)
{
    Test10ToRGBA<true, false, false>(Intrin, "ColorCvt::RGB10ToRGBf");
}

INTRIN_TEST(ColorCvt, BGR10ToRGBf)
{
    Test10ToRGBA<false, false, false>(Intrin, "ColorCvt::BGR10ToRGBf");
}

INTRIN_TEST(ColorCvt, RGB10ToRGBAf)
{
    Test10ToRGBA<true, true, false>(Intrin, "ColorCvt::RGB10ToRGBAf");
}

INTRIN_TEST(ColorCvt, BGR10ToRGBAf)
{
    Test10ToRGBA<false, true, false>(Intrin, "ColorCvt::BGR10ToRGBAf");
}

INTRIN_TEST(ColorCvt, RGB10A2ToRGBAf)
{
    Test10ToRGBA<true, true, true>(Intrin, "ColorCvt::RGB10A2ToRGBAf");
}

INTRIN_TEST(ColorCvt, BGR10A2ToRGBAf)
{
    Test10ToRGBA<false, true, true>(Intrin, "ColorCvt::BGR10A2ToRGBAf");
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

TEST(IntrinPerf, GA8ToRGB8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint8_t> dst(Size * 3);
    PerfTester::DoFastPath(&xziar::img::ColorConvertor::GrayAToRGB, "GA8ToRGB8", Size, 150,
        dst.data(), src.data(), Size);
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

TEST(IntrinPerf, RGBAGetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dst(Size);

    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannel, "RGBA8ToR8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannel, "RGBA8ToG8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannel, "RGBA8ToB8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(2));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannel, "RGBA8ToA8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(3));
}

TEST(IntrinPerf, RGBGetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint8_t> dst(Size);

    PerfTester::DoFastPath(&ColorConvertor::RGBGetChannel, "RGB8ToR8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(&ColorConvertor::RGBGetChannel, "RGB8ToG8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(&ColorConvertor::RGBGetChannel, "RGB8ToB8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(2));
}

TEST(IntrinPerf, RGBAGetChannelAlpha)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint16_t> dst(Size);

    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToRA8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToGA8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToBA8", Size, 150,
        dst.data(), src.data(), Size, uint8_t(2));
}

TEST(IntrinPerf, Extract8)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dsts[4];
    for (auto& dst : dsts)
        dst.resize(Size);
    uint8_t* const ptrs[4] = {dsts[0].data(), dsts[1].data(), dsts[2].data(), dsts[3].data()};

    PerfTester::DoFastPath(&ColorConvertor::RAToPlanar, "Extract8x2", Size, 150,
        common::span<uint8_t* const, 2>{ ptrs, 2}, reinterpret_cast<const uint16_t*>(src.data()), Size);
    PerfTester::DoFastPath(&ColorConvertor::RGBToPlanar, "Extract8x3", Size, 150,
        common::span<uint8_t* const, 3>{ ptrs, 3}, reinterpret_cast<const uint8_t*>(src.data()), Size);
    PerfTester::DoFastPath(&ColorConvertor::RGBAToPlanar, "Extract8x4", Size, 150,
        common::span<uint8_t* const, 4>{ ptrs, 4}, reinterpret_cast<const uint32_t*>(src.data()), Size);
}

TEST(IntrinPerf, RGB555ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dstRGBA(Size);
    std::vector<uint8_t> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGBA, "RGB5551ToRGBA8", Size, 150, 
        dstRGBA.data(), src.data(), Size, true);
    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGBA, "RGB555ToRGBA8", Size, 150,
        dstRGBA.data(), src.data(), Size, false);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGBA, "BGR5551ToRGBA8", Size, 150,
        dstRGBA.data(), src.data(), Size, true);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGBA, "BGR555ToRGBA8", Size, 150,
        dstRGBA.data(), src.data(), Size, false);
    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGB, "RGB555ToRGB8", Size, 150,
        dstRGB.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGB, "BGR555ToRGB8", Size, 150,
        dstRGB.data(), src.data(), Size);
}

TEST(IntrinPerf, RGB565ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dstRGBA(Size);
    std::vector<uint8_t> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB565ToRGBA, "RGB565ToRGBA8", Size, 150,
        dstRGBA.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR565ToRGBA, "BGR565ToRGBA8", Size, 150,
        dstRGBA.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::RGB565ToRGB, "RGB565ToRGB8", Size, 150,
        dstRGB.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR565ToRGB, "BGR565ToRGB8", Size, 150,
        dstRGB.data(), src.data(), Size);
}

TEST(IntrinPerf, RGB10ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint32_t> src(Size);
    std::vector<float> dstRGBA(Size * 4);
    std::vector<float> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGBA, "RGB10A2ToRGBAf", Size, 150,
        dstRGBA.data(), src.data(), Size, 1.0f, true);
    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGBA, "RGB10ToRGBAf", Size, 150,
        dstRGBA.data(), src.data(), Size, 1.0f, false);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGBA, "BGR10A2ToRGBAf", Size, 150,
        dstRGBA.data(), src.data(), Size, 1.0f, true);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGBA, "BGR10ToRGBAf", Size, 150,
        dstRGBA.data(), src.data(), Size, 1.0f, false);
    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGB, "RGB10ToRGBf", Size, 150,
        dstRGB.data(), src.data(), Size, 1.0f);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGB, "BGR10ToRGBf", Size, 150,
        dstRGB.data(), src.data(), Size, 1.0f);
}

#endif