#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::nailang
{
using tokenizer::SectorLangToken;

struct FuncEndDelimer
{
    static constexpr DelimTokenizer Stopper = ",)"sv;
};
struct GroupEndDelimer
{
    static constexpr DelimTokenizer Stopper = ")"sv;
};
struct StatementEndDelimer
{
    static constexpr DelimTokenizer Stopper = ";"sv;
};

template<typename StopDelimer>
std::pair<std::optional<RawArg>, char32_t> ComplexArgParser::ParseArg()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    
    constexpr DelimTokenizer StopDelim = StopDelimer::Stopper;
    constexpr auto ArgLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, 
        tokenizer::ParentheseTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, 
        tokenizer::VariableTokenizer, tokenizer::EmbedOpTokenizer>(StopDelim);
    
    std::optional<RawArg> oprend1, oprend2;
    std::optional<EmbedOps> op;
    char32_t stopChar = common::parser::special::CharEnd;

    while (true) 
    {
        const auto token = GetNextToken(ArgLexer, IgnoreBlank, IgnoreCommentToken);

        {
            const auto type = token.GetIDEnum();
            if (type == BaseToken::Unknown || type == BaseToken::Error)
                throw U"unknown or error token"sv;
            if (type == BaseToken::Delim || type == BaseToken::End)
            {
                stopChar = token.GetChar();  
                break;
            }
        }

        if (token.GetIDEnum<SectorLangToken>() == SectorLangToken::EmbedOp)
        {
            if (op.has_value())
                throw U"Already has op"sv;
            const auto opval = static_cast<EmbedOps>(token.GetUInt());
            const bool isUnary = EmbedOpHelper::IsUnaryOp(opval);
            if (isUnary && oprend1.has_value())
                throw U"Expect no operand before unary operator"sv;
            if (!isUnary && !oprend1.has_value())
                throw U"Expect operand before binary operator"sv;
            op = opval;
        }
        else
        {
            auto& target = op.has_value() ? oprend2 : oprend1;
            if (target.has_value())
                throw U"Already has oprend"sv;
#define EID(id) case common::enum_cast(id)
            switch (token.GetID())
            {
            EID(SectorLangToken::Var)   : target = LateBindVar{ token.GetString() };    break;
            EID(BaseToken::String)      : target = token.GetString();                   break;
            EID(BaseToken::Uint)        : target = token.GetUInt();                     break;
            EID(BaseToken::Int)         : target = token.GetInt();                      break;
            EID(BaseToken::FP)          : target = token.GetDouble();                   break;
            EID(BaseToken::Bool)        : target = token.GetBool();                     break;
            EID(SectorLangToken::Func)  : 
                target = MemPool.Create<FuncCall>(
                    ParseFuncBody(token.GetString(), MemPool, Context));                
                break;
            EID(SectorLangToken::Parenthese):
                if (token.GetChar() == U'(')
                    target = ParseArg<GroupEndDelimer>().first;
                else
                    throw U"Unexpected right parenthese"sv;
                break; 
            default                     : throw U"Unexpected token"sv;
            }
#undef EID
        }
    }
    // exit from delim or end

    if (!op.has_value())
    {
        Expects(!oprend2.has_value());
        return { std::move(oprend1), stopChar };
    }
    else if (EmbedOpHelper::IsUnaryOp(*op))
    {
        Expects(!oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack oprend for unary operator"sv;
        return { MemPool.Create<UnaryStatement>(*op, std::move(*oprend2)), stopChar };

    }
    else
    {
        Expects(oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack 2nd oprend for binary operator"sv;
        return { MemPool.Create<BinaryStatement>(*op, std::move(*oprend1), std::move(*oprend2)), stopChar };
    }
}

FuncCall ComplexArgParser::ParseFuncBody(std::u32string_view funcName, MemoryPool& pool, common::parser::ParserContext& context)
{
    ComplexArgParser parser(pool, context);
    parser.EatLeftParenthese();
    std::vector<RawArg> args;
    while (true)
    {
        auto [arg, delim] = parser.ParseArg<FuncEndDelimer>();
        if (arg.has_value())
            args.emplace_back(std::move(*arg));
        else
            if (delim != U')')
                throw U"Does not allow empty argument"sv;
        if (delim == U')')
            break;
        if (delim == common::parser::special::CharEnd)
            throw U"Expect ')' before reaching end"sv;
    }
    const auto sp = pool.CreateArray(args);
    return { funcName,sp };
}

std::optional<RawArg> ComplexArgParser::ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context)
{
    ComplexArgParser parser(pool, context);
    auto [arg, delim] = parser.ParseArg<StatementEndDelimer>();
    if (delim != U';')
        throw U"Expected end with ;"sv;
    return arg;
}

}
