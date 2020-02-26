#include "rely.h"
#include "common/parser/SectorFile/ParserRely.h"
#include "common/parser/SectorFile/SectorsParser.h"
#include "common/parser/SectorFile/FuncParser.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;
using xziar::sectorlang::SectorsParser;



#define CHECK_TK(token, etype, type, action, val) do { EXPECT_EQ(token.GetIDEnum<etype>(), etype::type); EXPECT_EQ(token.action(), val); } while(0)
#define CHECK_BASE_TK(token, type, action, val) CHECK_TK(token, common::parser::BaseToken, type, action, val)

TEST(SectorFileBase, EmbedOpTokenizer)
{
    using common::parser::BaseToken;
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        using namespace common::parser;
        constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
        constexpr auto lexer = ParserLexerBase<xziar::sectorlang::tokenizer::EmbedOpTokenizer, tokenizer::IntTokenizer>();
        ParserContext context(src);
        std::vector<ParserToken> tokens;
        while (true)
        {
            const auto token = lexer.GetTokenBy(context, ignore);
            const auto type = token.GetIDEnum();
            if (type != BaseToken::End)
                tokens.emplace_back(token);
            if (type == BaseToken::Error || type == BaseToken::Unknown || type == BaseToken::End)
                break;
        }
        return tokens;
    };
#define CHECK_BASE_UINT(token, val) CHECK_BASE_TK(token, Uint, GetUInt, val)
#define CHECK_EMBED_OP(token, type) CHECK_TK(token, xziar::sectorlang::tokenizer::SectorLangToken, EmbedOp, GetUInt, common::enum_cast(xziar::sectorlang::EmbedOps::type))

#define CHECK_BIN_OP(src, type) do \
    {                                       \
        const auto tokens = ParseAll(src);  \
        CHECK_BASE_UINT(tokens[0], 1);      \
        CHECK_EMBED_OP(tokens[1], type);    \
        CHECK_BASE_UINT(tokens[2], 2);      \
    } while(0)                              \


    CHECK_BIN_OP(U"1 == 2"sv, Equal);
    CHECK_BIN_OP(U"1 != 2"sv, NotEqual);
    CHECK_BIN_OP(U"1 < 2"sv,  Less);
    CHECK_BIN_OP(U"1 <= 2"sv, LessEqual);
    CHECK_BIN_OP(U"1 > 2"sv,  Greater);
    CHECK_BIN_OP(U"1 >= 2"sv, GreaterEqual);
    CHECK_BIN_OP(U"1 && 2"sv, And);
    CHECK_BIN_OP(U"1 || 2"sv, Or);
    CHECK_BIN_OP(U"1 + 2"sv,  Add);
    CHECK_BIN_OP(U"1 - 2"sv,  Sub);
    CHECK_BIN_OP(U"1 * 2"sv,  Mul);
    CHECK_BIN_OP(U"1 / 2"sv,  Div);
    CHECK_BIN_OP(U"1 % 2"sv,  Rem);
    {
        const auto tokens = ParseAll(U"! 1"sv);
        CHECK_EMBED_OP(tokens[0], Not);
        CHECK_BASE_UINT(tokens[1], 1);
    }
    {
        const auto tokens = ParseAll(U"1 += 2"sv);
        CHECK_BASE_UINT(tokens[0], 1);
        //CHECK_BASE_TK(tokens[1], Unknown, GetString, U"+="sv);
    }
}

