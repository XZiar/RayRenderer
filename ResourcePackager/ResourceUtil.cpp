#include "ResourceUtil.h"
#include "3rdParty/cryptopp/sha.h"

#pragma message("Compiling ResourcePackager with crypto++[" STRINGIZE(CRYPTOPP_VERSION) "])")

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
bytearray<32> ResourceUtil::SHA256(const void* data, const size_t size)
{
    static_assert(CryptoPP::SHA256::DIGESTSIZE == 32, "SHA256 should generate 32 bytes digest");
    bytearray<32> output;
    CryptoPP::SHA256 sha256;
    sha256.CalculateDigest(reinterpret_cast<CryptoPP::byte*>(output.data()), reinterpret_cast<const CryptoPP::byte*>(data), size);
    return output;
}


}

