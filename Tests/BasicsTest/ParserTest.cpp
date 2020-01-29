#include "pch.h"
#include "common/parser/ParserContext.hpp"

using namespace common::parser;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)


TEST(ParserCtx, CharType)
{
    {
        constexpr auto source = U"0 A\t\n"sv;
        ParserContext context(source);
        CHECK_CHTYPE(context.GetNext(), Digit);
        CHECK_CHTYPE(context.GetNext(), Blank);
        CHECK_CHTYPE(context.GetNext(), Normal);
        CHECK_CHTYPE(context.GetNext(), Blank);
        CHECK_CHTYPE(context.GetNext(), NewLine);
        CHECK_CHTYPE(context.GetNext(), End);
        CHECK_CHTYPE(context.GetNext(), End);
    }
}


TEST(ParserCtx, NewLine)
{
    constexpr auto test = [](std::u32string_view src) 
    {
        ParserContext context(src);

        CHECK_POS(context, 0, 0);
        CHECK_CHTYPE(context.GetNext(), Digit);
        CHECK_POS(context, 0, 1);
        CHECK_CHTYPE(context.GetNext(), NewLine);

        CHECK_POS(context, 1, 0);
        CHECK_CHTYPE(context.GetNext(), Digit);
        CHECK_POS(context, 1, 1);
        CHECK_CHTYPE(context.GetNext(), Digit);
        CHECK_POS(context, 1, 2);
        CHECK_CHTYPE(context.GetNext(), NewLine);


        CHECK_POS(context, 2, 0);
        CHECK_CHTYPE(context.GetNext(), End);
        CHECK_POS(context, 2, 0);
    };
    test(U"0\n12\n"sv);
    test(U"0\r12\r"sv);
    test(U"0\r\n12\r\n"sv);
}


TEST(ParserCtx, GetLine)
{
    constexpr auto test = [](std::u32string_view src)
    {
        ParserContext context(src);

        CHECK_POS(context, 0, 0);
        const auto line0 = context.GetLine();
        EXPECT_EQ(line0, U"line0"sv);

        CHECK_POS(context, 1, 0);
        const auto line1 = context.GetLine();
        EXPECT_EQ(line1, U"line1"sv);

        CHECK_POS(context, 2, 0);
        const auto line2 = context.GetLine();
        EXPECT_EQ(line2, U"line2"sv);
       
        CHECK_POS(context, 2, 5);
        CHECK_CHTYPE(context.GetNext(), End);
        CHECK_POS(context, 2, 5);
    };
    test(U"line0\nline1\nline2"sv);
    test(U"line0\rline1\rline2"sv);
    test(U"line0\r\nline1\r\nline2"sv);
}


TEST(ParserCtx, TryGetUntil)
{
    const auto source = U"Here\nThere*/"sv;
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result = context.TryGetUntil(U"*/", true);
        EXPECT_EQ(result, U"Here\nThere*/"sv);
        CHECK_POS(context, 1, 7);
    }
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result = context.TryGetUntil(U"xx", true);
        EXPECT_EQ(result, U""sv);
        CHECK_POS(context, 0, 0);
    }
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result = context.TryGetUntil(U"*/", false);
        EXPECT_EQ(result, U""sv);
        CHECK_POS(context, 0, 0);
    }
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result0 = context.TryGetUntil(U"re", false);
        EXPECT_EQ(result0, U"Here"sv);
        CHECK_POS(context, 0, 4);

        const auto result1 = context.TryGetUntil(U"re", false);
        EXPECT_EQ(result1, U""sv);
        CHECK_POS(context, 0, 4);

        CHECK_CHTYPE(context.GetNext(), NewLine);
        CHECK_POS(context, 1, 0);
        const auto result2 = context.TryGetUntil(U"re", false);
        EXPECT_EQ(result2, U"There"sv);
        CHECK_POS(context, 1, 5);
    }
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result0 = context.TryGetUntil(U"re", false);
        EXPECT_EQ(result0, U"Here"sv);
        CHECK_POS(context, 0, 4);

        const auto result1 = context.TryGetUntil(U"re", true);
        EXPECT_EQ(result1, U"\nThere"sv);
        CHECK_POS(context, 1, 5);
    }
}

constexpr auto CheckIsBlank = [](const char32_t ch)
{
    return ch == U' ' || ch == U'\t';
};
constexpr auto CheckIsWhiteSpace = [](const char32_t ch)
{
    return ch == U' ' || ch == U'\t' || ch == '\r' || ch == '\n';
};
constexpr auto CheckIsNumber = [](const char32_t ch)
{
    return (ch >= U'0' && ch <= U'9') || (ch == U'.' || ch == U'-');
};
TEST(ParserCtx, TryGetWhile)
{
    const auto source = U" 123\t456\n789"sv;
    {
        ParserContext context(source);

        CHECK_POS(context, 0, 0);
        const auto result0 = context.TryGetWhile(CheckIsBlank);
        EXPECT_EQ(result0, U" "sv);
        CHECK_POS(context, 0, 1);

        const auto result1 = context.TryGetWhile(CheckIsNumber);
        EXPECT_EQ(result1, U"123"sv);
        CHECK_POS(context, 0, 4);

        const auto result2 = context.TryGetWhile(CheckIsBlank);
        EXPECT_EQ(result2, U"\t"sv);
        CHECK_POS(context, 0, 5);

        const auto result3 = context.TryGetWhile(CheckIsNumber);
        EXPECT_EQ(result3, U"456"sv);
        CHECK_POS(context, 0, 8);

        const auto result4 = context.TryGetWhile<false>(CheckIsWhiteSpace);
        EXPECT_EQ(result4, U""sv);
        CHECK_POS(context, 0, 8);

        const auto result5 = context.TryGetWhile<true>(CheckIsWhiteSpace);
        EXPECT_EQ(result5, U"\n"sv);
        CHECK_POS(context, 1, 0);

        const auto result6 = context.TryGetWhile(CheckIsNumber);
        EXPECT_EQ(result6, U"789"sv);
        CHECK_POS(context, 1, 3);
    }
}


TEST(ParserCtx, ParseFunc)
{
    constexpr auto test = [](std::u32string_view src) -> std::pair<std::u32string_view, std::vector<std::u32string_view>>
    {
        std::vector<std::u32string_view> args;
        ParserContext context(src);
        context.TryGetWhile(CheckIsWhiteSpace);
        const auto fname = context.TryGetWhile<false>([](const char32_t ch)
            {
                return !CheckIsWhiteSpace(ch) && U"()'\"<>[]{}/\\,.?|+-*=&^%$#@!~`"sv.find(ch) == std::u32string_view::npos;
            });
        context.TryGetWhile([](const char32_t ch) { return ch != '('; });
        if (context.GetNext() == '(')
        {
            while (true)
            {
                context.TryGetWhile(CheckIsWhiteSpace);
                const auto arg = context.TryGetWhile<false>([](const char32_t ch)
                    {
                        return !CheckIsWhiteSpace(ch) && U"),"sv.find(ch) == std::u32string_view::npos;
                    });
                context.TryGetWhile(CheckIsWhiteSpace);
                const auto next = context.GetNext();
                switch (next)
                {
                case ')':
                    args.emplace_back(arg);
                    break;
                case ',':
                    args.emplace_back(arg);
                    continue;
                case ParserContext::CharEnd:
                    EXPECT_NE(next, ParserContext::CharEnd);
                    return { };
                default:
                    EXPECT_TRUE(false);
                    return {};
                }
                break;
            }
        }
        return { fname, args };
    };
    {
        const auto ret = test(U"func(foo)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U" \tfunc(foo)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U" \tfunc \t(foo)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U" \tfunc \t\n(foo)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U" \tfunc \t( \t\nfoo)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U" \tfunc \t( \tfoo \t\n)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"foo"sv));
    }
    {
        const auto ret = test(U"func (arg0,arg1)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"arg0"sv, U"arg1"sv));
    }
    {
        const auto ret = test(U"func (arg0,\t\n\targ1)"sv);
        EXPECT_EQ(ret.first, U"func"sv);
        EXPECT_THAT(ret.second, testing::ElementsAre(U"arg0"sv, U"arg1"sv));
    }
}

