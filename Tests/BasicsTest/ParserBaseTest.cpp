#include "pch.h"
#include "common/parser/ParserBase.hpp"


using namespace common::parser;
using namespace common::parser::tokenizer;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum(), BaseToken::type)

#define CHECK_TK(token, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); } while(0)

#define TO_BASETOKN_TYPE(r, dummy, i, type) BOOST_PP_COMMA_IF(i) BaseToken::type
#define EXPEND_TKTYPES(...) BOOST_PP_SEQ_FOR_EACH_I(TO_BASETOKN_TYPE, "", BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define CHECK_TKS_TYPE(types, ...) EXPECT_THAT(types, testing::ElementsAre(EXPEND_TKTYPES(__VA_ARGS__)))



