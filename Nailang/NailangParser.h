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
        ExceptionInfo(const std::u16string_view msg, const std::u32string_view name, const std::u32string_view part) noexcept
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
        ExceptionInfo(const std::u16string_view msg, const std::u16string_view file, std::pair<size_t, size_t> pos = { 0,0 }) noexcept
            : ExceptionInfo(TYPENAME, msg, file, pos)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u16string_view msg, const std::u16string_view file, 
            std::pair<size_t, size_t> pos = { 0,0 }) : TPInfo(type, msg), File(file), Position(pos)
        { }
    );
    NailangParseException(const std::u16string_view msg, const std::u16string_view file = {}, std::pair<size_t, size_t> pos = { 0,0 })
        : NailangParseException(T_<ExceptionInfo>{}, msg, file, pos)
    { }
    std::u16string_view GetFileName() const noexcept { return GetInfo().File; }
    virtual std::pair<size_t, size_t> GetPosition() const noexcept { return GetInfo().Position; }
};
class NAILANGAPI UnexpectedTokenException : public NailangParseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(UnexpectedTokenException, NailangParseException,
        common::parser::DetailToken Token;
        ExceptionInfo(const std::u16string_view msg, const common::parser::DetailToken& token, const std::u16string_view file) noexcept
            : TPInfo(TYPENAME, msg, file), Token(token) { }
    );
    UnexpectedTokenException(const std::u16string_view msg, const common::parser::DetailToken& token,
        const std::u16string_view file = {})
        : NailangParseException(T_<ExceptionInfo>{}, msg, token, file)
    { }
    std::pair<size_t, size_t> GetPosition() const noexcept final 
    {
        const auto& token = GetInfo().Token;
        return { token.Row + 1, token.Col };
    }
};


class NAILANGAPI NailangParser : public common::parser::ParserBase
{
private:
protected:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    constexpr std::pair<uint32_t, uint32_t> GetCurPos(const bool ignoreCol = false) const noexcept
    {
        return { gsl::narrow_cast<uint32_t>(Context.Row + 1), ignoreCol ? 0u : gsl::narrow_cast<uint32_t>(Context.Col) };
    }
    static constexpr std::pair<uint32_t, uint32_t> GetPosition(const common::parser::DetailToken& token) noexcept
    {
        return { token.Row + 1, token.Col };
    }
    [[nodiscard]] std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    [[nodiscard]] common::str::StrVariant<char16_t> GetCurrentFileName() const noexcept override;
    virtual void HandleException(const NailangParseException& ex) const;
    common::parser::DetailToken OnUnExpectedToken(const common::parser::DetailToken& token, const std::u16string_view extraInfo = {}) const override;

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
        std::pair<uint32_t, uint32_t> pos = { 0,0 }, FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] static std::optional<RawArg> ParseSingleArg(MemoryPool& pool, common::parser::ParserContext& context, std::string_view stopDelims, std::u32string_view stopChecker);
    [[nodiscard]] static std::optional<RawArg> ParseSingleArg(MemoryPool& pool, common::parser::ParserContext& context, std::string_view stopDelims)
    {
        std::u32string stopChecker(stopDelims.begin(), stopDelims.end());
        return ParseSingleArg(pool, context, stopDelims, stopChecker);
    }
    [[nodiscard]] static std::optional<RawArg> ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context)
    {
        return ParseSingleArg(pool, context, ";", U";");
    }
};


class NAILANGAPI RawBlockParser : public NailangParser
{
protected:
    void EatSemiColon();
    void FillBlockName(RawBlock& block);
    void FillFileName(RawBlock& block) const noexcept;
    [[nodiscard]] RawBlock FillRawBlock(const std::u32string_view name, std::pair<uint32_t, uint32_t> pos);
    [[nodiscard]] forceinline RawBlock FillRawBlock(const std::u32string_view name)
    {
        return FillRawBlock(name, GetCurPos(true));
    } 
    [[nodiscard]] forceinline RawBlock FillRawBlock(const common::parser::DetailToken& token)
    {
        return FillRawBlock(token.GetString(), GetPosition(token));
    }
public:
    using NailangParser::NailangParser;
    
    [[nodiscard]] RawBlockWithMeta GetNextRawBlock();
    [[nodiscard]] std::vector<RawBlockWithMeta> GetAllRawBlocks();
};


class NAILANGAPI BlockParser : public RawBlockParser
{
protected:
    [[nodiscard]] Assignment ParseAssignment(const std::u32string_view var, std::pair<uint32_t, uint32_t> pos);
    [[nodiscard]] Assignment ParseAssignment(const std::u32string_view var)
    {
        return ParseAssignment(var, GetCurPos());
    }
    [[nodiscard]] Assignment ParseAssignment(const common::parser::DetailToken& token)
    {
        return ParseAssignment(token.GetString(), GetPosition(token));
    }
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
