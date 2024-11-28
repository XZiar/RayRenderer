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
        return (Vals.size() == other.Vals.size()) && memcmp(Vals.data(), other.Vals.data(), Vals.size_bytes()) == 0;
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
static void VarLenTest(common::span<const std::byte> source, F&& func, R&& reff)
{
    constexpr bool TryInplace = std::is_same_v<Src, Dst> && M == N;
    const auto src = reinterpret_cast<const Src*>(source.data());
    const auto total = source.size() / (sizeof(Src) * M);
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
    VarLenTest<uint8_t, uint16_t, 1, 1>(src, [&](uint16_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint8_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint8_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint16_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint16_t* src, size_t count)
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
    VarLenTest<uint16_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, GfToGAf)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = 2.5f;
    SCOPED_TRACE("ColorCvt::GfToGAf");
    VarLenTest<float, float, 1, 2>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->GrayToGrayA(dst, src, count, RefAlpha);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src++;
            *dst++ = RefAlpha;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, GfToRGB8)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GfToRGBf");
    VarLenTest<float, float, 1, 3>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->GrayToRGB(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
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

INTRIN_TEST(ColorCvt, GfToRGBAf)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = 3.75f;
    SCOPED_TRACE("ColorCvt::GfToRGBAf");
    VarLenTest<float, float, 1, 4>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->GrayToRGBA(dst, src, count, RefAlpha);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src;
            *dst++ = *src;
            *dst++ = *src++;
            *dst++ = RefAlpha;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, GAfToRGBf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GAfToRGBf");
    VarLenTest<float, float, 2, 3>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->GrayAToRGB(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            src++;
            *dst++ = val;
            *dst++ = val;
            *dst++ = val;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, GAfToRGBAf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GAfToRGBAf");
    VarLenTest<float, float, 2, 4>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->GrayAToRGBA(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src;
            *dst++ = *src;
            *dst++ = *src++;
            *dst++ = *src++;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB8ToRGBA8)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = std::byte(0x39);
    SCOPED_TRACE("ColorCvt::RGB8ToRGBA8");
    VarLenTest<uint8_t, uint32_t, 3, 1>(src, [&](uint32_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint8_t, uint32_t, 3, 1>(src, [&](uint32_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint32_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint32_t* src, size_t count)
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
    VarLenTest<uint32_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint32_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGBfToRGBAf)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = 4.5f;
    SCOPED_TRACE("ColorCvt::RGBfToRGBAf");
    VarLenTest<float, float, 3, 4>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->RGBToRGBA(dst, src, count, RefAlpha);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = RefAlpha;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, BGRfToRGBAf)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = 0.25f;
    SCOPED_TRACE("ColorCvt::BGRfToRGBAf");
    VarLenTest<float, float, 3, 4>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->BGRToRGBA(dst, src, count, RefAlpha);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            *dst++ = RefAlpha;
            src += 3;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGBAfToRGBf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBAfToRGBf");
    VarLenTest<float, float, 4, 3>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->RGBAToRGB(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            src++;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGBAfToBGRf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBAfToBGRf");
    VarLenTest<float, float, 4, 3>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->RGBAToBGR(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            src += 4;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, RGB8ToBGR8)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGB8ToBGR8");
    VarLenTest<uint8_t, uint8_t, 3, 3>(src, [&](uint8_t* dst, const uint8_t* src, size_t count)
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
    VarLenTest<uint32_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint32_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGBfToBGRf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBfToBGRf");
    VarLenTest<float, float, 3, 3>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->RGBToBGR(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGBAfToBGRAf)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBAfToBGRAf");
    VarLenTest<float, float, 4, 4>(src, [&](float* dst, const float* src, size_t count)
    {
        Intrin->RGBAToBGRA(dst, src, count);
    }, [](float* dst, const float* src, size_t count)
    {
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
    VarLenTest<uint32_t, uint8_t, 1, 1>(src, [&](uint8_t* dst, const uint32_t* src, size_t count)
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
    VarLenTest<uint8_t, uint8_t, 3, 1>(src, [&](uint8_t* dst, const uint8_t* src, size_t count)
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

template<uint8_t N, uint8_t Ch>
void TestGetChannelF(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<float, float, N, 1>(src, [&](float* dst, const float* src, size_t count)
    {
        if constexpr (N == 4)
            intrin->RGBAGetChannel(dst, src, count, Ch);
        else
            intrin->RGBGetChannel(dst, src, count, Ch);
    }, [](float* dst, const float* src, size_t count)
    {
        while (count)
        {
            *dst++ = src[Ch];
            src += N;
            count--;
        }
    });
}
INTRIN_TEST(ColorCvt, RGBAfToRf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToRf");
    TestGetChannelF<4, 0>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToGf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToGf");
    TestGetChannelF<4, 1>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToBf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToBf");
    TestGetChannelF<4, 2>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToAf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToAf");
    TestGetChannelF<4, 3>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToRf)
{
    SCOPED_TRACE("ColorCvt::RGBfToRf");
    TestGetChannelF<3, 0>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToGf)
{
    SCOPED_TRACE("ColorCvt::RGBfToGf");
    TestGetChannelF<3, 1>(Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToBf)
{
    SCOPED_TRACE("ColorCvt::RGBfToBf");
    TestGetChannelF<3, 2>(Intrin);
}

template<uint8_t Ch>
void TestGetChannelA(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint32_t, uint16_t, 1, 1>(src, [&](uint16_t* dst, const uint32_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, Extract32x2)
{
    SCOPED_TRACE("ColorCvt::Extract32x2");
    const auto src = GetRandVals();
    VarLenExtractTest<float, float, 2, 2>(src, [&](const std::array<float*, 2>& dsts, const float* src, size_t count)
    {
        Intrin->RAToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract32x3)
{
    SCOPED_TRACE("ColorCvt::Extract32x3");
    const auto src = GetRandVals();
    VarLenExtractTest<float, float, 3, 3>(src, [&](const std::array<float*, 3>& dsts, const float* src, size_t count)
    {
        Intrin->RGBToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract32x4)
{
    SCOPED_TRACE("ColorCvt::Extract32x4");
    const auto src = GetRandVals();
    VarLenExtractTest<float, float, 4, 4>(src, [&](const std::array<float*, 4>& dsts, const float* src, size_t count)
    {
        Intrin->RGBAToPlanar(dsts, src, count);
    });
}


template<typename Src, typename Dst, size_t M, size_t N, typename F>
static void VarLenCombineTest(common::span<const std::byte> source, F&& func)
{
    static_assert(sizeof(Src) * M == sizeof(Dst) * N);
    const auto srcptr = reinterpret_cast<const Src*>(source.data());
    const auto total = source.size() / sizeof(Src) / M;
    for (const auto size : TestSizes)
    {
        const auto count = std::min(total, size);
        std::vector<Dst> dst(count * N, static_cast<Dst>(0xcc));
        std::array<const Src*, M> ptrs = {};
        for (uint32_t i = 0; i < M; ++i)
        {
            ptrs[i] = srcptr + count * i;
        }
        func(dst.data(), ptrs, count);
        bool isSuc = true;
        for (size_t i = 0; i < count; ++i)
        {
            const common::span<const Dst> dstval{ dst.data() + N * i, N};
            std::array<Src, M> srcval = { };
            for (uint32_t j = 0; j < M; ++j)
                srcval[j] = ptrs[j][i];

            HexTest<Src, Dst> test(count, i);
            test.SetSrc(srcval);
            test.SetOut(dstval, common::span<const Dst>{ reinterpret_cast<const Dst*>(srcval.data()), N });

            EXPECT_EQ(test.Dst, test.Ref) << test;
            isSuc = isSuc && (test.Dst == test.Ref);
        }
        if (total <= size || !isSuc) break;
    }
}

INTRIN_TEST(ColorCvt, Combine8x2)
{
    SCOPED_TRACE("ColorCvt::Combine8x2");
    const auto src = GetRandVals();
    VarLenCombineTest<uint8_t, uint16_t, 2, 1>(src, [&](uint16_t* dst, const std::array<const uint8_t*, 2>& srcs, size_t count)
    {
        Intrin->PlanarToRA(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine8x3)
{
    SCOPED_TRACE("ColorCvt::Combine8x3");
    const auto src = GetRandVals();
    VarLenCombineTest<uint8_t, uint8_t, 3, 3>(src, [&](uint8_t* dst, const std::array<const uint8_t*, 3>& srcs, size_t count)
    {
        Intrin->PlanarToRGB(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine8x4)
{
    SCOPED_TRACE("ColorCvt::Combine8x4");
    const auto src = GetRandVals();
    VarLenCombineTest<uint8_t, uint32_t, 4, 1>(src, [&](uint32_t* dst, const std::array<const uint8_t*, 4>& srcs, size_t count)
    {
        Intrin->PlanarToRGBA(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine32x2)
{
    SCOPED_TRACE("ColorCvt::Combine32x2");
    const auto src = GetRandVals();
    VarLenCombineTest<float, float, 2, 2>(src, [&](float* dst, const std::array<const float*, 2>& srcs, size_t count)
    {
        Intrin->PlanarToRA(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine32x3)
{
    SCOPED_TRACE("ColorCvt::Combine32x3");
    const auto src = GetRandVals();
    VarLenCombineTest<float, float, 3, 3>(src, [&](float* dst, const std::array<const float*, 3>& srcs, size_t count)
    {
        Intrin->PlanarToRGB(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine32x4)
{
    SCOPED_TRACE("ColorCvt::Combine32x4");
    const auto src = GetRandVals();
    VarLenCombineTest<float, float, 4, 4>(src, [&](float* dst, const std::array<const float*, 4>& srcs, size_t count)
    {
        Intrin->PlanarToRGBA(dst, srcs, count);
    });
}

template<bool isRGB>
void Test555ToRGB(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<uint16_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr(isRGB) intrin->RGB555ToRGB(dst, src, count);
        else intrin->BGR555ToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r_ = static_cast<float>((val >>  0) & 0x1f);
            const auto g_ = static_cast<float>((val >>  5) & 0x1f);
            const auto b_ = static_cast<float>((val >> 10) & 0x1f);
            const auto r = static_cast<uint8_t>(std::floor(r_ * 255.0f / 31.0f + 0.5f));
            const auto g = static_cast<uint8_t>(std::floor(g_ * 255.0f / 31.0f + 0.5f));
            const auto b = static_cast<uint8_t>(std::floor(b_ * 255.0f / 31.0f + 0.5f));
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
    VarLenTest<uint16_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr(isRGB) intrin->RGB555ToRGBA(dst, src, count, hasAlpha);
        else intrin->BGR555ToRGBA(dst, src, count, hasAlpha);
    }, [](uint32_t* dst_, const uint16_t* src, size_t count)
    {
        auto dst = reinterpret_cast<uint8_t*>(dst_);
        while (count)
        {
            const auto val = *src++;
            const auto r_ = static_cast<float>((val >>  0) & 0x1f);
            const auto g_ = static_cast<float>((val >>  5) & 0x1f);
            const auto b_ = static_cast<float>((val >> 10) & 0x1f);
            const auto r = static_cast<uint8_t>(std::floor(r_ * 255.0f / 31.0f + 0.5f));
            const auto g = static_cast<uint8_t>(std::floor(g_ * 255.0f / 31.0f + 0.5f));
            const auto b = static_cast<uint8_t>(std::floor(b_ * 255.0f / 31.0f + 0.5f));
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
    VarLenTest<uint16_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr (isRGB) intrin->RGB565ToRGB(dst, src, count);
        else intrin->BGR565ToRGB(dst, src, count);
    }, [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            const auto r_ = static_cast<float>((val >>  0) & 0x1f);
            const auto g_ = static_cast<float>((val >>  5) & 0x3f);
            const auto b_ = static_cast<float>((val >> 11) & 0x1f);
            const auto r = static_cast<uint8_t>(std::floor(r_ * 255.0f / 31.0f + 0.5f));
            const auto g = static_cast<uint8_t>(std::floor(g_ * 255.0f / 63.0f + 0.5f));
            const auto b = static_cast<uint8_t>(std::floor(b_ * 255.0f / 31.0f + 0.5f));
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
    VarLenTest<uint16_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint16_t* src, size_t count)
    {
        if constexpr (isRGB) intrin->RGB565ToRGBA(dst, src, count);
        else intrin->BGR565ToRGBA(dst, src, count);
    }, [](uint32_t* dst_, const uint16_t* src, size_t count)
    {
        auto dst = reinterpret_cast<uint8_t*>(dst_);
        while (count)
        {
            const auto val = *src++;
            const auto r_ = static_cast<float>((val >>  0) & 0x1f);
            const auto g_ = static_cast<float>((val >>  5) & 0x3f);
            const auto b_ = static_cast<float>((val >> 11) & 0x1f);
            const auto r = static_cast<uint8_t>(std::floor(r_ * 255.0f / 31.0f + 0.5f));
            const auto g = static_cast<uint8_t>(std::floor(g_ * 255.0f / 63.0f + 0.5f));
            const auto b = static_cast<uint8_t>(std::floor(b_ * 255.0f / 31.0f + 0.5f));
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
    VarLenTest<uint32_t, float, 1, NeedAlpha ? 4 : 3>(src, [&](float* dst, const uint32_t* src, size_t count)
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


constexpr auto Cvt8_12 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint8_t*, size_t, std::byte) const>(&xziar::img::ColorConvertor::GrayToGrayA);
constexpr auto Cvt8_13 = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t * const, const uint8_t*, size_t) const>(&xziar::img::ColorConvertor::GrayToRGB);
constexpr auto Cvt8_14 = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t* const, const uint8_t*, size_t, std::byte) const>(&xziar::img::ColorConvertor::GrayToRGBA);
constexpr auto Cvt8_23 = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t * const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGB);
constexpr auto Cvt8_24 = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t* const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGBA);
constexpr auto Cvt32_12 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t, float) const>(&xziar::img::ColorConvertor::GrayToGrayA);
constexpr auto Cvt32_13 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t) const>(&xziar::img::ColorConvertor::GrayToRGB);
constexpr auto Cvt32_14 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t, float) const>(&xziar::img::ColorConvertor::GrayToRGBA);
constexpr auto Cvt32_23 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGB);
constexpr auto Cvt32_24 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGBA);


TEST(IntrinPerf, G8To8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size);
    std::vector<uint16_t> dst2(Size);
    std::vector<uint8_t> dst3(Size * 3);
    std::vector<uint32_t> dst4(Size);
    PerfTester::DoFastPath(Cvt8_12, "G8ToGA8", Size, 100,
        dst2.data(), src.data(), Size, std::byte(0xff));
    PerfTester::DoFastPath(Cvt8_13, "G8ToRGB8", Size, 100,
        dst3.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt8_14, "G8ToRGBA8", Size, 100,
        dst4.data(), src.data(), Size, std::byte(0xff));
}
TEST(IntrinPerf, GA8To8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint8_t> dst3(Size * 3);
    std::vector<uint32_t> dst4(Size);
    PerfTester::DoFastPath(Cvt8_23, "GA8ToRGB8", Size, 100,
        dst3.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt8_24, "GA8ToRGBA8", Size, 100,
        dst4.data(), src.data(), Size);
}

TEST(IntrinPerf, G32To32)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size);
    std::vector<float> dst(Size * 4);
    PerfTester::DoFastPath(Cvt32_12, "GfToGAf", Size, 100,
        dst.data(), src.data(), Size, 1.0f);
    PerfTester::DoFastPath(Cvt32_13, "GfToRGBf", Size, 100,
        dst.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt32_14, "GfToRGBAf", Size, 100,
        dst.data(), src.data(), Size, 1.0f);
}
TEST(IntrinPerf, GA32To32)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 2);
    std::vector<float> dst(Size * 4);
    PerfTester::DoFastPath(Cvt32_23, "GAfToRGBf", Size, 100,
        dst.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt32_24, "GAfToRGBAf", Size, 100,
        dst.data(), src.data(), Size);
}

TEST(IntrinPerf, RGB8ToRGBA8)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint32_t> dst(Size);
    PerfTester tester1("RGB8ToRGBA8", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToRGBA(dst.data(), src.data(), Size);
    });
    PerfTester tester2("BGR8ToRGBA8", Size, 100);
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
    PerfTester tester1("RGBA8ToRGB8", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToRGB(dst.data(), src.data(), Size);
    });
    PerfTester tester2("RGBA8ToBGR8", Size, 100);
    tester2.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToBGR(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBfToRGBAf)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 3);
    std::vector<float> dst(Size * 4);
    PerfTester tester1("RGBfToRGBAf", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToRGBA(dst.data(), src.data(), Size);
    });
    PerfTester tester2("BGRfToRGBAf", Size, 100);
    tester2.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.BGRToRGBA(dst.data(), src.data(), Size);
    });
}
TEST(IntrinPerf, RGBAfToRGBf)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 4);
    std::vector<float> dst(Size * 3);
    PerfTester tester1("RGBAfToRGBf", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToRGB(dst.data(), src.data(), Size);
    });
    PerfTester tester2("RGBAfToBGRf", Size, 100);
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
    PerfTester tester("RGB8ToBGR8", Size, 100);
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
    PerfTester tester("RGBA8ToBGRA8", Size, 100);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToBGRA(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBfToBGRf)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 3);
    std::vector<float> dst(Size * 3);
    PerfTester tester("RGBfToBGRf", Size, 100);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToBGR(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBAfToBGRAf)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 4);
    std::vector<float> dst(Size * 4);
    PerfTester tester("RGBAfToBGRAf", Size, 100);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToBGRA(dst.data(), src.data(), Size);
    });
}

constexpr auto Ext8x4_1  = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t* const, const uint32_t*, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBAGetChannel);
constexpr auto Ext8x3_1  = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t* const, const uint8_t*, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBGetChannel);
constexpr auto Ext32x4_1 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBAGetChannel);
constexpr auto Ext32x3_1 = static_cast<void (xziar::img::ColorConvertor::*)(float* const, const float*, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBGetChannel);

TEST(IntrinPerf, RGBAGetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dst(Size);

    PerfTester::DoFastPath(Ext8x4_1, "RGBA8ToR8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(Ext8x4_1, "RGBA8ToG8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(Ext8x4_1, "RGBA8ToB8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(2));
    PerfTester::DoFastPath(Ext8x4_1, "RGBA8ToA8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(3));
}

TEST(IntrinPerf, RGBGetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint8_t> dst(Size);

    PerfTester::DoFastPath(Ext8x3_1, "RGB8ToR8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(Ext8x3_1, "RGB8ToG8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(Ext8x3_1, "RGB8ToB8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(2));
}

TEST(IntrinPerf, RGBfGetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 4);
    std::vector<float> dst(Size);

    PerfTester::DoFastPath(Ext32x4_1, "RGBAfToRf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(Ext32x4_1, "RGBAfToGf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(Ext32x4_1, "RGBAfToBf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(2));
    PerfTester::DoFastPath(Ext32x4_1, "RGBAfToAf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(3));

    PerfTester::DoFastPath(Ext32x3_1, "RGBfToRf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(Ext32x3_1, "RGBfToGf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(Ext32x3_1, "RGBfToBf", Size, 100,
        dst.data(), src.data(), Size, uint8_t(2));
}

TEST(IntrinPerf, RGBAGetChannelAlpha)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint16_t> dst(Size);

    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToRA8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToGA8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(&ColorConvertor::RGBAGetChannelAlpha, "RGBA8ToBA8", Size, 100,
        dst.data(), src.data(), Size, uint8_t(2));
}

template<typename T, size_t N> using PtrSpan = common::span<T* const, N>;
template<size_t N> using PS8 = PtrSpan<uint8_t, N>;
template<size_t N> using PS32 = PtrSpan<float, N>;
template<size_t N> using PSC8 = PtrSpan<const uint8_t, N>;
template<size_t N> using PSC32 = PtrSpan<const float, N>;
constexpr auto Ext8x2  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <2>, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::RAToPlanar);
constexpr auto Ext8x3  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <3>, const uint8_t *, size_t) const>(&xziar::img::ColorConvertor::RGBToPlanar);
constexpr auto Ext8x4  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <4>, const uint32_t*, size_t) const>(&xziar::img::ColorConvertor::RGBAToPlanar);
constexpr auto Ext32x2 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<2>, const float*, size_t) const>(&xziar::img::ColorConvertor::RAToPlanar);
constexpr auto Ext32x3 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<3>, const float*, size_t) const>(&xziar::img::ColorConvertor::RGBToPlanar);
constexpr auto Ext32x4 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<4>, const float*, size_t) const>(&xziar::img::ColorConvertor::RGBAToPlanar);
constexpr auto Cmb8x2  = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t*, PSC8<2>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRA);
constexpr auto Cmb8x3  = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t *, PSC8<3>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGB);
constexpr auto Cmb8x4  = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t*, PSC8<4>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGBA);
constexpr auto Cmb32x2 = static_cast<void (xziar::img::ColorConvertor::*)(float*, PSC32<2>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRA);
constexpr auto Cmb32x3 = static_cast<void (xziar::img::ColorConvertor::*)(float*, PSC32<3>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGB);
constexpr auto Cmb32x4 = static_cast<void (xziar::img::ColorConvertor::*)(float*, PSC32<4>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGBA);

TEST(IntrinPerf, Extract8)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> src(Size);
    std::vector<uint8_t> dsts[4];
    for (auto& dst : dsts)
        dst.resize(Size);
    uint8_t* const ptrs[4] = {dsts[0].data(), dsts[1].data(), dsts[2].data(), dsts[3].data()};

    PerfTester::DoFastPath(Ext8x2, "Extract8x2", Size, 100,
        PS8<2>{ ptrs, 2}, reinterpret_cast<const uint16_t*>(src.data()), Size);
    PerfTester::DoFastPath(Ext8x3, "Extract8x3", Size, 100,
        PS8<3>{ ptrs, 3}, reinterpret_cast<const uint8_t*>(src.data()), Size);
    PerfTester::DoFastPath(Ext8x4, "Extract8x4", Size, 100,
        PS8<4>{ ptrs, 4}, reinterpret_cast<const uint32_t*>(src.data()), Size);
}

TEST(IntrinPerf, Extract32)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> src(Size * 4);
    std::vector<float> dsts[4];
    for (auto& dst : dsts)
        dst.resize(Size);
    float* const ptrs[4] = { dsts[0].data(), dsts[1].data(), dsts[2].data(), dsts[3].data() };

    PerfTester::DoFastPath(Ext32x2, "Extract32x2", Size, 100,
        PS32<2>{ ptrs, 2}, reinterpret_cast<const float*>(src.data()), Size);
    PerfTester::DoFastPath(Ext32x3, "Extract32x3", Size, 100,
        PS32<3>{ ptrs, 3}, reinterpret_cast<const float*>(src.data()), Size);
    PerfTester::DoFastPath(Ext32x4, "Extract32x4", Size, 100,
        PS32<4>{ ptrs, 4}, reinterpret_cast<const float*>(src.data()), Size);
}

TEST(IntrinPerf, Combine8)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint32_t> dst(Size);
    std::vector<uint8_t> srcs[4];
    for (auto& src : srcs)
        src.resize(Size);
    const uint8_t* const ptrs[4] = { srcs[0].data(), srcs[1].data(), srcs[2].data(), srcs[3].data() };

    PerfTester::DoFastPath(Cmb8x2, "Combine8x2", Size, 100,
        reinterpret_cast<uint16_t*>(dst.data()), PSC8<2>{ ptrs, 2}, Size);
    PerfTester::DoFastPath(Cmb8x3, "Combine8x3", Size, 100,
        reinterpret_cast<uint8_t*>(dst.data()), PSC8<3>{ ptrs, 3}, Size);
    PerfTester::DoFastPath(Cmb8x4, "Combine8x4", Size, 100,
        reinterpret_cast<uint32_t*>(dst.data()), PSC8<4>{ ptrs, 4}, Size);
}

TEST(IntrinPerf, Combine32)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<float> dst(Size * 4);
    std::vector<float> srcs[4];
    for (auto& src : srcs)
        src.resize(Size);
    float* const ptrs[4] = { srcs[0].data(), srcs[1].data(), srcs[2].data(), srcs[3].data() };

    PerfTester::DoFastPath(Cmb32x2, "Combine32x2", Size, 100,
        dst.data(), PSC32<2>{ ptrs, 2}, Size);
    PerfTester::DoFastPath(Cmb32x3, "Combine32x3", Size, 100,
        dst.data(), PSC32<3>{ ptrs, 3}, Size);
    PerfTester::DoFastPath(Cmb32x4, "Combine32x4", Size, 100,
        dst.data(), PSC32<4>{ ptrs, 4}, Size);
}

TEST(IntrinPerf, RGB555ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dstRGBA(Size);
    std::vector<uint8_t> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGBA, "RGB5551ToRGBA8", Size, 100, 
        dstRGBA.data(), src.data(), Size, true);
    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGBA, "RGB555ToRGBA8", Size, 100,
        dstRGBA.data(), src.data(), Size, false);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGBA, "BGR5551ToRGBA8", Size, 100,
        dstRGBA.data(), src.data(), Size, true);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGBA, "BGR555ToRGBA8", Size, 100,
        dstRGBA.data(), src.data(), Size, false);
    PerfTester::DoFastPath(&ColorConvertor::RGB555ToRGB, "RGB555ToRGB8", Size, 100,
        dstRGB.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR555ToRGB, "BGR555ToRGB8", Size, 100,
        dstRGB.data(), src.data(), Size);
}

TEST(IntrinPerf, RGB565ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint32_t> dstRGBA(Size);
    std::vector<uint8_t> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB565ToRGBA, "RGB565ToRGBA8", Size, 100,
        dstRGBA.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR565ToRGBA, "BGR565ToRGBA8", Size, 100,
        dstRGBA.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::RGB565ToRGB, "RGB565ToRGB8", Size, 100,
        dstRGB.data(), src.data(), Size);
    PerfTester::DoFastPath(&ColorConvertor::BGR565ToRGB, "BGR565ToRGB8", Size, 100,
        dstRGB.data(), src.data(), Size);
}

TEST(IntrinPerf, RGB10ToRGBA)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint32_t> src(Size);
    std::vector<float> dstRGBA(Size * 4);
    std::vector<float> dstRGB(Size * 3);

    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGBA, "RGB10A2ToRGBAf", Size, 100,
        dstRGBA.data(), src.data(), Size, 1.0f, true);
    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGBA, "RGB10ToRGBAf", Size, 100,
        dstRGBA.data(), src.data(), Size, 1.0f, false);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGBA, "BGR10A2ToRGBAf", Size, 100,
        dstRGBA.data(), src.data(), Size, 1.0f, true);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGBA, "BGR10ToRGBAf", Size, 100,
        dstRGBA.data(), src.data(), Size, 1.0f, false);
    PerfTester::DoFastPath(&ColorConvertor::RGB10A2ToRGB, "RGB10ToRGBf", Size, 100,
        dstRGB.data(), src.data(), Size, 1.0f);
    PerfTester::DoFastPath(&ColorConvertor::BGR10A2ToRGB, "BGR10ToRGBf", Size, 100,
        dstRGB.data(), src.data(), Size, 1.0f);
}

#endif