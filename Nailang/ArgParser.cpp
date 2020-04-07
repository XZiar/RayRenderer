#include "ArgParser.h"
#include "ParserRely.h"

namespace xziar::nailang
{
using tokenizer::SectorLangToken;

struct FuncEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ",)"sv; }
};
struct GroupEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ")"sv; }
};
struct StatementEndDelimer
{
    static constexpr DelimTokenizer Stopper() noexcept { return ";"sv; }
};

template<typename StopDelimer>
std::pair<std::optional<RawArg>, char32_t> ComplexArgParser::ParseArg()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    
    constexpr DelimTokenizer StopDelim = StopDelimer::Stopper();
    constexpr auto ArgLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, 
        tokenizer::ParentheseTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, 
        tokenizer::VariableTokenizer, tokenizer::EmbedOpTokenizer>(StopDelim);
    constexpr auto OpLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer,
        tokenizer::EmbedOpTokenizer>(StopDelim);
    
    std::optional<RawArg> oprend1, oprend2;
    std::optional<EmbedOps> op;
    char32_t stopChar = common::parser::special::CharEnd;

    while (true) 
    {
        const bool isAtOp = oprend1.has_value() && !op.has_value() && !oprend2.has_value();
        ParserToken token = isAtOp ?
            GetNextToken(OpLexer,  IgnoreBlank, IgnoreCommentToken) :
            GetNextToken(ArgLexer, IgnoreBlank, IgnoreCommentToken);

        {
            const auto type = token.GetIDEnum();
            if (type == BaseToken::Unknown || type == BaseToken::Error)
            {
                if (isAtOp)
                    throw U"expect an operator"sv;
                else
                    throw U"unknown or error token"sv;
            }
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
            EID(BaseToken::Uint)        : target = token.GetUInt();                     break;
            EID(BaseToken::Int)         : target = token.GetInt();                      break;
            EID(BaseToken::FP)          : target = token.GetDouble();                   break;
            EID(BaseToken::Bool)        : target = token.GetBool();                     break;
            EID(SectorLangToken::Var)   : target = LateBindVar{ token.GetString() };    break;
            EID(BaseToken::String)      : 
                target = ProcessString(token.GetString(), MemPool); 
                break;
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
        return { MemPool.Create<UnaryExpr>(*op, std::move(*oprend2)), stopChar };

    }
    else
    {
        Expects(oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack 2nd oprend for binary operator"sv;
        return { MemPool.Create<BinaryExpr>(*op, std::move(*oprend1), std::move(*oprend2)), stopChar };
    }
}

RawArg ComplexArgParser::ProcessString(const std::u32string_view str, MemoryPool& pool)
{
    Expects(str.size() == 0 || str.back() != U'\\');
    const auto idx = str.find(U'\\');
    if (idx == std::u32string_view::npos)
        return str;
    std::u32string output; output.reserve(str.size() - 1);
    output.assign(str.data(), idx);

    for (size_t i = idx; i < str.size(); ++i)
    {
        if (str[i] != U'\\')
            output.push_back(str[i]);
        else
        {
            switch (str[++i]) // promise '\' won't be last element.
            {
            case U'\\': output.push_back(U'\\');  break;
            case U'0' : output.push_back(U'\0');  break;
            case U'r' : output.push_back(U'\r');  break;
            case U'n' : output.push_back(U'\n');  break;
            case U't' : output.push_back(U'\t');  break;
            case U'"' : output.push_back(U'"' );  break;
            default   : output.push_back(str[i]); break;
            }
        }
    }
    const auto space = pool.CreateArray(output);
    return std::u32string_view(space.data(), space.size());
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
