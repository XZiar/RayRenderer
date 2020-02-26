#include "SectorsParser.h"
#include "FuncParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;


SectorRaw SectorsParser::ParseNextSector()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto BracketDelim = DelimTokenizer("(){}");

    constexpr auto ExpectSectorOrMeta = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, SectorLangToken::Sector, SectorLangToken::MetaFunc);
    constexpr ParserToken BracketL(BaseToken::Delim, U'(');
    constexpr ParserToken BracketR(BaseToken::Delim, U')');
    constexpr ParserToken CurlyBraceL(BaseToken::Delim, U'{');
    constexpr auto ExpectBracketL = TokenMatcherHelper::GetMatcher(std::array{ BracketL });
    constexpr auto ExpectBracketR = TokenMatcherHelper::GetMatcher(std::array{ BracketR });
    constexpr auto ExpectString = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);
    constexpr auto ExpectCurlyBraceL = TokenMatcherHelper::GetMatcher(std::array{ CurlyBraceL });

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::MetaFuncPrefixTokenizer, tokenizer::BlockPrefixTokenizer>();

    SectorRaw sector;
    while (true)
    {
        const auto token = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectSectorOrMeta);
        const auto tkType = token.GetIDEnum<SectorLangToken>();
        if (tkType == SectorLangToken::Sector)
        {
            sector.Type = token.GetString();
            break;
        }
        Expects(tkType == SectorLangToken::MetaFunc);
        sector.MetaFunctions.emplace_back(FuncBodyParser::ParseFuncBody(token.GetString(), Context));
    }
        
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, StringTokenizer>(BracketDelim);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketL);
    const auto sectorNameToken = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectString);
    sector.Name = sectorNameToken.GetString();
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketR);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectCurlyBraceL);
        
    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    guardString.append(U"}");
    sector.Content = reader.ReadUntil(guardString);
    sector.Content.remove_suffix(guardString.size());

    return sector;
}

std::vector<SectorRaw> SectorsParser::ParseAllSectors()
{
    std::vector<SectorRaw> sectors;
    while (true)
    {
        ContextReader reader(Context);
        reader.ReadWhile(IgnoreBlank);
        if (reader.PeekNext() == common::parser::special::CharEnd)
            break;
        sectors.emplace_back(ParseNextSector());
    }
    return sectors;
}

}
