#include "pch.h"
#include "common/StringLinq.hpp"
#include "common/AlignedBuffer.hpp"

using namespace common::str;
using namespace common::linq;


TEST(Split, SplitSource)
{
    {
        [[maybe_unused]] const auto stream = SplitStream("a", [](const char ch) { return ch == ','; });
    }
    {
        const std::string_view sv = "a";
        [[maybe_unused]] const auto stream = SplitStream(sv, [](const char ch) { return ch == ','; });
    }
    {
        const std::u16string_view sv = u"a";
        [[maybe_unused]] const auto stream = SplitStream(sv, [](const char16_t ch) { return ch == ','; });
    }
    {
        const std::u16string str = u"a";
        [[maybe_unused]] const auto stream = SplitStream(str, [](const char16_t ch) { return ch == ','; });
    }
    {
        const common::SharedString<char> str = "a";
        [[maybe_unused]] const auto stream = SplitStream(str, [](const char16_t ch) { return ch == ','; });
    }
    {
        const std::vector<char> str{ 'a' };
        [[maybe_unused]] const auto stream = SplitStream(str, [](const char16_t ch) { return ch == ','; });
    }
    {
        const common::AlignedBuffer buf(1);
        [[maybe_unused]] const auto stream = SplitStream(buf.AsSpan<char>(), [](const char ch) { return ch == ','; });
    }
}


constexpr bool Judger1(const char ch) noexcept { return ch == ','; }
struct Judger2
{
    constexpr bool operator()(const char ch) noexcept { return ch == ','; }
};
template<typename T>
constexpr bool Judger3(const T ch) noexcept { return ch == ','; }
struct Judger4
{
    template<typename T>
    constexpr bool operator()(const T ch) noexcept { return ch == ','; }
};

TEST(Split, SplitJudger)
{
    {
        [[maybe_unused]] const auto stream = SplitStream("a", [](const char ch) { return ch == ','; });
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", ',');
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", Judger1);
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", Judger2{});
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", [](const auto ch) { return ch == ','; });
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", Judger3<char>);
    }
    {
        [[maybe_unused]] const auto stream = SplitStream("a", Judger4{});
    }
}


TEST(Split, Split)
{
    const auto kk = SplitStream(",a", ',', false).ToVector();


    EXPECT_THAT(SplitStream("a",   ',',  true).ToVector(), testing::ElementsAre("a"));
    EXPECT_THAT(SplitStream("a",   ',', false).ToVector(), testing::ElementsAre("a"));
                                   
    EXPECT_THAT(SplitStream("a,",  ',',  true).ToVector(), testing::ElementsAre("a", ""));
    EXPECT_THAT(SplitStream("a,",  ',', false).ToVector(), testing::ElementsAre("a"));
                                   
    EXPECT_THAT(SplitStream(",a",  ',',  true).ToVector(), testing::ElementsAre("", "a"));
    EXPECT_THAT(SplitStream(",a",  ',', false).ToVector(), testing::ElementsAre("a"));
                                   
    EXPECT_THAT(SplitStream("a,a", ',',  true).ToVector(), testing::ElementsAre("a", "a"));
    EXPECT_THAT(SplitStream("a,a", ',', false).ToVector(), testing::ElementsAre("a", "a"));

    EXPECT_THAT(SplitStream("ab,a t,,", ',',  true).ToVector(), testing::ElementsAre("ab", "a t", "", ""));
    EXPECT_THAT(SplitStream("ab,a t,,", ',', false).ToVector(), testing::ElementsAre("ab", "a t"));
}


TEST(Split, Mutable)
{
    std::string str = "a,b,c";
    const auto parts = SplitStream(str, ',', true).ToVector();
    EXPECT_THAT(parts, testing::ElementsAre("a", "b", "c"));
    str[0] = 'c', str[2] = 'a', str[4] = 'b';
    EXPECT_THAT(parts, testing::ElementsAre("c", "a", "b"));
}

