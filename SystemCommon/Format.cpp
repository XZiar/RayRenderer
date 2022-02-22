#include "SystemCommonPch.h"
#include "Format.h"
#include "Exceptions.h"
#include "StringFormat.h"

namespace common::str::exp
{
using namespace std::string_view_literals;


void ArgChecker::CheckDD(const StrArgInfo& strInfo, const ArgResult& argInfo)
{
    const auto strIndexArgCount = static_cast<uint8_t>(strInfo.IndexTypes.size());
    const auto strNamedArgCount = static_cast<uint8_t>(strInfo.NamedTypes.size());
    if (argInfo.IdxArgCount < strIndexArgCount)
    {
        COMMON_THROW(BaseException, FMTSTR(u"No enough indexed arg, expects [{}], has [{}]"sv, 
            strIndexArgCount, argInfo.IdxArgCount));
    }
    if (argInfo.NamedArgCount < strNamedArgCount)
    {
        COMMON_THROW(BaseException, FMTSTR(u"No enough named arg, expects [{}], has [{}]"sv,
            strNamedArgCount, argInfo.NamedArgCount));
    }
    if (strIndexArgCount > 0)
    {
        const auto index = ArgChecker::GetIdxArgMismatch(strInfo.IndexTypes.data(), argInfo.IndexTypes, strIndexArgCount);
        if (index != ParseResult::IdxArgSlots)
            COMMON_THROW(BaseException, FMTSTR(u"IndexArg[{}] type mismatch"sv, index)).Attach("arg", index)
                .Attach("argType", std::pair{ strInfo.IndexTypes[index], ArgResult::ToArgType(argInfo.IndexTypes[index]) });
    }
    if (strNamedArgCount > 0)
    {
        const auto [askIdx, giveIdx] = ArgChecker::GetNamedArgMismatch(
            strInfo.NamedTypes.data(), strInfo.FormatString, strNamedArgCount,
            argInfo.NamedTypes, argInfo.Names, argInfo.NamedArgCount);
        if (askIdx != ParseResult::NamedArgSlots) // arg not found
        {
            const auto& namedArg = strInfo.NamedTypes[askIdx];
            const auto name = strInfo.FormatString.substr(namedArg.Offset, namedArg.Length);
            if (giveIdx != ParseResult::NamedArgSlots) // type mismatch
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] type mismatch"sv, name)).Attach("arg", name)
                    .Attach("argType", std::pair{ namedArg.Type, ArgResult::ToArgType(argInfo.NamedTypes[giveIdx]) });
            else
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] not found in args"sv, name)).Attach("arg", name);
        }
        Ensures(giveIdx == ParseResult::NamedArgSlots);
    }
}


}

