#include "rely.h"
#include "common/parser/ParserContext.hpp"

using namespace common::parser;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row##u); EXPECT_EQ(ctx.Col, col##u)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)


TEST(ParserCtx, CharType)
{
    {
        constexpr auto source = U"0 A\t\n"sv;
        ParserContext context(source);
        ContextReader reader(context);

        CHECK_CHTYPE(reader.ReadNext(), Digit);
        CHECK_CHTYPE(reader.ReadNext(), Blank);
        CHECK_CHTYPE(reader.ReadNext(), Normal);
        CHECK_CHTYPE(reader.ReadNext(), Blank);
        CHECK_CHTYPE(reader.ReadNext(), NewLine);
        CHECK_CHTYPE(reader.ReadNext(), End);
        CHECK_CHTYPE(reader.ReadNext(), End);
    }
}


TEST(ParserCtx, Reader)
{
    {
        constexpr auto source = U"01\n\r\n"sv;
        ParserContext context(source);
        CHECK_POS(context, 0, 0);
        {
            ContextReader reader(context);
            EXPECT_EQ(reader.PeekNext(), U'0');
            CHECK_POS(context, 0, 0);
            EXPECT_EQ(reader.PeekNext(), U'0');
            CHECK_POS(context, 0, 0);
            reader.MoveNext();
            EXPECT_EQ(reader.PeekNext(), U'1');
            CHECK_POS(context, 0, 0);
            reader.MoveNext();
            EXPECT_EQ(reader.CommitRead(), U"01"sv);
            CHECK_POS(context, 0, 2);
            EXPECT_EQ(context.Index, 2u);
        }
        {
            ContextReader reader(context);
            EXPECT_EQ(reader.PeekNext(), U'\n');
            reader.MoveNext();
            EXPECT_EQ(reader.PeekNext(), U'\n');
            reader.MoveNext();
            EXPECT_EQ(reader.CommitRead(), U"\n\r\n"sv);
            CHECK_POS(context, 2, 0);
            EXPECT_EQ(context.Index, 5u);
        }
        {
            ContextReader reader(context);
            EXPECT_EQ(reader.PeekNext(), special::CharEnd);
            reader.MoveNext();
            reader.CommitRead();
            CHECK_POS(context, 2, 0);
            EXPECT_EQ(context.Index, 5u);
        }
    }
}


TEST(ParserCtx, NewLine)
{
    constexpr auto test = [](std::u32string_view src) 
    {
        ParserContext context(src);
        ContextReader reader(context);

        CHECK_POS(context, 0, 0);
        CHECK_CHTYPE(reader.ReadNext(), Digit);
        CHECK_POS(context, 0, 1);
        CHECK_CHTYPE(reader.ReadNext(), NewLine);

        CHECK_POS(context, 1, 0);
        CHECK_CHTYPE(reader.ReadNext(), Digit);
        CHECK_POS(context, 1, 1);
        CHECK_CHTYPE(reader.ReadNext(), Digit);
        CHECK_POS(context, 1, 2);
        CHECK_CHTYPE(reader.ReadNext(), NewLine);


        CHECK_POS(context, 2, 0);
        CHECK_CHTYPE(reader.ReadNext(), End);
        CHECK_POS(context, 2, 0);
    };
    test(U"0\n12\n"sv);
    test(U"0\r12\r"sv);
    test(U"0\r\n12\r\n"sv);
}


TEST(ParserCtx, ReadLine)
{
    constexpr auto test = [](std::u32string_view src)
    {
        ParserContext context(src);
        ContextReader reader(context);

        CHECK_POS(context, 0, 0);
        const auto line0 = reader.ReadLine();
        EXPECT_EQ(line0, U"line0"sv);

        CHECK_POS(context, 1, 0);
        const auto line1 = reader.ReadLine();
        EXPECT_EQ(line1, U"line1"sv);

        CHECK_POS(context, 2, 0);
        const auto line2 = reader.ReadLine();
        EXPECT_EQ(line2, U"line2"sv);
       
        CHECK_POS(context, 2, 5);
        CHECK_CHTYPE(reader.ReadNext(), End);
        CHECK_POS(context, 2, 5);
    };
    test(U"line0\nline1\nline2"sv);
    test(U"line0\rline1\rline2"sv);
    test(U"line0\r\nline1\r\nline2"sv);
}


TEST(ParserCtx, ReadUntil)
{
    const auto source = U"Here\nThere*/"sv;
    {
        ParserContext context(source);
        ContextReader reader(context);

        CHECK_POS(context, 0, 0);
        const auto result = reader.ReadUntil(U"*/");
        EXPECT_EQ(result, U"Here\nThere*/"sv);
        CHECK_POS(context, 1, 7);
    }
    {
        ParserContext context(source);
        ContextReader reader(context);

        CHECK_POS(context, 0, 0);
        const auto result = reader.ReadUntil(U"xx");
        EXPECT_EQ(result, U""sv);
        CHECK_POS(context, 0, 0);
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
TEST(ParserCtx, ReadWhile)
{
    const auto source = U" 123\t456\n789\r\n"sv;
    {
        ParserContext context(source);
        ContextReader reader(context);

        CHECK_POS(context, 0, 0);
        const auto result0 = reader.ReadWhile(CheckIsBlank);
        EXPECT_EQ(result0, U" "sv);
        EXPECT_EQ(context.Index, 1u);
        CHECK_POS(context, 0, 1);

        const auto result1 = reader.ReadWhile(CheckIsNumber);
        EXPECT_EQ(result1, U"123"sv);
        EXPECT_EQ(context.Index, 4u);
        CHECK_POS(context, 0, 4);

        const auto result2 = reader.ReadWhile(CheckIsBlank);
        EXPECT_EQ(result2, U"\t"sv);
        EXPECT_EQ(context.Index, 5u);
        CHECK_POS(context, 0, 5);

        const auto result3 = reader.ReadWhile(CheckIsNumber);
        EXPECT_EQ(result3, U"456"sv);
        EXPECT_EQ(context.Index, 8u);
        CHECK_POS(context, 0, 8);

        const auto result5 = reader.ReadWhile(CheckIsWhiteSpace);
        EXPECT_EQ(result5, U"\n"sv);
        EXPECT_EQ(context.Index, 9u);
        CHECK_POS(context, 1, 0);

        const auto result6 = reader.ReadWhile(CheckIsNumber);
        EXPECT_EQ(result6, U"789"sv);
        EXPECT_EQ(context.Index, 12u);
        CHECK_POS(context, 1, 3);

        const auto result7 = reader.ReadWhile([](const char32_t ch) { return ch == U'\n'; });
        EXPECT_EQ(result7, U"\r\n"sv);
        EXPECT_EQ(context.Index, 14u);
        CHECK_POS(context, 2, 0);
    }
}


TEST(ParserCtx, ParseFunc)
{
    constexpr auto test = [](std::u32string_view src) -> std::pair<std::u32string_view, std::vector<std::u32string_view>>
    {
        std::vector<std::u32string_view> args;
        ParserContext context(src);
        ContextReader reader(context);
        reader.ReadWhile(CheckIsWhiteSpace);
        const auto fname = reader.ReadWhile([](const char32_t ch)
            {
                return !CheckIsWhiteSpace(ch) && U"()'\"<>[]{}/\\,.?|+-*=&^%$#@!~`"sv.find(ch) == std::u32string_view::npos;
            });
        reader.ReadWhile([](const char32_t ch) { return ch != '('; });
        if (reader.ReadNext() == '(')
        {
            while (true)
            {
                reader.ReadWhile(CheckIsWhiteSpace);
                const auto arg = reader.ReadWhile([](const char32_t ch)
                    {
                        return !CheckIsWhiteSpace(ch) && U"),"sv.find(ch) == std::u32string_view::npos;
                    });
                reader.ReadWhile(CheckIsWhiteSpace);
                const auto next = reader.ReadNext();
                switch (next)
                {
                case ')':
                    args.emplace_back(arg);
                    break;
                case ',':
                    args.emplace_back(arg);
                    continue;
                case special::CharEnd:
                    EXPECT_NE(next, special::CharEnd);
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

