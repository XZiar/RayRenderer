#include "FuncParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;

void FuncBodyParser::FillFuncBody(MetaFunc& func)
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto BracketDelim = DelimTokenizer("(){},");

    constexpr ParserToken BracketL(BaseToken::Delim, U'(');
    constexpr ParserToken BracketR(BaseToken::Delim, U')');
    constexpr ParserToken Comma   (BaseToken::Delim, U',');
    constexpr auto ExpectBracketL        = TokenMatcherHelper::GetMatcher(std::array{ BracketL });
    constexpr auto ExpectCommaOrBracketR = TokenMatcherHelper::GetMatcher(std::array{ Comma, BracketR });

    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, tokenizer::VariableTokenizer>(BracketDelim);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketL);

    while (true)
    {
        const auto token = GetNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken);
        switch (token.GetIDEnum())
        {
        case BaseToken::String: func.Args.emplace_back(token.GetString());  break;
        case BaseToken::Uint:   func.Args.emplace_back(token.GetUInt());    break;
        case BaseToken::Int:    func.Args.emplace_back(token.GetInt());     break;
        case BaseToken::FP:     func.Args.emplace_back(token.GetDouble());  break;
        case BaseToken::Bool:   func.Args.emplace_back(token.GetBool());    break;
        case BaseToken::Delim:  
            if (token.GetChar() == U')')
                return;
            else
                throw U"Not ')' delim"sv;
        default:
            if (token.GetIDEnum<SectorLangToken>() == SectorLangToken::Var)
            {
                func.Args.emplace_back(LateBindVar{ token.GetString() }); break;
            }
            throw U"Not Variable"sv;
        }
        const auto next = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectCommaOrBracketR);
        if (next.GetChar() == U')')
            return;
    }

}

MetaFunc FuncBodyParser::ParseFuncBody(std::u32string_view funcName, common::parser::ParserContext& context)
{
    FuncBodyParser parser(context);
    MetaFunc func{ funcName, {} };
    parser.FillFuncBody(func);
    return func;
}


template<typename StopDelimer>
std::pair<std::optional<FuncArgRaw>, char32_t> ComplexArgParser::ParseArg()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    
    constexpr DelimTokenizer StopDelim = StopDelimer::Stopper;
    constexpr auto ArgLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, tokenizer::NormalFuncPrefixTokenizer,
        StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, tokenizer::VariableTokenizer, tokenizer::EmbedOpTokenizer>(StopDelim);
    
    std::optional<FuncArgRaw> oprend1, oprend2;
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
            EID(SectorLangToken::Var)   : target = LateBindVar{ token.GetString() };            break;
            EID(BaseToken::String)      : target = token.GetString();                           break;
            EID(BaseToken::Uint)        : target = token.GetUInt();                             break;
            EID(BaseToken::Int)         : target = token.GetInt();                              break;
            EID(BaseToken::FP)          : target = token.GetDouble();                           break;
            EID(BaseToken::Bool)        : target = token.GetBool();                             break;
            EID(SectorLangToken::Func)  : 
                target = std::make_unique<FuncCall>(ParseFuncBody(token.GetString(), Context)); break;
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
        return { std::make_unique<UnaryStatement>(*op, std::move(*oprend2)), stopChar };

    }
    else
    {
        Expects(oprend1.has_value());
        if (!oprend2.has_value())
            throw U"Lack 2nd oprend for binary operator"sv;
        return { std::make_unique<BinaryStatement>(*op, std::move(*oprend1), std::move(*oprend2)), stopChar };
    }
}

void ComplexArgParser::EatLeftBracket()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    constexpr ParserToken BracketL(BaseToken::Delim, U'(');
    constexpr auto ExpectBracketL = TokenMatcherHelper::GetMatcher(std::array{ BracketL });

    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer>();
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketL);
}

struct FuncDelimer
{
    static constexpr DelimTokenizer Stopper = ",)"sv;
};

FuncCall ComplexArgParser::ParseFuncBody(std::u32string_view funcName, common::parser::ParserContext& context)
{
    ComplexArgParser parser(context);
    parser.EatLeftBracket();
    FuncCall func{ funcName,{} };
    while (true)
    {
        auto [arg, delim] = parser.ParseArg<FuncDelimer>();
        if (!arg.has_value())
            break;
        func.Args.emplace_back(std::move(*arg));
        if (delim == U')' || delim == common::parser::special::CharEnd)
            break;
    }
    return func;
}

}
