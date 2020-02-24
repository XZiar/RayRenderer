#include "SectorsParser.h"
#include "common/parser/ParserBase.hpp"
#include "common/StringEx.hpp"

namespace xziar::sectorlang
{
using namespace std::string_view_literals;
using common::parser::tokenizer::TokenizerResult;
using common::parser::tokenizer::StateData;
using common::parser::tokenizer::ASCIIRawTokenizer;
using common::parser::tokenizer::CommentTokenizer;
using common::parser::tokenizer::DelimTokenizer;
using common::parser::tokenizer::StringTokenizer;
using common::parser::ASCIIChecker;
using common::parser::BaseToken;
using common::parser::ParserToken;
using common::parser::ContextReader;
using common::parser::ParserBase;
using common::parser::ParserLexerBase;


namespace tokenizer
{

enum class SectorLangToken : uint16_t 
{
    __RangeMin = common::enum_cast(BaseToken::__RangeMax), 
    
    Sector, Block, MetaFunc, Func,
    
    __RangeMax = 192
};


class BlockPrefixTokenizer : public common::parser::tokenizer::TokenizerBase
{
public:
    constexpr TokenizerResult OnChar(StateData, const char32_t ch, const size_t idx) const noexcept
    {
        if (idx == 0 && ch == U'$')
            return TokenizerResult::FullMatch;
        return TokenizerResult::NotMatch;
    }
    
    forceinline constexpr ParserToken GetToken(StateData, ContextReader& reader, std::u32string_view txt) const noexcept
    {
        constexpr auto FullnameMatcher = ASCIIChecker<false>("\r\n\v\t (");

        Expects(txt.size() == 1);
        const auto fullname = reader.ReadWhile(FullnameMatcher);

        if (common::str::IsBeginWith(fullname, U"Sector."sv))
        {
            const auto typeName = fullname.substr(7);
            if (typeName.size() > 0)
                return ParserToken(SectorLangToken::Sector, typeName);
        }
        else if (common::str::IsBeginWith(fullname, U"Block."sv))
        {
            const auto typeName = fullname.substr(6);
            if (typeName.size() > 0)
                return ParserToken(SectorLangToken::Block, typeName);
        }
        return ParserToken(BaseToken::Error, fullname);
    }
};

}


constexpr static auto IgnoreBlank = ASCIIChecker("\r\n\v\t ");

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
