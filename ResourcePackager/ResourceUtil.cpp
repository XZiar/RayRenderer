#include "ResourceUtil.h"
#include "3rdParty/cryptopp/sha.h"

#pragma message("Compiling ResourcePackager with crypto++[" STRINGIZE(CRYPTOPP_VERSION) "])")

namespace xziar::respak
{


string ResourceUtil::Hex2Str(const void * data, const size_t size)
{
    constexpr auto ch = "0123456789abcdef";
    string ret;
    ret.reserve(size * 2);
    for (size_t i = 0; i < size; ++i)
    {
        const uint8_t dat = reinterpret_cast<const uint8_t*>(data)[i];
        ret.push_back(ch[dat / 16]);
        ret.push_back(ch[dat % 16]);
    }
    return ret;
}

vector<byte> ResourceUtil::Str2Hex(const string_view& str)
{
    auto conv = [](const char ch) constexpr->uint32_t
    {
        if (ch >= '0'&& ch <= '9')
            return ch - '0';
        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        return UINT32_MAX;
    };
    const auto size = str.size();
    if (size % 2)
        return {};
    vector<byte> ret;
    ret.reserve(size / 2);
    for (size_t i = 0; i < size; i += 2)
    {
        const auto hi = conv(str[i]), lo = conv(str[i + 1]);
        if (hi < 256 && lo < 256)
            ret.push_back(byte((hi << 8) + lo));
        else
            return {};
    }
    return ret;
}

string ResourceUtil::ToBase64(const void* data, const size_t size)
{
    constexpr auto ch = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t* __restrict pdata = reinterpret_cast<const uint8_t*>(data);
    const auto suffix = size % 3, realsize = size - suffix;
    string ret;
    ret.reserve((size + 2) / 3 * 4);
    for (size_t i = 0; i < realsize; i += 3)
    {
        const uint32_t tmp = (pdata[i] << 16) + (pdata[i + 1] << 8) + pdata[i + 2];
        ret.push_back(ch[tmp >> 18]);
        ret.push_back(ch[(tmp >> 12) & 0x3f]);
        ret.push_back(ch[(tmp >> 6) & 0x3f]);
        ret.push_back(ch[tmp & 0x3f]);
    }
    if (suffix == 1)
    {
        const auto tmp = pdata[realsize];
        ret.push_back(ch[tmp >> 2]);
        ret.push_back(ch[(tmp << 4) & 0x3f]);
        ret.append("==");
    }
    else if (suffix == 2)
    {
        const auto tmp = (pdata[realsize] << 16) + (pdata[realsize + 1] << 8);
        ret.push_back(ch[tmp >> 18]);
        ret.push_back(ch[(tmp >> 12) & 0x3f]);
        ret.push_back(ch[(tmp >> 6) & 0x3f]);
        ret.push_back('=');
    }
    return ret;
}

vector<byte> ResourceUtil::FromBase64(const string_view& str)
{
    auto conv = [](const char ch) constexpr->uint32_t
    {
        if (ch >= 'A' && ch <= 'Z')
            return ch - 'A';
        if (ch >= 'a' && ch <= 'z')
            return ch - 'a' + 26;
        if (ch >= '0'&& ch <= '9')
            return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        if (ch == '=') return UINT8_MAX;
        return UINT32_MAX;
    };
    const auto size = str.size();
    if (size % 4)
        return {};
    vector<byte> ret;
    ret.reserve(size / 4 * 3);
    for (size_t i = 0; i + 4 < size; i += 4)
    {
        const auto num1 = conv(str[i]), num2 = conv(str[i + 1]), num3 = conv(str[i + 2]), num4 = conv(str[i + 3]);
        if (num1 < 64 && num2 < 64 && num3 < 64 && num4 < 64)
        {
            ret.push_back(byte((num1 << 2) + (num2 >> 4)));
            ret.push_back(byte((num2 << 4) + (num3 >> 2)));
            ret.push_back(byte((num3 << 6) + num4));
        }
        else
            return {};
    }
    {
        const auto num1 = conv(str[size - 4]), num2 = conv(str[size - 3]), num3 = conv(str[size - 2]), num4 = conv(str[size - 1]);
        if (num1 >= 64 || num2 >= 64 || num3 >= 256 || num4 >= 256 || (num3 >= 64 && num4 < 64))
            return {};
        ret.push_back(byte((num1 << 2) + (num2 >> 4)));
        if (num3 != UINT8_MAX)
        {
            ret.push_back(byte((num2 << 4) + (num3 >> 2)));
            if (num4 != UINT8_MAX)
                ret.push_back(byte((num3 << 6) + num4));
        }
    }
    return ret;
}

bytearray<32> ResourceUtil::SHA256(const void* data, const size_t size)
{
    static_assert(CryptoPP::SHA256::DIGESTSIZE == 32, "SHA256 should generate 32 bytes digest");
    bytearray<32> output;
    CryptoPP::SHA256 sha256;
    sha256.CalculateDigest(reinterpret_cast<CryptoPP::byte*>(output.data()), reinterpret_cast<const CryptoPP::byte*>(data), size);
    return output;
}


}

