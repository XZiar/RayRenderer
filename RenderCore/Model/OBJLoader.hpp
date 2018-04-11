#pragma once
#include "RenderCoreRely.h"
#include "uchardetlib/uchardetlib.h"


namespace rayr::detail
{


class OBJLoder
{
private:
    fs::path FilePath;
    vector<uint8_t> Content;
    size_t CurPos, Length;
public:
    Charset chset;
    struct TextLine
    {
        uint64_t Type;
        std::string_view Line;
        vector<std::string_view> Params;
        Charset charset;

        TextLine() {}

        TextLine(const Charset chset, const string& prefix) : charset(chset), Type(hash_(prefix)) {}

        template<size_t N>
        TextLine(const Charset chset, const char(&prefix)[N] = "EMPTY") : charset(chset), Type(hash_(prefix)) {}

        TextLine(const Charset chset, const std::string_view& line) : charset(chset), Line(line) { Params.reserve(8); }

        TextLine(const TextLine& other) = default;
        TextLine(TextLine&& other) = default;
        TextLine& operator =(const TextLine& other) = default;
        TextLine& operator =(TextLine&& other) = default;

        template<typename T>
        void SetType(const T& prefix) { Type = hash_(prefix); }

        std::string_view Rest(const size_t fromIndex = 1)
        {
            if (Params.size() <= fromIndex)
                return {};
            const auto lenTotal = Line.length();
            const auto beginLine = Line.data(), beginRest = Params[fromIndex].data();
            return std::string_view(beginRest, lenTotal - (beginRest - beginLine));
        }

        u16string GetUString(const size_t index)
        {
            if (Params.size() <= index)
                return u"";
            return common::str::to_u16string(Params[index], charset);
        }

        u16string ToUString() { return common::str::to_u16string(Line, charset); }

        template<size_t N>
        int8_t ParseInts(const uint8_t idx, int32_t(&output)[N])
        {
            int8_t cnt = 0;
            str::SplitAndDo(Params[idx], '/', [&cnt, &output](const char *substr, const size_t len)
            {
                if (cnt < N)
                {
                    if (len == 0)
                        output[cnt++] = 0;
                    else
                        output[cnt++] = atoi(substr);
                }
            });
            return cnt;
        }

        template<size_t N>
        int8_t ParseFloats(const uint8_t idx, float(&output)[N])
        {
            str::SplitAndDo(Params[idx], '/', [&cnt, &output](const char *substr, const size_t len)
            {
                if (cnt < N)
                {
                    if (len == 0)
                        output[cnt++] = 0;
                    else
                        output[cnt++] = atof(Params[idx]);
                }
            });
        }

        operator const bool() const
        {
            return Type != "EOF"_hash;
        }
    };

    OBJLoder(const fs::path& fpath_) : FilePath(fpath_)
    {
        using namespace common::file;
        //pre-load data, in case of Acess-Violate while reading file
        file::ReadAll(FilePath, Content);
        Length = Content.size() - 1;
        CurPos = 0;
        chset = uchdet::detectEncoding(Content);
        basLog().debug(u"obj file[{}]--encoding[{}]\n", FilePath.u16string(), getCharsetWName(chset));
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

        str::Split(textLine.Line, [](const char ch)
        {
            return (uint8_t)(ch) < uint8_t(0x21) || (uint8_t)(ch) == uint8_t(0x7f);//non-graph character
        }, textLine.Params, false);
        if (textLine.Params.size() == 0)
            return { chset, "EMPTY" };

        textLine.SetType(textLine.Params[0]);
        return textLine;
    }
};


}