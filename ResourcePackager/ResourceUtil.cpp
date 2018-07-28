#include "ResourceUtil.h"
#include "3rdParty/cryptopp/sha.h"

namespace xziar::respak
{


string ResourceUtil::Hex2Str(const void * data, const size_t size)
{
    constexpr auto ch = "0123456789abcdef";
    string ret(size * 2, '\0');
    for (size_t i = 0; i < size; ++i)
    {
        const uint8_t dat = reinterpret_cast<const uint8_t*>(data)[i];
        ret[i * 2] = ch[dat / 16], ret[i * 2 + 1] = ch[dat % 16];
    }
    return ret;
}
string ResourceUtil::TestSHA256(const void* data, const size_t size)
{
    byte output[CryptoPP::SHA256::DIGESTSIZE];
    CryptoPP::SHA256 sha256;
    sha256.CalculateDigest(reinterpret_cast<CryptoPP::byte*>(output), reinterpret_cast<const CryptoPP::byte*>(data), size);
    return Hex2Str(output, sizeof(output));
}


}

