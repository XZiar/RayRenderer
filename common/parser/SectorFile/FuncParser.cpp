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
    //constexpr auto ExpectArg = TokenMatcherHelper::GetMatcher(std::array{ BracketR }, BaseToken::String, BaseToken::Uint, BaseToken::Int, BaseToken::FP, BaseToken::Bool, SectorLangToken::Var);

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
        default: // must be SectorLangToken::Var
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
ComplexFuncArgRaw ComplexArgParser::ParseArg()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
    
    std::optional<BasicFuncArgRaw> oprend1, oprend2;
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer, tokenizer::VariableTokenizer>(StopDelimer());
    const auto token = GetNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken);


    return ComplexFuncArgRaw();
}

}
