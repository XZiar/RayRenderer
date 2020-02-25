#include "SectorsParser.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{



SectorRaw SectorsParser::ParseNextSector()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;
       
    constexpr auto BracketDelim = DelimTokenizer("(){}");

    constexpr auto ExpectSector = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, tokenizer::SectorLangToken::Sector);
    constexpr ParserToken BracketL(BaseToken::Delim, U'(');
    constexpr ParserToken BracketR(BaseToken::Delim, U')');
    constexpr ParserToken CurlyBraceL(BaseToken::Delim, U'{');
    constexpr auto ExpectBracketL = TokenMatcherHelper::GetMatcher(std::array{ BracketL });
    constexpr auto ExpectBracketR = TokenMatcherHelper::GetMatcher(std::array{ BracketR });
    constexpr auto ExpectString = TokenMatcherHelper::GetMatcher(EmptyTokenArray{}, BaseToken::String);
    constexpr auto ExpectCurlyBraceL = TokenMatcherHelper::GetMatcher(std::array{ CurlyBraceL });

    constexpr auto MainLexer = ParserLexerBase<CommentTokenizer, tokenizer::BlockPrefixTokenizer>();
    const auto sectorToken = ExpectNextToken(MainLexer, IgnoreBlank, IgnoreCommentToken, ExpectSector);

        
    constexpr auto NameLexer = ParserLexerBase<CommentTokenizer, DelimTokenizer, StringTokenizer>(BracketDelim);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketL);
    const auto sectorNameToken = ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectString);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectBracketR);
    ExpectNextToken(NameLexer, IgnoreBlank, IgnoreCommentToken, ExpectCurlyBraceL);
        
    ContextReader reader(Context);
    auto guardString = std::u32string(reader.ReadLine());
    guardString.append(U"}");
    auto content = reader.ReadUntil(guardString);
    content.remove_suffix(guardString.size());

    return SectorRaw(sectorToken.GetString(), sectorNameToken.GetString(), content);
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
