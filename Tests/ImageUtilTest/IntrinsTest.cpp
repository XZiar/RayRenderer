#include "rely.h"
#include "ImageUtil/ImageUtilRely.h"
#include "ImageUtil/ColorConvert.h"
#include "SystemCommon/Format.h"
#include "common/TimeUtil.hpp"
#include <algorithm>
#include <random>
#include <cmath>

#if COMMON_COMPILER_MSVC
#   pragma intrinsic(abs) // force opt
#endif


using common::enum_cast;
using common::enum_cast;

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

    void FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& ctx, const common::str::FormatSpec*) const noexcept
    {
        for (uint32_t i = 0; i < Vals.size(); ++i)
        {
            if (i > 0)
                host.PutString(ctx, " ", nullptr);
            using U = std::conditional_t<(sizeof(T) > sizeof(uint32_t)), uint64_t, uint32_t>;
            if constexpr (std::is_floating_point_v<T>)
            {
                host.PutFloat(ctx, Vals[i], nullptr);
                host.PutString(ctx, "(", nullptr);
                U intVal;
                memcpy_s(&intVal, sizeof(U), &Vals[i], sizeof(T));
                host.PutInteger(ctx, intVal, false, &SpecIntHex);
                host.PutString(ctx, ")", nullptr);
            }
            else
            {
                host.PutInteger(ctx, static_cast<U>(Vals[i]), false, &SpecIntHex);
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


template<typename Src, typename Dst, size_t M, size_t N, typename V, typename F, typename R>
static void VarLenTest_(common::span<const std::byte> source, F&& func, R&& reff, V&& veri, common::span<const size_t> testSizes = TestSizes)
{
    constexpr bool TryInplace = std::is_same_v<Src, Dst> && M == N;
    const auto src = reinterpret_cast<const Src*>(source.data());
    const auto total = source.size() / (sizeof(Src) * M);
    std::vector<Dst> ref;
    ref.resize(total * N, static_cast<Dst>(0xcc));
    reff(ref.data(), src, total);
    for (const auto size : testSizes)
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

                isSuc = isSuc && veri(test, cond);
            }
            cond = " [inplace]"; // only used when > 1
        }
        if (total <= size || !isSuc) break;
    }
}
template<typename Src, typename Dst, size_t M, size_t N, typename F, typename R>
static void VarLenTest(common::span<const std::byte> source, F&& func, R&& reff, common::span<const size_t> testSizes = TestSizes)
{
    VarLenTest_<Src, Dst, M, N>(source, std::forward<F>(func), std::forward<R>(reff), [](const HexTest<Src, Dst>& test, std::string_view cond)
    {
        EXPECT_EQ(test.Dst, test.Ref) << test << cond;
        return test.Dst == test.Ref;
    }, testSizes);
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

INTRIN_TEST(ColorCvt, G16ToGA16)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = (uint16_t)0x4000;
    SCOPED_TRACE("ColorCvt::G16ToGA16");
    VarLenTest<uint16_t, uint16_t, 1, 2>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayToGrayA(dst, src, count, RefAlpha);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            *dst++ = *src++;
            *dst++ = RefAlpha;
            count--;
        }
    });
}

INTRIN_TEST(ColorCvt, G16ToRGB16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::G16ToRGB16");
    VarLenTest<uint16_t, uint16_t, 1, 3>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayToRGB(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, G16ToRGBA16)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = (uint16_t)0x3000;
    SCOPED_TRACE("ColorCvt::G16ToRGBA16");
    VarLenTest<uint16_t, uint16_t, 1, 4>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayToRGBA(dst, src, count, RefAlpha);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, GA16ToRGB16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GA16ToRGB16");
    VarLenTest<uint16_t, uint16_t, 2, 3>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayAToRGB(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, GA16ToRGBA16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::GA16ToRGBA16");
    VarLenTest<uint16_t, uint16_t, 2, 4>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->GrayAToRGBA(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, GfToRGBf)
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

INTRIN_TEST(ColorCvt, RGB16ToRGBA16)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = (uint16_t)0x3000;
    SCOPED_TRACE("ColorCvt::RGB16ToRGBA16");
    VarLenTest<uint16_t, uint16_t, 3, 4>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->RGBToRGBA(dst, src, count, RefAlpha);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, BGR16ToRGBA16)
{
    const auto src = GetRandVals();
    constexpr auto RefAlpha = (uint16_t)0x4000;
    SCOPED_TRACE("ColorCvt::BGR16ToRGBA16");
    VarLenTest<uint16_t, uint16_t, 3, 4>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->BGRToRGBA(dst, src, count, RefAlpha);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGBA16ToRGB16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBA16ToRGB16");
    VarLenTest<uint16_t, uint16_t, 4, 3>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->RGBAToRGB(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGBA16ToBGR16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBA16ToBGR16");
    VarLenTest<uint16_t, uint16_t, 4, 3>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->RGBAToBGR(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, RGB16ToBGR16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGB16ToBGR16");
    VarLenTest<uint16_t, uint16_t, 3, 3>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->RGBToBGR(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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
INTRIN_TEST(ColorCvt, RGBA16ToBGRA16)
{
    const auto src = GetRandVals();
    SCOPED_TRACE("ColorCvt::RGBA16ToBGRA16");
    VarLenTest<uint16_t, uint16_t, 4, 4>(src, [&](uint16_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->RGBAToBGRA(dst, src, count);
    }, [](uint16_t* dst, const uint16_t* src, size_t count)
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

template<typename T1, typename TN, uint8_t N, uint8_t Ch>
void TestGetChannelN(const xziar::img::ColorConvertor& intrin)
{
    const auto src = GetRandVals();
    VarLenTest<TN, T1, sizeof(T1) * N / sizeof(TN), 1>(src, [&](T1* dst, const TN* src, size_t count)
    {
        if constexpr (N == 4)
            intrin.RGBAGetChannel(dst, src, count, Ch);
        else
            intrin.RGBGetChannel(dst, src, count, Ch);
    }, [](T1* dst, const TN* src_, size_t count)
    {
        auto src = reinterpret_cast<const T1*>(src_);
        while (count)
        {
            *dst++ = src[Ch];
            src += N;
            count--;
        }
    });
}

TEST(ColorCvtWrap, RGBAGetChannel)
{
    xziar::img::ColorConvertor host;
    TestGetChannelN<uint8_t,  uint32_t, 4, 0>(host);
    TestGetChannelN<uint8_t,  uint32_t, 4, 1>(host);
    TestGetChannelN<uint8_t,  uint32_t, 4, 2>(host);
    TestGetChannelN<uint8_t,  uint32_t, 4, 3>(host);
    TestGetChannelN<uint16_t, uint16_t, 4, 0>(host);
    TestGetChannelN<uint16_t, uint16_t, 4, 1>(host);
    TestGetChannelN<uint16_t, uint16_t, 4, 2>(host);
    TestGetChannelN<uint16_t, uint16_t, 4, 3>(host);
}

INTRIN_TEST(ColorCvt, RGB8ToR8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToR8");
    TestGetChannelN<uint8_t, uint8_t, 3, 0>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGB8ToG8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToG8");
    TestGetChannelN<uint8_t, uint8_t, 3, 1>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGB8ToB8)
{
    SCOPED_TRACE("ColorCvt::RGB8ToB8");
    TestGetChannelN<uint8_t, uint8_t, 3, 2>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGB16ToR16)
{
    SCOPED_TRACE("ColorCvt::RGB16ToR16");
    TestGetChannelN<uint16_t, uint16_t, 3, 0>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGB16ToG16)
{
    SCOPED_TRACE("ColorCvt::RGB16ToG16");
    TestGetChannelN<uint16_t, uint16_t, 3, 1>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGB16ToB16)
{
    SCOPED_TRACE("ColorCvt::RGB16ToB16");
    TestGetChannelN<uint16_t, uint16_t, 3, 2>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToRf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToRf");
    TestGetChannelN<float, float, 4, 0>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToGf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToGf");
    TestGetChannelN<float, float, 4, 1>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToBf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToBf");
    TestGetChannelN<float, float, 4, 2>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBAfToAf)
{
    SCOPED_TRACE("ColorCvt::RGBAfToAf");
    TestGetChannelN<float, float, 4, 3>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToRf)
{
    SCOPED_TRACE("ColorCvt::RGBfToRf");
    TestGetChannelN<float, float, 3, 0>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToGf)
{
    SCOPED_TRACE("ColorCvt::RGBfToGf");
    TestGetChannelN<float, float, 3, 1>(*Intrin);
}

INTRIN_TEST(ColorCvt, RGBfToBf)
{
    SCOPED_TRACE("ColorCvt::RGBfToBf");
    TestGetChannelN<float, float, 3, 2>(*Intrin);
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

INTRIN_TEST(ColorCvt, Extract16x2)
{
    SCOPED_TRACE("ColorCvt::Extract16x2");
    const auto src = GetRandVals();
    VarLenExtractTest<uint16_t, uint16_t, 2, 2>(src, [&](const std::array<uint16_t*, 2>& dsts, const uint16_t* src, size_t count)
    {
        Intrin->RAToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract16x3)
{
    SCOPED_TRACE("ColorCvt::Extract16x3");
    const auto src = GetRandVals();
    VarLenExtractTest<uint16_t, uint16_t, 3, 3>(src, [&](const std::array<uint16_t*, 3>& dsts, const uint16_t* src, size_t count)
    {
        Intrin->RGBToPlanar(dsts, src, count);
    });
}

INTRIN_TEST(ColorCvt, Extract16x4)
{
    SCOPED_TRACE("ColorCvt::Extract16x4");
    const auto src = GetRandVals();
    VarLenExtractTest<uint16_t, uint16_t, 4, 4>(src, [&](const std::array<uint16_t*, 4>& dsts, const uint16_t* src, size_t count)
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

INTRIN_TEST(ColorCvt, Combine16x2)
{
    SCOPED_TRACE("ColorCvt::Combine16x2");
    const auto src = GetRandVals();
    VarLenCombineTest<uint16_t, uint16_t, 2, 2>(src, [&](uint16_t* dst, const std::array<const uint16_t*, 2>& srcs, size_t count)
    {
        Intrin->PlanarToRA(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine16x3)
{
    SCOPED_TRACE("ColorCvt::Combine16x3");
    const auto src = GetRandVals();
    VarLenCombineTest<uint16_t, uint16_t, 3, 3>(src, [&](uint16_t* dst, const std::array<const uint16_t*, 3>& srcs, size_t count)
    {
        Intrin->PlanarToRGB(dst, srcs, count);
    });
}

INTRIN_TEST(ColorCvt, Combine16x4)
{
    SCOPED_TRACE("ColorCvt::Combine16x4");
    const auto src = GetRandVals();
    VarLenCombineTest<uint16_t, uint16_t, 4, 4>(src, [&](uint16_t* dst, const std::array<const uint16_t*, 4>& srcs, size_t count)
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


template<typename T>
static common::span<const std::byte> GetFillArray() noexcept
{
    static const auto Data = []() 
    {
        constexpr auto N = 1u << (sizeof(T) * 8);
        std::array<T, N> ret = {};
        for (size_t i = 0; i < N; ++i)
            ret[i] = static_cast<T>(i);
        return ret;
    }();
    return { reinterpret_cast<const std::byte*>(Data.data()), sizeof(Data) };
}

INTRIN_TEST(ColorCvt, G8ToG16)
{
    const auto func = [&](uint16_t* dst, const uint8_t* src, size_t count)
    {
        Intrin->Gray8To16(dst, src, count);
    };
    constexpr auto reff = [](uint16_t* dst, const uint8_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            *dst++ = static_cast<uint16_t>((val * 1.0f / UINT8_MAX) * UINT16_MAX);
            count--;
        }
    };
    VarLenTest<uint8_t, uint16_t, 1, 1>(GetRandVals(), func, reff);
    size_t fulltest[] = { 1u << 8 };
    VarLenTest<uint8_t, uint16_t, 1, 1>(GetFillArray<uint8_t>(), func, reff, fulltest);
}
INTRIN_TEST(ColorCvt, G16ToG8)
{
    const auto func = [&](uint8_t* dst, const uint16_t* src, size_t count)
    {
        Intrin->Gray16To8(dst, src, count);
    };
    constexpr auto reff = [](uint8_t* dst, const uint16_t* src, size_t count)
    {
        while (count)
        {
            const auto val = *src++;
            *dst++ = static_cast<uint8_t>((val * 1.0f / UINT16_MAX) * UINT8_MAX);
            count--;
        }
    };
    VarLenTest<uint16_t, uint8_t, 1, 1>(GetRandVals(), func, reff);
    size_t fulltest[] = { 1u << 16 };
    VarLenTest<uint16_t, uint8_t, 1, 1>(GetFillArray<uint16_t>(), func, reff, fulltest);
}

static const auto AllUint16 = []() 
{
    std::vector<uint16_t> ret;
    for (uint32_t i = 0; i <= UINT16_MAX; ++i)
        ret.push_back(static_cast<uint16_t>(i));
    return ret;
}();

template<bool isRGB>
void Test555ToRGB(const std::unique_ptr<xziar::img::ColorConvertor>& intrin)
{
    const common::span<const std::byte> src{ reinterpret_cast<const std::byte*>(AllUint16.data()), AllUint16.size() * sizeof(uint16_t) };
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(AllUint16.size());
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
    }, testSizes);
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
    const common::span<const std::byte> src{ reinterpret_cast<const std::byte*>(AllUint16.data()), AllUint16.size() * sizeof(uint16_t) };
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(AllUint16.size());
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
    }, testSizes);
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
    const common::span<const std::byte> src{ reinterpret_cast<const std::byte*>(AllUint16.data()), AllUint16.size() * sizeof(uint16_t) };
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(AllUint16.size());
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
    }, testSizes);
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
    const common::span<const std::byte> src{ reinterpret_cast<const std::byte*>(AllUint16.data()), AllUint16.size() * sizeof(uint16_t) };
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(AllUint16.size());
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
    }, testSizes);
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


INTRIN_TESTSUITE(YCCCvt, xziar::img::YCCConvertor, xziar::img::YCCConvertor::Get());


#if COMMON_COMPILER_MSVC
#   pragma optimize("t", on) // /Og /Oi /Ot /Oy /Ob2 /GF /Gy
#   define FORCE_OPT
#elif COMMON_COMPILER_GCC
#   define FORCE_OPT __attribute__((optimize("O2")))
#else
#   define FORCE_OPT
#endif

using ::xziar::img::RGB2YCCBase;
using xziar::img::YCCMatrix;
using xziar::img::EncodeYCCM;
FORCE_OPT static forceinline constexpr std::tuple<int32_t, int32_t, uint32_t> GetRGBRange(bool isFull) noexcept
{
    if (isFull) return { 0, 255, 256u * 256u * 256u };
    else return { 16, 235, 220u * 220u * 220u };
}
FORCE_OPT static forceinline constexpr std::tuple<int32_t, int32_t, int32_t, int32_t, uint32_t> GetYCCRange(bool isFull) noexcept
{
    if (isFull) return { 0, 255, 0, 255, 256u * 256u * 256u };
    else return { 16, 235, 16, 240, 220u * 225u * 225u };
}
FORCE_OPT static forceinline constexpr int32_t Clamp(int32_t val, int32_t vmin, int32_t vmax) noexcept
{
    return val < vmin ? vmin : (val > vmax ? vmax : val);
}
static constexpr std::string_view YCCMatrixName(YCCMatrix matrix) noexcept
{
    switch (matrix & YCCMatrix::TypeMask)
    {
    case YCCMatrix::BT601:  return "bt.601";
    case YCCMatrix::BT709:  return "bt.709";
    case YCCMatrix::BT2020: return "bt.2020";
    default: return "error";
    }
}

FORCE_OPT static auto CalcYCC8Output(YCCMatrix matrix, bool toYCC) noexcept
{
    const bool isRGBFull = HAS_FIELD(matrix, YCCMatrix::RGBFull);
    const bool isYCCFull = HAS_FIELD(matrix, YCCMatrix::YCCFull);
    const auto [rgbMin, rgbMax, rgbCnt] = GetRGBRange(isRGBFull);
    const auto [yMin, yMax, cMin, cMax, yuvCnt] = GetYCCRange(isYCCFull);
    const auto coeffIdx = common::enum_cast(matrix & YCCMatrix::TypeMask);
    const auto& coeff = RGB2YCCBase[coeffIdx];
    const auto cRange = (cMax - cMin + 1u) & 0xfffffffeu; // round up to make chroma symmetric
    const auto pbCoeff = 2 * (1.0 - coeff[2]), prCoeff = 2 * (1.0 - coeff[0]);

    std::vector<uint8_t> output;
    output.resize((toYCC ? rgbCnt : yuvCnt) * 3);

    if (toYCC)
    {
        const auto d2aRGB = 1.0 * (rgbMax - rgbMin);
        const double a2dY = yMax - yMin, a2dC = cRange;
        for (int32_t r_ = rgbMin, i = 0; r_ <= rgbMax; ++r_)
        {
            const double r = (r_ - rgbMin) / d2aRGB;
            for (int32_t g_ = rgbMin; g_ <= rgbMax; ++g_)
            {
                const double g = (g_ - rgbMin) / d2aRGB;
                for (int32_t b_ = rgbMin; b_ <= rgbMax; ++b_)
                {
                    const double b = (b_ - rgbMin) / d2aRGB;
                    const auto y = r * coeff[0] + g * coeff[1] + b * coeff[2];
                    const auto pb = (b - y) / pbCoeff, pr = (r - y) / prCoeff;
                    const auto y_  = static_cast<int32_t>(std::nearbyint(y * a2dY)) + yMin;
                    const auto cb_ = static_cast<int32_t>(std::nearbyint((pb + 0.5) * a2dC)) + cMin;
                    const auto cr_ = static_cast<int32_t>(std::nearbyint((pr + 0.5) * a2dC)) + cMin;
                    output[i++] = static_cast<uint8_t>(y_);
                    output[i++] = static_cast<uint8_t>(Clamp(cb_, cMin, cMax));
                    output[i++] = static_cast<uint8_t>(Clamp(cr_, cMin, cMax));
                }
            }
        }
    }
    else
    {
        const auto d2aY = 1.0 * (yMax - yMin);
        const auto d2aC = 1.0 * (cRange);
        const auto a2dRGB = 1.0 * (rgbMax - rgbMin);
        const auto pbGCoeff = pbCoeff * coeff[2] / coeff[1], prGCoeff = prCoeff * coeff[0] / coeff[1];
        for (int32_t y_ = yMin, i = 0; y_ <= yMax; ++y_)
        {
            const double y = (y_ - yMin) / d2aY;
            for (int32_t cb_ = cMin; cb_ <= cMax; ++cb_)
            {
                const double pb = (cb_ - cMin) / d2aC - 0.5;
                for (int32_t cr_ = cMin; cr_ <= cMax; ++cr_)
                {
                    const double pr = (cr_ - cMin) / d2aC - 0.5;
                    const auto r = y + pr * prCoeff;
                    const auto g = y - pb * pbGCoeff - pr * prGCoeff;
                    const auto b = y + pb * pbCoeff;

                    const auto r_ = static_cast<int32_t>(std::nearbyint(r * a2dRGB)) + rgbMin;
                    const auto g_ = static_cast<int32_t>(std::nearbyint(g * a2dRGB)) + rgbMin;
                    const auto b_ = static_cast<int32_t>(std::nearbyint(b * a2dRGB)) + rgbMin;

                    output[i++] = static_cast<uint8_t>(Clamp(r_, rgbMin, rgbMax));
                    output[i++] = static_cast<uint8_t>(Clamp(g_, rgbMin, rgbMax));
                    output[i++] = static_cast<uint8_t>(Clamp(b_, rgbMin, rgbMax));
                }
            }
        }
    }
    return output;
}
static const std::vector<uint8_t>& GetYCC8Output(YCCMatrix matrix, bool toYCC) noexcept
{
    static const auto refs = []() 
    {
        common::RegionRounding rd(common::RoundMode::ToNearest);
        common::SimpleTimer timer;
        timer.Start();
        std::map<uint8_t, std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> ret;
#define Gen(mat, rgb, ycc) do { const auto mval = EncodeYCCM(YCCMatrix::mat, rgb, ycc); ret[enum_cast(mval)] = \
    std::pair{ CalcYCC8Output(mval, true), CalcYCC8Output(mval, false) }; } while(0)
        Gen(BT601,  false, false);
        Gen(BT601,  true,  false);
        Gen(BT601,  false, true);
        Gen(BT601,  true,  true);
        Gen(BT709,  false, false);
        Gen(BT709,  true,  false);
        Gen(BT709,  false, true);
        Gen(BT709,  true,  true);
        Gen(BT2020, false, false);
        Gen(BT2020, true,  false);
        Gen(BT2020, false, true);
        Gen(BT2020, true,  true);
#undef Gen
        timer.Stop();
        TestCout() << "Calc FullYCC8 in [" << timer.ElapseMs() << "]ms\n";
        return ret;
    }();
    const auto& vecs = refs.find(enum_cast(matrix))->second;
    return toYCC ? vecs.first : vecs.second;
}
FORCE_OPT static const std::vector<uint8_t>& GetFullComp8x3(uint8_t idx) noexcept
{
    static const auto vecs = []() 
    {
        std::array<std::vector<uint8_t>, 3> ret;
        for (uint32_t x = 0; x < 3; ++x)
        {
            uint32_t rMin = 0, rMax = 0, gMin = 0, gMax = 0, bMin = 0, bMax = 0, cnt = 0;
            if (x < 2)
            {
                const auto [rgbMin, rgbMax, rgbCnt] = GetRGBRange(x == 0);
                rMin = gMin = bMin = rgbMin;
                rMax = gMax = bMax = rgbMax;
                cnt = rgbCnt;
            }
            else
            {
                const auto [yMin, yMax, cMin, cMax, yuvCnt] = GetYCCRange(false);
                rMin = yMin, rMax = yMax;
                gMin = bMin = cMin;
                gMax = bMax = cMax;
                cnt = yuvCnt;
            }
            auto& vec = ret[x];
            vec.resize(cnt * 3);
            for (uint32_t r = rMin, i = 0; r <= rMax; ++r)
                for (uint32_t g = gMin; g <= gMax; ++g)
                    for (uint32_t b = bMin; b <= bMax; ++b)
                    {
                        vec[i++] = static_cast<uint8_t>(r);
                        vec[i++] = static_cast<uint8_t>(g);
                        vec[i++] = static_cast<uint8_t>(b);
                    }
        }
        return ret;
    }();
    return vecs[idx];
}

MATCHER_P2(WrapRef, ref, res, "")
{
    [[maybe_unused]] const auto& tmp0 = arg;
    [[maybe_unused]] const auto& tmp1 = result_listener;
    return res;
}

#if COMMON_COMPILER_MSVC
#   pragma optimize("", on)
#endif
#undef FORCE_OPT

template<typename F, typename... Args>
void TestRGBYUV(const F& func, const std::unique_ptr<xziar::img::YCCConvertor>& intrin, Args&&... args)
{
    static const auto isSlim = IsSlimTest();
    func(intrin, YCCMatrix::BT601,  false, false,  true, std::forward<Args>(args)...);
    func(intrin, YCCMatrix::BT601,   true, false,  true, std::forward<Args>(args)...);
    func(intrin, YCCMatrix::BT601,   true,  true,  true, std::forward<Args>(args)...);
    func(intrin, YCCMatrix::BT601,  false, false, false, std::forward<Args>(args)...);
    func(intrin, YCCMatrix::BT601,   true, false, false, std::forward<Args>(args)...);
    func(intrin, YCCMatrix::BT601,   true,  true, false, std::forward<Args>(args)...);
    if (!isSlim)
    {
        func(intrin, YCCMatrix::BT709,  false, false, true, std::forward<Args>(args)...);
        func(intrin, YCCMatrix::BT709,   true, false, true, std::forward<Args>(args)...);
        func(intrin, YCCMatrix::BT709,   true,  true, true, std::forward<Args>(args)...);
        func(intrin, YCCMatrix::BT2020, false, false, true, std::forward<Args>(args)...);
        func(intrin, YCCMatrix::BT2020,  true, false, true, std::forward<Args>(args)...);
        func(intrin, YCCMatrix::BT2020,  true,  true, true, std::forward<Args>(args)...); 
    }
}

void TestRGBToYCC(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB, const bool isFast)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::RGB8ToYCbCr8" : "YCCCvt::BGR8ToYCbCr8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t RGB24Cnt = std::get<2>(GetRGBRange(isRGBFull));
    const auto& refOutput = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), true);
    Ensures(RGB24Cnt * 3 == refOutput.size());
    const auto& src24 = GetFullComp8x3(isRGBFull ? 0 : 1);
    Ensures(RGB24Cnt * 3 == src24.size());
    std::vector<uint8_t> srctemp;
    common::span<const std::byte> src;
    if (isRGB)
        src = common::as_bytes(common::to_span(src24));
    else
    {
        srctemp.resize(RGB24Cnt * 3);
        xziar::img::ColorConvertor::Get().RGBToBGR(srctemp.data(), src24.data(), RGB24Cnt);
        src = common::as_bytes(common::to_span(srctemp));
    }
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(RGB24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint8_t, uint8_t, 3, 3>(src, [&](uint8_t* dst, const uint8_t* src, size_t count)
    {
        if (isRGB)
            intrin->RGBToYCC(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
        else
            intrin->BGRToYCC(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
    }, [&](uint8_t* dst, const uint8_t*, size_t count)
    {
        memcpy(dst, refOutput.data(), count * 3);
    }, [&](const HexTest<uint8_t, uint8_t>& test, std::string_view cond) noexcept
    {
        const uint32_t absY = std::abs(test.Dst.Vals[0] - test.Ref.Vals[0]),
            absCb = std::abs(test.Dst.Vals[1] - test.Ref.Vals[1]),
            absCr = std::abs(test.Dst.Vals[2] - test.Ref.Vals[2]);
        const auto suc = (absY | absCb | absCr) <= 1; // only 0 or 1 allowed
        if (test.Count == RGB24Cnt)
            mismatches[0] += absY, mismatches[1] += absCb, mismatches[2] += absCr;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    // consider in-place, hald the mismatch count
    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: Y[{:.3f}%] Cb[{:.3f}%] Cr[{:.3f}%]\n"),
        situation, mismatches[0] * 50.0 / RGB24Cnt, mismatches[1] * 50.0 / RGB24Cnt, mismatches[2] * 50.0 / RGB24Cnt);
}
void TestRGBAToYCC(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB, const bool isFast)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::RGBA8ToYCbCr8" : "YCCCvt::BGRA8ToYCbCr8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t RGB24Cnt = std::get<2>(GetRGBRange(isRGBFull));
    const auto& refOutput = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), true);
    Ensures(RGB24Cnt * 3 == refOutput.size());
    const auto& src24 = GetFullComp8x3(isRGBFull ? 0 : 1);
    Ensures(RGB24Cnt * 3 == src24.size());
    std::vector<uint32_t> src_(RGB24Cnt);
    const auto& cvt = xziar::img::ColorConvertor::Get();
    if (isRGB)
        cvt.RGBToRGBA(src_.data(), src24.data(), src_.size());
    else
        cvt.BGRToRGBA(src_.data(), src24.data(), src_.size());
    const auto src = common::as_bytes(common::to_span(src_));
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(RGB24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint32_t, uint8_t, 1, 3>(src, [&](uint8_t* dst, const uint32_t* src, size_t count)
    {
        if (isRGB)
            intrin->RGBAToYCC(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
        else
            intrin->BGRAToYCC(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
    }, [&](uint8_t* dst, const uint32_t*, size_t count)
    {
        memcpy(dst, refOutput.data(), count * 3);
    }, [&](const HexTest<uint32_t, uint8_t>& test, std::string_view cond) noexcept
    {
        const uint32_t absY = std::abs(test.Dst.Vals[0] - test.Ref.Vals[0]),
            absCb = std::abs(test.Dst.Vals[1] - test.Ref.Vals[1]),
            absCr = std::abs(test.Dst.Vals[2] - test.Ref.Vals[2]);
        const auto suc = (absY | absCb | absCr) <= 1; // only 0 or 1 allowed
        if (test.Count == RGB24Cnt)
            mismatches[0] += absY, mismatches[1] += absCb, mismatches[2] += absCr;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: Y[{:.3f}%] Cb[{:.3f}%] Cr[{:.3f}%]\n"),
        situation, mismatches[0] * 100.0 / RGB24Cnt, mismatches[1] * 100.0 / RGB24Cnt, mismatches[2] * 100.0 / RGB24Cnt);
}
void TestRGBToAYUV(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB, const bool isFast)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::RGB8ToAYUV8" : "YCCCvt::BGR8ToAYUV8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t RGB24Cnt = std::get<2>(GetRGBRange(isRGBFull));
    const auto& refOutput_ = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), true);
    Ensures(RGB24Cnt * 3 == refOutput_.size());
    std::vector<uint32_t> refOutput(RGB24Cnt);
    const auto& cvt = xziar::img::ColorConvertor::Get();
    cvt.BGRToRGBA(refOutput.data(), refOutput_.data(), RGB24Cnt, std::byte(0xfa));
    const auto& src24 = GetFullComp8x3(isRGBFull ? 0 : 1);
    Ensures(RGB24Cnt * 3 == src24.size());
    std::vector<uint8_t> srctemp;
    common::span<const std::byte> src;
    if (isRGB)
        src = common::as_bytes(common::to_span(src24));
    else
    {
        srctemp.resize(RGB24Cnt * 3);
        cvt.RGBToBGR(srctemp.data(), src24.data(), RGB24Cnt);
        src = common::as_bytes(common::to_span(srctemp));
    }
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(RGB24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint8_t, uint32_t, 3, 1>(src, [&](uint32_t* dst, const uint8_t* src, size_t count)
    {
        if (isRGB)
            intrin->RGBToAYUV(dst, src, count, matrix, std::byte(0xfa), isRGBFull, isYCCFull, isFast);
        else
            intrin->BGRToAYUV(dst, src, count, matrix, std::byte(0xfa), isRGBFull, isYCCFull, isFast);
    }, [&](uint32_t* dst, const uint8_t*, size_t count)
    {
        memcpy(dst, refOutput.data(), count * 4);
    }, [&](const HexTest<uint8_t, uint32_t>& test, std::string_view cond) noexcept
    {
        std::array<uint8_t, 4> dst, ref;
        memcpy_s(dst.data(), 4, &test.Dst.Vals[0], 4);
        memcpy_s(ref.data(), 4, &test.Ref.Vals[0], 4);
        const uint32_t absY = std::abs(dst[2] - ref[2]),
            absCb = std::abs(dst[1] - ref[1]),
            absCr = std::abs(dst[0] - ref[0]);
        const auto suc = (absY | absCb | absCr) <= 1 && dst[3] == ref[3]; // only 0 or 1 allowed
        if (test.Count == RGB24Cnt)
            mismatches[0] += absY, mismatches[1] += absCb, mismatches[2] += absCr;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    // consider in-place, hald the mismatch count
    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: Y[{:.3f}%] Cb[{:.3f}%] Cr[{:.3f}%]\n"),
        situation, mismatches[0] * 100.0 / RGB24Cnt, mismatches[1] * 100.0 / RGB24Cnt, mismatches[2] * 100.0 / RGB24Cnt);
}
void TestRGBAToAYUV(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB, const bool isFast)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::RGBA8ToAYUV8" : "YCCCvt::BGRA8ToAYUV8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t RGB24Cnt = std::get<2>(GetRGBRange(isRGBFull));
    const auto& refOutput_ = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), true);
    Ensures(RGB24Cnt * 3 == refOutput_.size());
    std::vector<uint32_t> refOutput(RGB24Cnt);
    const auto& cvt = xziar::img::ColorConvertor::Get();
    cvt.BGRToRGBA(refOutput.data(), refOutput_.data(), RGB24Cnt, std::byte(0xc3));
    const auto& src24 = GetFullComp8x3(isRGBFull ? 0 : 1);
    Ensures(RGB24Cnt * 3 == src24.size());
    std::vector<uint32_t> src_(RGB24Cnt);
    if (isRGB)
        cvt.RGBToRGBA(src_.data(), src24.data(), src_.size(), std::byte(0xc3));
    else
        cvt.BGRToRGBA(src_.data(), src24.data(), src_.size(), std::byte(0xc3));
    const auto src = common::as_bytes(common::to_span(src_));
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(RGB24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint32_t, uint32_t, 1, 1>(src, [&](uint32_t* dst, const uint32_t* src, size_t count)
    {
        if (isRGB)
            intrin->RGBAToAYUV(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
        else
            intrin->BGRAToAYUV(dst, src, count, matrix, isRGBFull, isYCCFull, isFast);
    }, [&](uint32_t* dst, const uint32_t*, size_t count)
    {
        memcpy(dst, refOutput.data(), count * 4);
    }, [&](const HexTest<uint32_t, uint32_t>& test, std::string_view cond) noexcept
    {
        std::array<uint8_t, 4> dst, ref;
        memcpy_s(dst.data(), 4, &test.Dst.Vals[0], 4);
        memcpy_s(ref.data(), 4, &test.Ref.Vals[0], 4);
        const uint32_t absY = std::abs(dst[2] - ref[2]),
            absCb = std::abs(dst[1] - ref[1]),
            absCr = std::abs(dst[0] - ref[0]);
        const auto suc = (absY | absCb | absCr) <= 1 && dst[3] == ref[3]; // only 0 or 1 allowed
        if (test.Count == RGB24Cnt)
            mismatches[0] += absY, mismatches[1] += absCb, mismatches[2] += absCr;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    // consider in-place, hald the mismatch count
    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: Y[{:.3f}%] Cb[{:.3f}%] Cr[{:.3f}%]\n"),
        situation, mismatches[0] * 50.0 / RGB24Cnt, mismatches[1] * 50.0 / RGB24Cnt, mismatches[2] * 50.0 / RGB24Cnt);
}
void TestYCCToRGB(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::YCbCr8ToRGB8" : "YCCCvt::YCbCr8ToBGR8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t YCC24Cnt = std::get<4>(GetYCCRange(isYCCFull));
    const auto& refOutput = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), false);
    Ensures(YCC24Cnt * 3 == refOutput.size());
    const auto src = common::as_bytes(common::to_span(GetFullComp8x3(isYCCFull ? 0 : 2)));
    Ensures(YCC24Cnt * 3 == src.size());
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(YCC24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint8_t, uint8_t, 3, 3>(src, [&](uint8_t* dst, const uint8_t* src, size_t count)
    {
        if (isRGB)
            intrin->YCCToRGB(dst, src, count, matrix, isRGBFull, isYCCFull);
        else
            intrin->YCCToBGR(dst, src, count, matrix, isRGBFull, isYCCFull);
    }, [&](uint8_t* dst, const uint8_t*, size_t count)
    {
        if (isRGB)
            memcpy(dst, refOutput.data(), count * 3);
        else
            xziar::img::ColorConvertor::Get().RGBToBGR(dst, refOutput.data(), count);
    }, [&](const HexTest<uint8_t, uint8_t>& test, std::string_view cond) noexcept
    {
        const uint32_t absR = std::abs(test.Dst.Vals[0] - test.Ref.Vals[0]),
            absG = std::abs(test.Dst.Vals[1] - test.Ref.Vals[1]),
            absB = std::abs(test.Dst.Vals[2] - test.Ref.Vals[2]);
        const auto suc = (absR | absG | absB) <= 1; // only 0 or 1 allowed
        if (test.Count == YCC24Cnt)
            mismatches[0] += absR, mismatches[1] += absG, mismatches[2] += absB;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    const auto ch0 = isRGB ? 'R' : 'B', ch2 = isRGB ? 'B' : 'R';
    // consider in-place, hald the mismatch count
    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: {}[{:.3f}%] G[{:.3f}%] {}[{:.3f}%]\n"),
        situation, ch0, mismatches[0] * 50.0 / YCC24Cnt, mismatches[1] * 50.0 / YCC24Cnt, ch2, mismatches[2] * 50.0 / YCC24Cnt);
}
void TestYCCToRGBA(const std::unique_ptr<xziar::img::YCCConvertor>& intrin, const YCCMatrix matrix, const bool isRGBFull, const bool isYCCFull, const bool isRGB)
{
    std::string situation(YCCMatrixName(matrix));
    situation.append(isRGBFull ? " rFull " : " rLimit").append(isYCCFull ? " yFull " : " yLimit");
    std::string name = isRGB ? "YCCCvt::YCbCr8ToRGB8" : "YCCCvt::YCbCr8ToBGR8";
    name.append(", ").append(situation);
    const ::testing::ScopedTrace scope(__FILE__, __LINE__, name);

    const uint32_t YCC24Cnt = std::get<4>(GetYCCRange(isYCCFull));
    const auto& refOutput_ = GetYCC8Output(EncodeYCCM(matrix, isRGBFull, isYCCFull), false);
    Ensures(YCC24Cnt * 3 == refOutput_.size());
    std::vector<uint32_t> refOutput(YCC24Cnt);
    const auto& cvt = xziar::img::ColorConvertor::Get();
    if (isRGB)
        cvt.RGBToRGBA(refOutput.data(), refOutput_.data(), YCC24Cnt, std::byte(0xfa));
    else
        cvt.BGRToRGBA(refOutput.data(), refOutput_.data(), YCC24Cnt, std::byte(0xfa));
    const auto src = common::as_bytes(common::to_span(GetFullComp8x3(isYCCFull ? 0 : 2)));
    std::vector<size_t> testSizes(std::begin(TestSizes), std::end(TestSizes));
    testSizes.push_back(YCC24Cnt);
    std::array<uint32_t, 3> mismatches = { 0,0,0 };
    VarLenTest_<uint8_t, uint32_t, 3, 1>(src, [&](uint32_t* dst, const uint8_t* src, size_t count)
    {
        if (isRGB)
            intrin->YCCToRGBA(dst, src, count, matrix, std::byte(0xfa), isRGBFull, isYCCFull);
        else
            intrin->YCCToBGRA(dst, src, count, matrix, std::byte(0xfa), isRGBFull, isYCCFull);
    }, [&](uint32_t* dst, const uint8_t*, size_t count)
    {
        memcpy(dst, refOutput.data(), count * 4);
    }, [&](const HexTest<uint8_t, uint32_t>& test, std::string_view cond) noexcept
    {
        std::array<uint8_t, 4> dst, ref;
        memcpy_s(dst.data(), 4, &test.Dst.Vals[0], 4);
        memcpy_s(ref.data(), 4, &test.Ref.Vals[0], 4);
        const uint32_t absR = std::abs(dst[0] - ref[0]),
            absG = std::abs(dst[1] - ref[1]),
            absB = std::abs(dst[2] - ref[2]);
        const auto suc = (absR | absG | absB) <= 1 && dst[3] == ref[3]; // only 0 or 1 allowed
        if (test.Count == YCC24Cnt)
            mismatches[0] += absR, mismatches[1] += absG, mismatches[2] += absB;
        if (!suc)
        {
            EXPECT_THAT(test.Dst, test.Ref) << test << cond;
        }
        return suc;
    }, testSizes);

    const auto ch0 = isRGB ? 'R' : 'B', ch2 = isRGB ? 'B' : 'R';
    // consider in-place, hald the mismatch count
    TestCout() << common::str::Formatter<char>{}.FormatStatic(FmtString("[{}] SAD: {}[{:.3f}%] G[{:.3f}%] {}[{:.3f}%]\n"),
        situation, ch0, mismatches[0] * 100.0 / YCC24Cnt, mismatches[1] * 100.0 / YCC24Cnt, ch2, mismatches[2] * 100.0 / YCC24Cnt);
}

INTRIN_TEST(YCCCvt, RGB8ToYCbCr8)
{
    TestRGBYUV(&TestRGBToYCC, Intrin, false);
}
INTRIN_TEST(YCCCvt, RGB8ToYCbCr8Fast)
{
    TestRGBYUV(&TestRGBToYCC, Intrin, true);
}
INTRIN_TEST(YCCCvt, RGB8ToAYUV8)
{
    TestRGBYUV(&TestRGBToAYUV, Intrin, false);
}
INTRIN_TEST(YCCCvt, RGB8ToAYUV8Fast)
{
    TestRGBYUV(&TestRGBToAYUV, Intrin, true);
}
INTRIN_TEST(YCCCvt, RGBA8ToYCbCr8)
{
    TestRGBYUV(&TestRGBAToYCC, Intrin, false);
}
INTRIN_TEST(YCCCvt, RGBA8ToYCbCr8Fast)
{
    TestRGBYUV(&TestRGBAToYCC, Intrin, true);
}
INTRIN_TEST(YCCCvt, RGBA8ToAYUV8)
{
    TestRGBYUV(&TestRGBAToAYUV, Intrin, false);
}
INTRIN_TEST(YCCCvt, RGBA8ToAYUV8Fast)
{
    TestRGBYUV(&TestRGBAToAYUV, Intrin, true);
}
INTRIN_TEST(YCCCvt, YCbCr8ToRGB8)
{
    TestRGBYUV(&TestYCCToRGB, Intrin);
}
INTRIN_TEST(YCCCvt, YCbCr8ToRGBA8)
{
    TestRGBYUV(&TestYCCToRGBA, Intrin);
}


#if CM_DEBUG == 0


constexpr auto Cvt8_12 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint8_t*, size_t, std::byte) const>(&xziar::img::ColorConvertor::GrayToGrayA);
constexpr auto Cvt8_13 = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t * const, const uint8_t*, size_t) const>(&xziar::img::ColorConvertor::GrayToRGB);
constexpr auto Cvt8_14 = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t* const, const uint8_t*, size_t, std::byte) const>(&xziar::img::ColorConvertor::GrayToRGBA);
constexpr auto Cvt8_23 = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t * const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGB);
constexpr auto Cvt8_24 = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t* const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGBA);
constexpr auto Cvt16_12 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t, uint16_t) const>(&xziar::img::ColorConvertor::GrayToGrayA);
constexpr auto Cvt16_13 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayToRGB);
constexpr auto Cvt16_14 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t, uint16_t) const>(&xziar::img::ColorConvertor::GrayToRGBA);
constexpr auto Cvt16_23 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGB);
constexpr auto Cvt16_24 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::GrayAToRGBA);
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

TEST(IntrinPerf, G16To16)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size);
    std::vector<uint16_t> dst(Size * 4);
    PerfTester::DoFastPath(Cvt16_12, "G16ToGA16", Size, 100,
        dst.data(), src.data(), Size, (uint16_t)0x3c00);
    PerfTester::DoFastPath(Cvt16_13, "G16ToRGB16", Size, 100,
        dst.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt16_14, "G16ToRGBA16", Size, 100,
        dst.data(), src.data(), Size, (uint16_t)0x3c00);
}
TEST(IntrinPerf, GA16To16)
{
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size * 2);
    std::vector<uint16_t> dst(Size * 4);
    PerfTester::DoFastPath(Cvt16_23, "GA16ToRGB16", Size, 100,
        dst.data(), src.data(), Size);
    PerfTester::DoFastPath(Cvt16_24, "GA16ToRGBA16", Size, 100,
        dst.data(), src.data(), Size);
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

TEST(IntrinPerf, RGB16ToRGBA16)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> src(Size * 3);
    std::vector<uint16_t> dst(Size * 4);
    PerfTester tester1("RGB16ToRGBA16", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToRGBA(dst.data(), src.data(), Size);
    });
    PerfTester tester2("BGR16ToRGBA16", Size, 100);
    tester2.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.BGRToRGBA(dst.data(), src.data(), Size);
    });
}
TEST(IntrinPerf, RGBA16ToRGB16)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> src(Size * 4);
    std::vector<uint16_t> dst(Size * 3);
    PerfTester tester1("RGBA16ToRGB16", Size, 100);
    tester1.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBAToRGB(dst.data(), src.data(), Size);
    });
    PerfTester tester2("RGBA16ToBGR16", Size, 100);
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

TEST(IntrinPerf, RGB16ToBGR16)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> src(Size * 3);
    std::vector<uint16_t> dst(Size * 3);
    PerfTester tester("RGB16ToBGR16", Size, 100);
    tester.FastPathTest<xziar::img::ColorConvertor>([&](const xziar::img::ColorConvertor& host)
    {
        host.RGBToBGR(dst.data(), src.data(), Size);
    });
}

TEST(IntrinPerf, RGBA16ToBGRA16)
{
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> src(Size * 4);
    std::vector<uint16_t> dst(Size * 4);
    PerfTester tester("RGBA16ToBGRA16", Size, 100);
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

constexpr auto Ext8x3_1  = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t * const, const uint8_t *, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBGetChannel);
constexpr auto Ext16x3_1 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t* const, const uint16_t*, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBGetChannel);
constexpr auto Ext32x4_1 = static_cast<void (xziar::img::ColorConvertor::*)(float   * const, const float   *, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBAGetChannel);
constexpr auto Ext32x3_1 = static_cast<void (xziar::img::ColorConvertor::*)(float   * const, const float   *, size_t, const uint8_t) const>(&xziar::img::ColorConvertor::RGBGetChannel);

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

TEST(IntrinPerf, RGB16GetChannel)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint16_t> src(Size * 3);
    std::vector<uint16_t> dst(Size);

    PerfTester::DoFastPath(Ext16x3_1, "RGB16ToR16", Size, 100,
        dst.data(), src.data(), Size, uint8_t(0));
    PerfTester::DoFastPath(Ext16x3_1, "RGB16ToG16", Size, 100,
        dst.data(), src.data(), Size, uint8_t(1));
    PerfTester::DoFastPath(Ext16x3_1, "RGB16ToB16", Size, 100,
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


template<typename T, size_t N> using PtrSpan = common::span<T* const, N>;
template<size_t N> using PS8 = PtrSpan<uint8_t, N>;
template<size_t N> using PS16 = PtrSpan<uint16_t, N>;
template<size_t N> using PS32 = PtrSpan<float, N>;
template<size_t N> using PSC8 = PtrSpan<const uint8_t, N>;
template<size_t N> using PSC16 = PtrSpan<const uint16_t, N>;
template<size_t N> using PSC32 = PtrSpan<const float, N>;
constexpr auto Ext8x2  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <2>, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::RAToPlanar);
constexpr auto Ext8x3  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <3>, const uint8_t *, size_t) const>(&xziar::img::ColorConvertor::RGBToPlanar);
constexpr auto Ext8x4  = static_cast<void (xziar::img::ColorConvertor::*)(PS8 <4>, const uint32_t*, size_t) const>(&xziar::img::ColorConvertor::RGBAToPlanar);
constexpr auto Ext16x2 = static_cast<void (xziar::img::ColorConvertor::*)(PS16<2>, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::RAToPlanar);
constexpr auto Ext16x3 = static_cast<void (xziar::img::ColorConvertor::*)(PS16<3>, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::RGBToPlanar);
constexpr auto Ext16x4 = static_cast<void (xziar::img::ColorConvertor::*)(PS16<4>, const uint16_t*, size_t) const>(&xziar::img::ColorConvertor::RGBAToPlanar);
constexpr auto Ext32x2 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<2>, const float*, size_t) const>(&xziar::img::ColorConvertor::RAToPlanar);
constexpr auto Ext32x3 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<3>, const float*, size_t) const>(&xziar::img::ColorConvertor::RGBToPlanar);
constexpr auto Ext32x4 = static_cast<void (xziar::img::ColorConvertor::*)(PS32<4>, const float*, size_t) const>(&xziar::img::ColorConvertor::RGBAToPlanar);
constexpr auto Cmb8x2  = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t*, PSC8<2>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRA);
constexpr auto Cmb8x3  = static_cast<void (xziar::img::ColorConvertor::*)(uint8_t *, PSC8<3>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGB);
constexpr auto Cmb8x4  = static_cast<void (xziar::img::ColorConvertor::*)(uint32_t*, PSC8<4>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGBA);
constexpr auto Cmb16x2 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t*, PSC16<2>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRA);
constexpr auto Cmb16x3 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t*, PSC16<3>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGB);
constexpr auto Cmb16x4 = static_cast<void (xziar::img::ColorConvertor::*)(uint16_t*, PSC16<4>, size_t) const>(&xziar::img::ColorConvertor::PlanarToRGBA);
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

TEST(IntrinPerf, Extract16)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> src(Size * 4);
    std::vector<uint16_t> dsts[4];
    for (auto& dst : dsts)
        dst.resize(Size);
    uint16_t* const ptrs[4] = { dsts[0].data(), dsts[1].data(), dsts[2].data(), dsts[3].data() };

    PerfTester::DoFastPath(Ext16x2, "Extract16x2", Size, 100,
        PS16<2>{ ptrs, 2}, src.data(), Size);
    PerfTester::DoFastPath(Ext16x3, "Extract16x3", Size, 100,
        PS16<3>{ ptrs, 3}, src.data(), Size);
    PerfTester::DoFastPath(Ext16x4, "Extract16x4", Size, 100,
        PS16<4>{ ptrs, 4}, src.data(), Size);
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
        PS32<2>{ ptrs, 2}, src.data(), Size);
    PerfTester::DoFastPath(Ext32x3, "Extract32x3", Size, 100,
        PS32<3>{ ptrs, 3}, src.data(), Size);
    PerfTester::DoFastPath(Ext32x4, "Extract32x4", Size, 100,
        PS32<4>{ ptrs, 4}, src.data(), Size);
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

TEST(IntrinPerf, Combine16)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 512 * 512;
    std::vector<uint16_t> dst(Size * 4);
    std::vector<uint16_t> srcs[4];
    for (auto& src : srcs)
        src.resize(Size);
    uint16_t* const ptrs[4] = { srcs[0].data(), srcs[1].data(), srcs[2].data(), srcs[3].data() };

    PerfTester::DoFastPath(Cmb16x2, "Combine16x2", Size, 100,
        dst.data(), PSC16<2>{ ptrs, 2}, Size);
    PerfTester::DoFastPath(Cmb16x3, "Combine16x3", Size, 100,
        dst.data(), PSC16<3>{ ptrs, 3}, Size);
    PerfTester::DoFastPath(Cmb16x4, "Combine16x4", Size, 100,
        dst.data(), PSC16<4>{ ptrs, 4}, Size);
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

TEST(IntrinPerf, G8To16)
{
    using xziar::img::ColorConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t > dat8 (Size);
    std::vector<uint16_t> dat16(Size);

    PerfTester::DoFastPath(&ColorConvertor::Gray8To16, "G8ToG16", Size, 100,
        dat16.data(), dat8.data(), Size);
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

TEST(IntrinPerf, RGB8ToYCbCr8)
{
    using xziar::img::YCCConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src3(Size * 3);
    std::vector<uint32_t> src4(Size);
    std::vector<uint8_t> dst3(Size * 3);

    PerfTester::DoFastPath(&YCCConvertor::RGBToYCC, "RGB8ToYCbCr8", Size, 100,
        dst3.data(), src3.data(), Size, YCCMatrix::BT601, true, true, false);
    PerfTester::DoFastPath(&YCCConvertor::RGBToYCC, "RGB8ToYCbCr8Fast", Size, 100,
        dst3.data(), src3.data(), Size, YCCMatrix::BT601, true, true, true);
    PerfTester::DoFastPath(&YCCConvertor::RGBAToYCC, "RGBA8ToYCbCr8", Size, 100,
        dst3.data(), src4.data(), Size, YCCMatrix::BT601, true, true, false);
    PerfTester::DoFastPath(&YCCConvertor::RGBAToYCC, "RGBA8ToYCbCr8Fast", Size, 100,
        dst3.data(), src4.data(), Size, YCCMatrix::BT601, true, true, true);
}

TEST(IntrinPerf, RGB8ToAYUV8)
{
    using xziar::img::YCCConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src3(Size * 3);
    std::vector<uint32_t> src4(Size);
    std::vector<uint32_t> dst4(Size);

    PerfTester::DoFastPath(&YCCConvertor::RGBToAYUV, "RGB8ToAYUV8", Size, 100,
        dst4.data(), src3.data(), Size, YCCMatrix::BT601, std::byte(0xff), true, true, false);
    PerfTester::DoFastPath(&YCCConvertor::RGBToAYUV, "RGB8ToAYUV8Fast", Size, 100,
        dst4.data(), src3.data(), Size, YCCMatrix::BT601, std::byte(0xff), true, true, true);
    PerfTester::DoFastPath(&YCCConvertor::RGBAToAYUV, "RGBA8ToAYUV8", Size, 100,
        dst4.data(), src4.data(), Size, YCCMatrix::BT601, true, true, false);
    PerfTester::DoFastPath(&YCCConvertor::RGBAToAYUV, "RGBA8ToAYUV8Fast", Size, 100,
        dst4.data(), src4.data(), Size, YCCMatrix::BT601, true, true, true);
}

TEST(IntrinPerf, YCbCr8ToRGB8)
{
    using xziar::img::YCCConvertor;
    constexpr uint32_t Size = 1024 * 1024;
    std::vector<uint8_t> src(Size * 3);
    std::vector<uint8_t> dst(Size * 3);
    std::vector<uint32_t> dst4(Size);

    PerfTester::DoFastPath(&YCCConvertor::YCCToRGB, "YCbCr8ToRGB8", Size, 100,
        dst.data(), src.data(), Size, YCCMatrix::BT601, true, true);
    PerfTester::DoFastPath(&YCCConvertor::YCCToRGBA, "YCbCr8ToRGBA8", Size, 100,
        dst4.data(), src.data(), Size, YCCMatrix::BT601, std::byte(0xff), true, true);
}

#endif