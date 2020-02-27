#include "SectorsStruct.h"
#include <boost/range/adaptor/reversed.hpp>


namespace xziar::sectorlang
{


common::span<std::byte> MemoryPool::Alloc(const size_t size)
{
    for (auto& [ptr, offset, avaliable] : boost::adaptors::reverse(Trunks))
    {
        if (avaliable > size)
        {
            const common::span<std::byte> ret(ptr + offset, size);
            offset += size, avaliable -= size;
            return ret;
        }
    }
    if (size > 16 * TrunkSize)
    {
        auto ptr = reinterpret_cast<std::byte*>(malloc(size));
        Trunks.emplace_back(ptr, size, 0);
        return common::span<std::byte>(ptr, size);
    }
    else
    {
        const auto total = (size + TrunkSize - 1) / TrunkSize * TrunkSize;
        auto ptr = reinterpret_cast<std::byte*>(malloc(size));
        Trunks.emplace_back(ptr, size, total - size);
        return common::span<std::byte>(ptr, size);
    }
}


}
