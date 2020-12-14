#pragma once
#include "NailangStruct.h"
#include "common/Exceptions.hpp"
#include "common/parser/ParserBase.hpp"
#include <optional>
#include <any>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::nailang
{

class NailangPartedNameException final : public common::BaseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(NailangPartedNameException, BaseException,
        std::u32string_view Name;
        std::u32string_view Part;
        ExceptionInfo(const std::u16string_view msg, const std::u32string_view name, const std::u32string_view part)
            : TPInfo(TYPENAME, msg), Name(name), Part(part)
        { }
    );
    NailangPartedNameException(const std::u16string_view msg, const std::u32string_view name, const std::u32string_view part)
        : BaseException(T_<ExceptionInfo>{}, msg, name, part)
    { }
};

class NAILANGAPI NailangParseException : public common::BaseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(NailangParseException, BaseException,
        std::u16string File;
        std::pair<size_t, size_t> Position;
        ExceptionInfo(const std::u16string_view msg, const std::u16string_view file, std::pair<size_t, size_t> pos)
            : ExceptionInfo(TYPENAME, msg, file, pos)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u16string_view msg, const std::u16string_view file, std::pair<size_t, size_t> pos)
            : TPInfo(type, msg), File(file), Position(pos)
        { }
    );
    NailangParseException(const std::u16string_view msg, const std::u16string_view file = {}, std::pair<size_t, size_t> pos = { 0,0 })
        : NailangParseException(T_<ExceptionInfo>{}, msg, file, pos)
    { }
    std::u16string_view GetFileName() const noexcept { return GetInfo().File; }
    std::pair<size_t, size_t> GetPosition() const noexcept { return GetInfo().Position; }
};
class NAILANGAPI UnexpectedTokenException : public NailangParseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(UnexpectedTokenException, NailangParseException,
        common::parser::ParserToken Token;
        ExceptionInfo(const std::u16string_view msg, common::parser::ParserToken token, 
            const std::u16string_view file, std::pair<size_t, size_t> pos)
            : TPInfo(TYPENAME, msg, file, pos), Token(token) { }
    );
    UnexpectedTokenException(const std::u16string_view msg, common::parser::ParserToken token,
        const std::u16string_view file = {}, std::pair<size_t, size_t> pos = { 0,0 })
        : NailangParseException(T_<ExceptionInfo>{}, msg, token, file, pos)
    { }
};


class NAILANGAPI NailangParser : public common::parser::ParserBase
{
private:
protected:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    static std::pair<uint32_t, uint32_t> GetPosition(const common::parser::ParserContext& context, const bool ignoreCol = false) noexcept;

    [[nodiscard]] std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    [[nodiscard]] common::str::StrVariant<char16_t> GetCurrentFileName() const noexcept override;
    virtual void HandleException(const NailangParseException& ex) const;
    common::parser::ParserToken OnUnExpectedToken(const common::parser::ParserToken& token, const std::u16string_view extraInfo = {}) const override;

    template<typename ExpectHolder, typename... TKs>
    void EatSingleToken()
    {
        //constexpr common::parser::ParserToken ExpectToken = ExpectHolder::Token;
        constexpr auto ExpectMatcher = common::parser::detail::TokenMatcherHelper::GetMatcher(std::array{ ExpectHolder::Token });
        constexpr auto IgnoreChar = common::parser::ASCIIChecker("\r\n\v\t ");
        constexpr auto Lexer = common::parser::ParserLexerBase<common::parser::tokenizer::CommentTokenizer, TKs...>();
        ExpectNextToken(Lexer, IgnoreChar, IgnoreCommentToken, ExpectMatcher);
    }

    void EatLeftParenthese();
    void EatRightParenthese();
    void EatLeftCurlyBrace();
    void EatRightCurlyBrace();

    [[nodiscard]] LateBindVar* CreateVar(std::u32string_view name) const;
public:
    NailangParser(MemoryPool& pool, common::parser::ParserContext& context, std::u16string subScope = u"") :
        ParserBase(context), MemPool(pool), SubScopeName(std::move(subScope)) { }
    virtual ~NailangParser() { }
};


class NAILANGAPI ComplexArgParser : public NailangParser
{
private:
    using NailangParser::NailangParser;

    [[nodiscard]] std::pair<std::optional<RawArg>, char32_t> ParseArg(std::string_view stopDelim);
    [[nodiscard]] FuncName* CreateFuncName(std::u32string_view name, FuncName::FuncInfo info) const;
public:
    [[nodiscard]] static RawArg ProcessString(std::u32string_view str, MemoryPool& pool);
    [[nodiscard]] static FuncCall ParseFuncBody(std::u32string_view name, MemoryPool& pool, common::parser::ParserContext& context, 
        FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] static std::optional<RawArg> ParseSingleArg(std::string_view stopDelim, MemoryPool& pool, common::parser::ParserContext& context);
    [[nodiscard]] static std::optional<RawArg> ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context)
    {
        return ParseSingleArg(";", pool, context);
    }
};


class NAILANGAPI RawBlockParser : public NailangParser
{
protected:
    void EatSemiColon();
    void FillBlockName(RawBlock& block);
    void FillFileName(RawBlock& block) const noexcept;
    [[nodiscard]] RawBlock FillRawBlock(const std::u32string_view name);
public:
    using NailangParser::NailangParser;
    
    [[nodiscard]] RawBlockWithMeta GetNextRawBlock();
    [[nodiscard]] std::vector<RawBlockWithMeta> GetAllRawBlocks();
};


class NAILANGAPI BlockParser : public RawBlockParser
{
protected:
    [[nodiscard]] Assignment ParseAssignment(const std::u32string_view var);
    void ParseContentIntoBlock(const bool allowNonBlock, Block& block, const bool tillTheEnd = true);
    using RawBlockParser::RawBlockParser;
public:
    [[nodiscard]] static Block ParseRawBlock(const RawBlock& block, MemoryPool& pool);
    [[nodiscard]] static Block ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context);
};


class NAILANGAPI ReplaceEngine
{
protected:
    static std::u32string_view TrimStrBlank(const std::u32string_view str) noexcept;
    virtual void HandleException(const NailangParseException& ex) const;
    virtual void OnReplaceOptBlock(std::u32string& output, void* cookie, std::u32string_view cond, std::u32string_view content);
    virtual void OnReplaceVariable(std::u32string& output, void* cookie, std::u32string_view var);
    virtual void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args);
    std::u32string ProcessOptBlock(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
    std::u32string ProcessVariable(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
    std::u32string ProcessFunction(const std::u32string_view source, const std::u32string_view prefix, const std::u32string_view suffix, void* cookie = nullptr);
public:
    virtual ~ReplaceEngine();
};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
