#pragma once
#include "RenderCoreRely.h"
#include "SystemCommon/StringDetect.h"
#include "SystemCommon/CharConvs.h"

namespace dizz::detail
{


class OBJLoder
{
private:
    common::fs::path FilePath;
    std::vector<uint8_t> Content;
    size_t CurPos, Length;
public:
    common::str::Encoding chset;
    struct TextLine
    {
        uint64_t Type;
        std::string_view Line;
        std::vector<std::string_view> Params;
        common::str::Encoding charset;

        TextLine() {}

        TextLine(const common::str::Encoding chset, const string& prefix) : 
            Type(common::DJBHash::HashC(prefix)), charset(chset) {}

        template<size_t N>
        TextLine(const common::str::Encoding chset, const char(&prefix)[N] = "EMPTY") : 
            Type(common::DJBHash::HashP(prefix)), charset(chset) {}

        TextLine(const common::str::Encoding chset, const std::string_view& line) : Line(line), charset(chset) { Params.reserve(8); }

        TextLine(const TextLine& other) = default;
        TextLine(TextLine&& other) = default;
        TextLine& operator =(const TextLine& other) = default;
        TextLine& operator =(TextLine&& other) = default;

        template<typename T>
        void SetType(const T& prefix) { Type = common::DJBHash::Hash(prefix); }

        std::string_view Rest(const size_t fromIndex = 1)
        {
            if (Params.size() <= fromIndex)
                return {};
            const auto lenTotal = Line.length();
            const auto beginLine = Line.data(), beginRest = Params[fromIndex].data();
            return std::string_view(beginRest, lenTotal - (beginRest - beginLine));
        }

        u16string GetUString(const size_t index) const
        {
            if (Params.size() <= index)
                return u"";
            return common::str::to_u16string(Params[index], charset);
        }

        u16string ToUString() const { return common::str::to_u16string(Line, charset); }

        mbase::Vec2 ParamsToFloat2(size_t offset) const
        {
            mbase::Vec2 ret;
            for (uint8_t i = 0; offset < Params.size() && i < 2; ++offset, ++i)
                common::StrToFP(Params[offset], ret[i]);
            return ret;
        }
        mbase::Vec3 ParamsToFloat3(size_t offset) const
        {
            mbase::Vec3 ret;
            for (uint8_t i = 0; offset < Params.size() && i < 3; ++offset, ++i)
                common::StrToFP(Params[offset], ret[i]);
            return ret;
        }
        mbase::Vec4 ParamsToFloat4(size_t offset) const
        {
            mbase::Vec4 ret;
            for (uint8_t i = 0; offset < Params.size() && i < 4; ++offset, ++i)
                common::StrToFP(Params[offset], ret[i]);
            return ret;
        }
        mbase::IVec4 ParamsToInt4(size_t offset) const
        {
            mbase::IVec4 ret;
            for (uint8_t i = 0; offset < Params.size() && i < 4; ++offset, ++i)
                common::StrToInt(Params[offset], ret[i]);
            return ret;
        }

        template<size_t N>
        uint8_t ParseInts(const uint8_t idx, int32_t(&output)[N]) const
        {
            uint8_t cnt = 0;
            common::str::SplitStream(Params[idx], '/', true)
                .Take(N)
                .ForEach([&cnt, &output](const auto sv)
                    {
                        output[cnt++] = sv.empty() ? 0 : atoi(sv.data());
                    });
            return cnt;
        }

        template<size_t N>
        uint8_t ParseFloats(const uint8_t idx, float(&output)[N]) const
        {
            uint8_t cnt = 0;
            common::str::SplitStream(Params[idx], '/', true)
                .Take(N)
                .ForEach([&cnt, &output](const auto sv)
                    {
                        output[cnt++] = sv.empty() ? 0 : atof(sv.data());
                    });
            return cnt;
        }

        operator const bool() const
        {
            return Type != "EOF"_hash;
        }
    };

    OBJLoder(const fs::path& fpath_) : FilePath(fpath_)
    {
        //pre-load data, in case of Acess-Violate while reading file
        common::file::ReadAll(FilePath, Content);
        Length = Content.size() - 1;
        CurPos = 0;
        chset = common::str::DetectEncoding(Content);
        dizzLog().Debug(u"obj file[{}]--encoding[{}]\n", FilePath.u16string(), GetEncodingName(chset));
    }

    TextLine ReadLine()
    {
        using std::string_view;
        size_t fromPos = CurPos, lineLength = 0;
        for (bool isInLine = false; CurPos < Length;)
        {
            const uint8_t curChar = Content[CurPos++];
            const bool isLineEnd = (curChar == '\r' || curChar == '\n');
            if (isLineEnd)
            {
                if (isInLine)//finish this line
                    break;
                else
                    ++fromPos;
            }
            else
            {
                ++lineLength;//linelen EQUALS count how many ch is not "NEWLINE"
                isInLine = true;
            }
        }
        if (lineLength == 0)
        {
            if (CurPos == Length)//EOF
                return { chset, "EOF" };
            else
                return { chset, "EMPTY" };
        }
        TextLine textLine(chset, string_view((const char*)&Content[fromPos], lineLength));

        textLine.Params = common::str::Split(textLine.Line, [](const char ch)
            {
                return (uint8_t)(ch) < uint8_t(0x21) || (uint8_t)(ch) == uint8_t(0x7f);//non-graph character
            }, false);
        if (textLine.Params.size() == 0)
            return { chset, "EMPTY" };

        textLine.SetType(textLine.Params[0]);
        return textLine;
    }
};


}