#include "rely.h"
#include "ParserCommon.h"
#include "Nailang/ParserRely.h"
#include "Nailang/NailangParser.h"
#include "StringUtil/Format.h"
#include <algorithm>


using namespace std::string_view_literals;
using common::parser::ParserContext;
using common::parser::ContextReader;



#define CHECK_TK(token, etype, type, action, val) do { EXPECT_EQ(token.GetIDEnum<etype>(), etype::type); EXPECT_EQ(token.action(), val); } while(0)
#define CHECK_BASE_TK(token, type, action, val) CHECK_TK(token, common::parser::BaseToken, type, action, val)


struct MemPool_ : public xziar::nailang::MemoryPool
{
    using MemoryPool::Trunks;
    using MemoryPool::DefaultTrunkLength;
};

static std::string PrintMemPool(const xziar::nailang::MemoryPool& pool_) noexcept
{
    const auto& pool = static_cast<const MemPool_&>(pool_);
    const auto [used, total] = pool.Usage();
    std::string txt = fmt::format("MemoryPool[{} trunks(default {} bytes)]: [{}/{}]\n", 
        pool.Trunks.size(), pool.DefaultTrunkLength, used, total);
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

TEST(NailangBase, OpSymbolTokenizer)
{
    using common::parser::BaseToken;
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        return TKParse<xziar::nailang::tokenizer::OpSymbolTokenizer, common::parser::tokenizer::IntTokenizer>(src);
    };
#define CHECK_BASE_INT(token, val) CHECK_BASE_TK(token, Int, GetInt, val)
#define CHECK_EMBED_OP(token, type) CHECK_TK(token, xziar::nailang::tokenizer::NailangToken, OpSymbol, GetInt, common::enum_cast(xziar::nailang::type))

#define CHECK_BIN_OP(src, type) do                  \
    {                                               \
        const auto tokens = ParseAll(src);          \
        CHECK_BASE_INT(tokens[0], 1);               \
        CHECK_EMBED_OP(tokens[1], EmbedOps::type);  \
        CHECK_BASE_INT(tokens[2], 2);               \
    } while(0)

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
        const auto tokens = ParseAll(U"?1"sv);
        CHECK_EMBED_OP(tokens[0], ExtraOps::Quest);
        CHECK_BASE_INT(tokens[1], 1);
    }
    {
        const auto tokens = ParseAll(U"!1"sv);
        CHECK_EMBED_OP(tokens[0], EmbedOps::Not);
        CHECK_BASE_INT(tokens[1], 1);
    }
    {
        const auto tokens = ParseAll(U"1? 2 : 3"sv);
        CHECK_BASE_INT(tokens[0], 1);
        CHECK_EMBED_OP(tokens[1], ExtraOps::Quest);
        CHECK_BASE_INT(tokens[2], 2);
        CHECK_EMBED_OP(tokens[3], ExtraOps::Colon);
        CHECK_BASE_INT(tokens[4], 3);
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
        return TKParse<xziar::nailang::tokenizer::AssignOpTokenizer, common::parser::tokenizer::IntTokenizer>(src);
    };
#define CHECK_BASE_INT(token, val) CHECK_BASE_TK(token, Int, GetInt, val)
#define CHECK_ASSIGN_OP(token, type) CHECK_TK(token, xziar::nailang::tokenizer::NailangToken, Assign, GetInt, common::enum_cast(xziar::nailang::tokenizer::AssignOps::type))

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
    CHECK_ASSIGN(U"1 ?= 2"sv, NilAssign);
    CHECK_ASSIGN(U"1 := 2"sv, NewCreate);

#undef CHECK_BIN_OP
#undef CHECK_EMBED_OP
#undef CHECK_BASE_UINT
}



TEST(NailangBase, FuncName)
{
    using xziar::nailang::FuncName;
    using xziar::nailang::TempFuncName;
    using VI = xziar::nailang::FuncName::FuncInfo;
    xziar::nailang::MemoryPool Pool;
    {
        constexpr auto name = U"a.b.c"sv;
        const auto func = FuncName::Create(Pool, name, FuncName::FuncInfo::Empty);
        EXPECT_EQ(*func, name);
        EXPECT_EQ(func->Info(), VI::Empty);
        EXPECT_THAT(func->Parts(), testing::ElementsAre(U"a"sv, U"b"sv, U"c"sv));
        EXPECT_EQ(func->GetRest(0), name);
        EXPECT_EQ(func->GetRest(1), name.substr(2));
    }
    std::vector<TempFuncName> funcs;
    {
        ASSERT_EQ(funcs.size(), 0u);
        constexpr auto name = U"a.b.c"sv;
        auto func_ = FuncName::CreateTemp(name, FuncName::FuncInfo::Empty);
        {
            const FuncName& func = func_;
            EXPECT_EQ(reinterpret_cast<const void*>(&func), reinterpret_cast<const void*>(&func_));
            EXPECT_EQ(func, name);
        }
        funcs.push_back(std::move(func_));
        {
            const FuncName& func = func_;
            EXPECT_EQ(&func, nullptr);
        }
        {
            const FuncName& func = funcs.back();
            EXPECT_EQ(func.Info(), VI::Empty);
            EXPECT_THAT(func.Parts(), testing::ElementsAre(U"a"sv, U"b"sv, U"c"sv));
            EXPECT_EQ(func.GetRest(0), name);
            EXPECT_EQ(func.GetRest(1), name.substr(2));
        }
    }
    {
        constexpr auto name = U"a..c"sv;
        EXPECT_THROW(FuncName::Create(Pool, name, FuncName::FuncInfo::Empty), xziar::nailang::NailangPartedNameException);
    }
}


TEST(NailangBase, Serializer)
{
    using xziar::nailang::Serializer;
    using xziar::nailang::Expr;
    using xziar::nailang::LateBindVar;
    using xziar::nailang::EmbedOps;
    using xziar::nailang::FuncName;
    Expr a1{ true };
    Expr a2{ false };
    Expr a3{ uint64_t(1234) };
    Expr a4{ int64_t(-5678) };
    Expr a5{ U"10ab"sv };
    Expr a6{ LateBindVar(U"`cdef"sv) };
    EXPECT_EQ(Serializer::Stringify(a1), U"true"sv);
    EXPECT_EQ(Serializer::Stringify(a2), U"false"sv);
    EXPECT_EQ(Serializer::Stringify(a3), U"1234"sv);
    EXPECT_EQ(Serializer::Stringify(a4), U"-5678"sv);
    EXPECT_EQ(Serializer::Stringify(a5), U"\"10ab\""sv);
    EXPECT_EQ(Serializer::Stringify(a6), U"`cdef"sv);
    {
        xziar::nailang::UnaryExpr expr(EmbedOps::Not, a1);
        EXPECT_EQ(Serializer::Stringify(&expr), U"!true"sv);
    }
    {
        std::vector<Expr> queries;
        xziar::nailang::SubQuery::PushQuery(queries, a3);
        xziar::nailang::QueryExpr expr(a5, queries);
        EXPECT_EQ(Serializer::Stringify(&expr), U"\"10ab\"[1234]"sv);
    }
    {
        std::vector<Expr> queries;
        xziar::nailang::SubQuery::PushQuery(queries, a4);
        xziar::nailang::QueryExpr expr(a6, queries);
        EXPECT_EQ(Serializer::Stringify(&expr), U"`cdef[-5678]"sv);
    }
    {
        std::vector<Expr> queries;
        xziar::nailang::SubQuery::PushQuery(queries, U"xyzw"sv);
        xziar::nailang::QueryExpr expr(a6, queries);
        EXPECT_EQ(Serializer::Stringify(&expr), U"`cdef.xyzw"sv);
    }
    {
        std::vector<Expr> queries;
        xziar::nailang::SubQuery::PushQuery(queries, U"xyzw"sv);
        xziar::nailang::SubQuery::PushQuery(queries, a3);
        xziar::nailang::SubQuery::PushQuery(queries, U"abcd"sv);
        xziar::nailang::QueryExpr expr(a6, queries);
        EXPECT_EQ(Serializer::Stringify(&expr), U"`cdef.xyzw[1234].abcd"sv);
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
        const auto name = FuncName::CreateTemp(U"Func"sv, FuncName::FuncInfo::Empty);
        std::vector<Expr> args{ a1 };
        xziar::nailang::FuncCall call{ &name.Get(), args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func(true)"sv);
    }
    {
        const auto name = FuncName::CreateTemp(U"Func2"sv, FuncName::FuncInfo::Empty);
        std::vector<Expr> args{ a2,a3,a4 };
        xziar::nailang::FuncCall call{ &name.Get(), args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func2(false, 1234, -5678)"sv);
    }
    {
        const auto name = FuncName::CreateTemp(U"Func3"sv, FuncName::FuncInfo::Empty);
        xziar::nailang::BinaryExpr expr0(EmbedOps::Or, a1, a2);
        xziar::nailang::BinaryExpr expr(EmbedOps::And, &expr0, a3);
        std::vector<Expr> args{ &expr,a4 };
        xziar::nailang::FuncCall call{ &name.Get(), args };
        EXPECT_EQ(Serializer::Stringify(&call), U"Func3((true || false) && 1234, -5678)"sv);
    }
    {
        const auto name = FuncName::CreateTemp(U"Func4"sv, FuncName::FuncInfo::Empty);
        xziar::nailang::BinaryExpr expr0(EmbedOps::Or, a1, a2);
        xziar::nailang::BinaryExpr expr1(EmbedOps::And, &expr0, a3);
        std::vector<Expr> args{ &expr1,a4 };
        xziar::nailang::FuncCall call{ &name.Get(), args };
        xziar::nailang::BinaryExpr expr(EmbedOps::Div, a5, &call);
        EXPECT_EQ(Serializer::Stringify(&expr), U"\"10ab\" / Func4((true || false) && 1234, -5678)"sv);
    }
}