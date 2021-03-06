#include "rely.h"
#include "ParserCommon.h"
#include "common/parser/ParserLexer.hpp"
#include "common/Linq2.hpp"


using namespace common::parser;
using namespace common::parser::tokenizer;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum(), BaseToken::type)

//#define CHECK_TK(token, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); } while(0)
#define CHECK_TK(token, row, col, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); EXPECT_EQ(token.Row, row); EXPECT_EQ(token.Col, col); } while(0)

#define MAP_TOKENS(tokens, action) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.action; }).ToVector()
#define MAP_TOKEN_TYPES(tokens) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.GetIDEnum(); }).ToVector()


TEST(ParserLexer, ASCIIChecker)
{
    std::string str;
    for (uint32_t i = 0; i < 128; i += 2)
        str.push_back(static_cast<char>(i));
    ASCIIChecker checker(str);

    for (uint16_t i = 0; i < 128; i++)
        EXPECT_EQ(checker(i), (i & 1) == 0 ? true : false);

    for (uint16_t i = 128; i < 512; i++)
        EXPECT_EQ(checker(i), false);
}


TEST(ParserLexer, LexerComment)
{
    {
        const auto tokens = TKParse<CommentTokenizer>(U"//hello"sv);
        CHECK_TK(tokens[0], 0u, 0u, Comment, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello*/"sv);
        CHECK_TK(tokens[0], 0u, 0u, Comment, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"//hello there"sv);
        CHECK_TK(tokens[0], 0u, 0u, Comment, GetString, U"hello there"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello\nthere*/"sv);
        CHECK_TK(tokens[0], 0u, 0u, Comment, GetString, U"hello\nthere"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/* hello\nthere */\n// hello"sv);
        CHECK_TK(tokens[0], 0u, 0u, Comment, GetString, U" hello\nthere "sv);
        CHECK_TK(tokens[1], 2u, 0u, Comment, GetString, U" hello"sv);
    }
}


TEST(ParserLexer, LexerString)
{

    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello")"sv);
        CHECK_TK(tokens[0], 0u, 0u, String, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello there")"sv);
        CHECK_TK(tokens[0], 0u, 0u, String, GetString, U"hello there"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello\"there")"sv);
        CHECK_TK(tokens[0], 0u, 0u, String, GetString, UR"(hello\"there)"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello)"sv);
        CHECK_TK(tokens[0], 0u, 0u, Error,  GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello" " " "there")"sv);
        CHECK_TK(tokens[0], 0u, 0u,  String, GetString, U"hello"sv);
        CHECK_TK(tokens[1], 0u, 8u,  String, GetString, U" "sv);
        CHECK_TK(tokens[2], 0u, 12u, String, GetString, U"there"sv);
    }
}


TEST(ParserLexer, LexerInt)
{
    {
        const auto tokens = TKParse<IntTokenizer>(U"123"sv);
        CHECK_TK(tokens[0], 0u, 0u, Int, GetInt, 123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-123"sv);
        CHECK_TK(tokens[0], 0u, 0u, Int, GetInt, -123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"123u"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, 123u);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-123u"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, static_cast<uint64_t>(-123));
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x13579bdf"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, 0x13579bdfu);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b1010010100111110"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, 0b1010010100111110u);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0"sv);
        CHECK_TK(tokens[0], 0u, 0u, Int, GetInt, 0);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0X12"sv);
        CHECK_TK(tokens[0], 0u, 0u, Int, GetInt, 0); // max-match
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b123"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, 0b1u); // max-match
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x1ax3"sv);
        CHECK_TK(tokens[0], 0u, 0u, Uint, GetUInt, 0x1au); // max-match
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x12345678901234567890"sv);
        CHECK_TK(tokens[0], 0u, 0u, Error, GetString, U"0x12345678901234567890"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
        CHECK_TK(tokens[0], 0u, 0u, Error, GetString, U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
    }
}


TEST(ParserLexer, LexerFP)
{
    {
        const auto tokens = TKParse<FPTokenizer>(U"123"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"123.0"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-123.0"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, -123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U".5"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, .5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-.5"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, -.5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e5"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, 1e5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1.5E-2"sv);
        CHECK_TK(tokens[0], 0u, 0u, FP, GetDouble, 1.5e-2);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"."sv);
        CHECK_TK(tokens[0], 0u, 0u, Unknown, GetString, U"."sv);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"--123"sv);
        CHECK_TK(tokens[0], 0u, 0u, Unknown, GetString, U"-"sv); // fast-stop
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e.5"sv);
        CHECK_TK(tokens[0], 0u, 0u, Unknown, GetString, U"1e"sv); // fast-stop
    }
}


TEST(ParserLexer, LexerBool)
{
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TRUE"sv);
        CHECK_TK(tokens[0], 0u, 0u, Bool, GetBool, true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"false"sv);
        CHECK_TK(tokens[0], 0u, 0u, Bool, GetBool, false);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TrUe"sv);
        CHECK_TK(tokens[0], 0u, 0u, Bool, GetBool, true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"Fals3"sv);
        CHECK_TK(tokens[0], 0u, 0u, Unknown, GetString, U"Fals"sv); // fast-stop
    }
}


TEST(ParserLexer, LexerASCIIRaw)
{
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"func"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu1nc"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"1func"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"1func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"fu_nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc("sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"fu_nc"sv); // fast-stop
    }
}


TEST(ParserLexer, LexerASCII2Part)
{
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"func"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"fu1nc"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"1func"sv);
        CHECK_TK(tokens[0], 0u, 0u, Unknown, GetString, U"1"sv);
    }
}


TEST(ParserLexer, LexerDelim)
{
    {
        const auto tokens = TKParse<DelimTokenizer, ASCIIRawTokenizer>(U"func"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw, GetString, U"func"sv);
    }
    {
        const auto tokens = TKParse<DelimTokenizer, ASCIIRawTokenizer>(U"func,foo"sv);
        CHECK_TK(tokens[0], 0u, 0u, Raw,    GetString,  U"func"sv);
        CHECK_TK(tokens[1], 0u, 4u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], 0u, 5u, Raw,    GetString,  U"foo"sv);
    }
    {
        const auto tokens = TKParse<DelimTokenizer, ASCIIRawTokenizer>(U"func,foo,,bar"sv);
        CHECK_TK(tokens[0], 0u, 0u,  Raw,    GetString,  U"func"sv);
        CHECK_TK(tokens[1], 0u, 4u,  Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], 0u, 5u,  Raw,    GetString,  U"foo"sv);
        CHECK_TK(tokens[3], 0u, 8u,  Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], 0u, 9u,  Delim,  GetChar,    U',');
        CHECK_TK(tokens[5], 0u, 10u, Raw,    GetString,  U"bar"sv);
    }
}


TEST(ParserLexer, LexerArgs)
{
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        return TKParse<DelimTokenizer, CommentTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer>(src);
    };
    {
        const auto tokens = ParseAll(UR"(123,456,789)"sv);
        CHECK_TK(tokens[0], 0u, 0u, Int,   GetInt,  123);
        CHECK_TK(tokens[1], 0u, 3u, Delim, GetChar, U',');
        CHECK_TK(tokens[2], 0u, 4u, Int,   GetInt,  456);
        CHECK_TK(tokens[3], 0u, 7u, Delim, GetChar, U',');
        CHECK_TK(tokens[4], 0u, 8u, Int,   GetInt,  789);
    }
    {
        const auto tokens = ParseAll(UR"(-123,"hello",3.5,0xff)"sv);
        CHECK_TK(tokens[0], 0u, 0u,  Int,    GetInt,     -123);
        CHECK_TK(tokens[1], 0u, 4u,  Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], 0u, 5u,  String, GetString,  U"hello"sv);
        CHECK_TK(tokens[3], 0u, 12u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], 0u, 13u, FP,     GetDouble,  3.5);
        CHECK_TK(tokens[5], 0u, 16u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[6], 0u, 17u, Uint,   GetUInt,    0xffu);
    }
    {
        const auto tokens = ParseAll(UR"(-123, "hello",3.5 ,0xff)"sv);
        CHECK_TK(tokens[0], 0u, 0u,  Int,    GetInt,     -123);
        CHECK_TK(tokens[1], 0u, 4u,  Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], 0u, 6u,  String, GetString,  U"hello"sv);
        CHECK_TK(tokens[3], 0u, 13u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], 0u, 14u, FP,     GetDouble,  3.5);
        CHECK_TK(tokens[5], 0u, 18u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[6], 0u, 19u, Uint,   GetUInt,    0xffu);
    }
}


TEST(ParserLexer, LexerFunc)
{
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        constexpr ASCII2PartTokenizer MetaFuncTK(BaseToken::Raw, 1,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_@"sv,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"sv);
        
        constexpr ParserLexerBase<DelimTokenizer, CommentTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, ASCII2PartTokenizer> Lexer(MetaFuncTK);
        return TKParse_(src, Lexer);
    };
    
    {
        const auto tokens = ParseAll(UR"(@func(-123, "hello",3.5 ,0xff))"sv);
        CHECK_TK(tokens[0], 0u, 0u,  Raw,    GetString,  U"@func"sv);
        CHECK_TK(tokens[1], 0u, 5u,  Delim,  GetChar,    U'(');
        CHECK_TK(tokens[2], 0u, 6u,  Int,    GetInt,     -123);
        CHECK_TK(tokens[3], 0u, 10u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], 0u, 12u, String, GetString,  U"hello"sv);
        CHECK_TK(tokens[5], 0u, 19u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[6], 0u, 20u, FP,     GetDouble,  3.5);
        CHECK_TK(tokens[7], 0u, 24u, Delim,  GetChar,    U',');
        CHECK_TK(tokens[8], 0u, 25u, Uint,   GetUInt,    0xffu);
        CHECK_TK(tokens[9], 0u, 29u, Delim,  GetChar,    U')');
    }
}

