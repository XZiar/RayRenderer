#include "rely.h"
#include <algorithm>
#include "Nailang/ParserRely.h"
#include "Nailang/NailangParser.h"
#include "3rdParty/fmt/utfext.h"


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;



#define CHECK_TK(token, etype, type, action, val) do { EXPECT_EQ(token.GetIDEnum<etype>(), etype::type); EXPECT_EQ(token.action(), val); } while(0)
#define CHECK_BASE_TK(token, type, action, val) CHECK_TK(token, common::parser::BaseToken, type, action, val)


static std::string PrintMemPool(const xziar::nailang::MemoryPool& pool) noexcept
{
    const auto [used, total] = pool.Usage();
    std::string txt = fmt::format("MemoryPool[{} trunks(default {} bytes)]: [{}/{}]\n", 
        pool.Trunks.size(), pool.TrunkSize, used, total);
    auto ins = std::back_inserter(txt);
    size_t i = 0;
    for (const auto& [ptr, offset, avaliable] : pool.Trunks)
    {
        fmt::format_to(ins, "[{}] ptr[{}], offset[{}], avaliable[{}]\n", i++, (void*)ptr, offset, avaliable);
    }
    return txt;
}

TEST(NailangBase, MemoryPool)
{
#define EXPECT_ALIGN(ptr, align) EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % align, 0u)
    xziar::nailang::MemoryPool pool;
    {
        const auto space = pool.Alloc(10, 1);
        std::generate(space.begin(), space.end(), [i = 0]() mutable { return std::byte(i++); });
        {
            int i = 0;
            for (const auto dat : space)
                EXPECT_EQ(dat, std::byte(i++));
        }
    }
    {
        const auto space = pool.Alloc(4, 128);
        EXPECT_ALIGN(space.data(), 128) << PrintMemPool(pool);
    }
    {
        const auto& arr = *pool.Create<std::array<double, 3>>(std::array{ 1.0,2.0,3.0 });
        EXPECT_ALIGN(&arr, alignof(double)) << PrintMemPool(pool);
        EXPECT_THAT(arr, testing::ElementsAre(1.0, 2.0, 3.0)) << PrintMemPool(pool);
    }
    {
        const std::array arr{ 1.0,2.0,3.0 };
        const auto sp = pool.CreateArray(arr);
        EXPECT_ALIGN(sp.data(), alignof(double)) << PrintMemPool(pool);
        std::vector<double> vec(sp.begin(), sp.end());
        EXPECT_THAT(vec, testing::ElementsAre(1.0, 2.0, 3.0)) << PrintMemPool(pool);
    }
    {
        const auto space = pool.Alloc(3 * 1024 * 1024, 4096);
        EXPECT_ALIGN(space.data(), 4096) << PrintMemPool(pool);
        const auto space2 = pool.Alloc(128, 1);
        EXPECT_EQ(space2.data() - space.data(), 3 * 1024 * 1024) << PrintMemPool(pool);
    }
    {
        const auto space = pool.Alloc(36 * 1024 * 1024, 8192);
        EXPECT_ALIGN(space.data(), 4096) << PrintMemPool(pool);
    }
#undef EXPECT_ALIGN
}

TEST(NailangBase, EmbedOpTokenizer)
{
    using common::parser::BaseToken;
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        using namespace common::parser;
        constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
        constexpr auto lexer = ParserLexerBase<xziar::nailang::tokenizer::EmbedOpTokenizer, tokenizer::IntTokenizer>();
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
#define CHECK_BASE_INT(token, val) CHECK_BASE_TK(token, Int, GetInt, val)
#define CHECK_EMBED_OP(token, type) CHECK_TK(token, xziar::nailang::tokenizer::SectorLangToken, EmbedOp, GetInt, common::enum_cast(xziar::nailang::EmbedOps::type))

#define CHECK_BIN_OP(src, type) do          \
    {                                       \
        const auto tokens = ParseAll(src);  \
        CHECK_BASE_INT(tokens[0], 1);       \
        CHECK_EMBED_OP(tokens[1], type);    \
        CHECK_BASE_INT(tokens[2], 2);       \
    } while(0)                              \


    CHECK_BIN_OP(U"1==2"sv,   Equal);
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
        const auto tokens = ParseAll(U"!1"sv);
        CHECK_EMBED_OP(tokens[0], Not);
        CHECK_BASE_INT(tokens[1], 1);
    }
    {
        const auto tokens = ParseAll(U"1+=2"sv);
        CHECK_BASE_INT(tokens[0], 1);
        //CHECK_BASE_TK(tokens[1], Unknown, GetString, U"+="sv);
    }
#undef CHECK_BIN_OP
#undef CHECK_EMBED_OP
#undef CHECK_BASE_UINT
}


TEST(NailangBase, AssignOpTokenizer)
{
    using common::parser::BaseToken;
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        using namespace common::parser;
        constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
        constexpr auto lexer = ParserLexerBase<xziar::nailang::tokenizer::AssignOpTokenizer, tokenizer::IntTokenizer>();
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
#define CHECK_BASE_INT(token, val) CHECK_BASE_TK(token, Int, GetInt, val)
#define CHECK_ASSIGN_OP(token, type) CHECK_TK(token, xziar::nailang::tokenizer::SectorLangToken, Assign, GetInt, common::enum_cast(xziar::nailang::tokenizer::AssignOps::type))

#define CHECK_ASSIGN(src, type) do          \
    {                                       \
        const auto tokens = ParseAll(src);  \
        CHECK_BASE_INT(tokens[0], 1);       \
        CHECK_ASSIGN_OP(tokens[1], type);   \
        CHECK_BASE_INT(tokens[2], 2);       \
    } while(0)                              \


    CHECK_ASSIGN(U"1=2"sv,       Assign);
    CHECK_ASSIGN(U"1 &= 2"sv, AndAssign);
    CHECK_ASSIGN(U"1 |= 2"sv,  OrAssign);
    CHECK_ASSIGN(U"1 += 2"sv, AddAssign);
    CHECK_ASSIGN(U"1 -= 2"sv, SubAssign);
    CHECK_ASSIGN(U"1 *= 2"sv, MulAssign);
    CHECK_ASSIGN(U"1 /= 2"sv, DivAssign);
    CHECK_ASSIGN(U"1 %= 2"sv, RemAssign);

#undef CHECK_BIN_OP
#undef CHECK_EMBED_OP
#undef CHECK_BASE_UINT
}


TEST(NailangBase, Serializer)
{
    using xziar::nailang::Serializer;
    using xziar::nailang::RawArg;
    using xziar::nailang::LateBindVar;
    using xziar::nailang::EmbedOps;
    RawArg a1{ true };
    RawArg a2{ false };
    RawArg a3{ uint64_t(1234) };
    RawArg a4{ int64_t(-5678) };
    RawArg a5{ U"10ab"sv };
    RawArg a6{ LateBindVar{U"`cd.ef"sv} };
    EXPECT_EQ(Serializer::Stringify(a1), U"true"sv);
    EXPECT_EQ(Serializer::Stringify(a2), U"false"sv);
    EXPECT_EQ(Serializer::Stringify(a3), U"1234"sv);
    EXPECT_EQ(Serializer::Stringify(a4), U"-5678"sv);
    EXPECT_EQ(Serializer::Stringify(a5), U"\"10ab\""sv);
    EXPECT_EQ(Serializer::Stringify(a6), U"`cd.ef"sv);
    {
        xziar::nailang::UnaryExpr expr(EmbedOps::Not, a1);
        EXPECT_EQ(Serializer::Stringify(&expr), U"!true"sv);
    }
    {
        xziar::nailang::BinaryExpr expr(EmbedOps::Add, a1, a2);
        EXPECT_EQ(Serializer::Stringify(&expr), U"true + false"sv);
    }
    {
        xziar::nailang::BinaryExpr expr(EmbedOps::NotEqual, a2, a3);
        EXPECT_EQ(Serializer::Stringify(&expr), U"false != 1234"sv);
    }
    {
        xziar::nailang::BinaryExpr expr0(EmbedOps::Equal, a1, a2);
        xziar::nailang::UnaryExpr expr(EmbedOps::Not, &expr0);
        EXPECT_EQ(Serializer::Stringify(&expr), U"!(true == false)"sv);
    }
    {
        xziar::nailang::BinaryExpr expr0(EmbedOps::Or, a1, a2);
        xziar::nailang::BinaryExpr expr(EmbedOps::And, &expr0, a3);
        EXPECT_EQ(Serializer::Stringify(&expr), U"(true || false) && 1234"sv);
    }
    {
        std::vector<RawArg> args{ a1 };
        xziar::nailang::FuncCall call{ U"Func"sv, args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func(true)"sv);
    }
    {
        std::vector<RawArg> args{ a2,a3,a4 };
        xziar::nailang::FuncCall call{ U"Func2"sv, args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func2(false, 1234, -5678)"sv);
    }
    {
        xziar::nailang::BinaryExpr expr0(EmbedOps::Or, a1, a2);
        xziar::nailang::BinaryExpr expr(EmbedOps::And, &expr0, a3);
        std::vector<RawArg> args{ &expr,a4 };
        xziar::nailang::FuncCall call{ U"Func3"sv, args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func3((true || false) && 1234, -5678)"sv);
    }
    {
        xziar::nailang::BinaryExpr expr0(EmbedOps::Or, a1, a2);
        xziar::nailang::BinaryExpr expr1(EmbedOps::And, &expr0, a3);
        std::vector<RawArg> args{ &expr1,a4 };
        xziar::nailang::FuncCall call{ U"Func4"sv, args };
        xziar::nailang::BinaryExpr expr(EmbedOps::Div, a5, &call);
        EXPECT_EQ(Serializer::Stringify(&expr), U"\"10ab\" / Func4((true || false) && 1234, -5678)"sv);
    }
}