#include "pch.h"
#include "common/parser/ParserContext.hpp"

using namespace common::parser;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::SingleChar::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col);


TEST(ParserCtx, CharType)
{
    {
        constexpr auto source = U"0 A\t\n"sv;
        ParserContext context(source);
        EXPECT_EQ(context.GetNext().Type, CharType::Digit);
        EXPECT_EQ(context.GetNext().Type, CharType::WhiteSpace);
        EXPECT_EQ(context.GetNext().Type, CharType::Normal);
        EXPECT_EQ(context.GetNext().Type, CharType::WhiteSpace);
        EXPECT_EQ(context.GetNext().Type, CharType::NewLine);
        EXPECT_EQ(context.GetNext().Type, CharType::End);
        EXPECT_EQ(context.GetNext().Type, CharType::End);
    }
}


TEST(ParserCtx, NewLine)
{
    constexpr auto test = [](std::u32string_view src) 
    {
        ParserContext context(src);

        CHECK_POS(context, 0, 0);
        EXPECT_EQ(context.GetNext().Type, CharType::Digit);
        CHECK_POS(context, 0, 1);
        EXPECT_EQ(context.GetNext().Type, CharType::NewLine);

        CHECK_POS(context, 1, 0);
        EXPECT_EQ(context.GetNext().Type, CharType::Digit);
        CHECK_POS(context, 1, 1);
        EXPECT_EQ(context.GetNext().Type, CharType::Digit);
        CHECK_POS(context, 1, 2);
        EXPECT_EQ(context.GetNext().Type, CharType::NewLine);


        CHECK_POS(context, 2, 0);
        EXPECT_EQ(context.GetNext().Type, CharType::End);
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
        EXPECT_EQ(context.GetNext().Type, CharType::End);
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

        EXPECT_EQ(context.GetNext().Type, CharType::NewLine);
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

//
//TEST(ParserCtx, ParseFunc)
//{
//    constexpr auto test = [](std::u32string_view src)
//    {
//        ParserContext context(src);
//        context.TryGetUntilNot(U"\t ", false);
//
//        context.TryGetUntilAny(U"\t (", false);
//    };

//}

