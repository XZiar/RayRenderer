#include "NailangStruct.h"
#include "common/AlignedBase.hpp"
#include <boost/range/adaptor/reversed.hpp>

namespace xziar::nailang
{

MemoryPool::~MemoryPool()
{
    for ([[maybe_unused]] const auto& [ptr, offset, avaliable] : Trunks)
    {
        common::free_align(ptr);
    }
}

common::span<std::byte> MemoryPool::Alloc(const size_t size, const size_t align)
{
    Expects(align > 0);
    for (auto& [ptr, offset, avaliable] : boost::adaptors::reverse(Trunks))
    {
        if (avaliable > size)
        {
            const auto ptr_ = reinterpret_cast<uintptr_t>(ptr + offset + align - 1) / align * align;
            const auto offset_ = reinterpret_cast<std::byte*>(ptr_) - ptr;
            if (offset_ + size <= offset + avaliable)
            {
                const common::span<std::byte> ret(ptr + offset_, size);
                avaliable -= (offset_ - offset) + size;
                offset = offset_ + size;
                return ret;
            }
        }
    }
    if (size > 16 * TrunkSize)
    {
        auto ptr = reinterpret_cast<std::byte*>(common::malloc_align(size, align));
        Trunks.emplace_back(ptr, size, 0);
        return common::span<std::byte>(ptr, size);
    }
    else
    {
        const auto total = (size + TrunkSize - 1) / TrunkSize * TrunkSize;
        auto ptr = reinterpret_cast<std::byte*>(common::malloc_align(total, align));
        Trunks.emplace_back(ptr, size, total - size);
        return common::span<std::byte>(ptr, size);
    }
}

std::pair<size_t, size_t> MemoryPool::Usage() const noexcept
{
    size_t used = 0, unused = 0;
    for (auto& [ptr, offset, avaliable] : Trunks)
        used += offset, unused += avaliable;
    return { used, used + unused };
}


//LateBindVar LateBindVar::Generate(std::u32string_view name, MemoryPool& pool)
//{
//    const auto parts = common::str::SplitStream(name, U'.', true)
//        .Select([=](const std::u32string_view part)
//            {
//                return std::pair{ gsl::narrow_cast<uint32_t>(part.data() - name.data()), gsl::narrow_cast<uint32_t>(part.size()) };
//            })
//        .ToVector();
//    if (parts.size() == 1)
//        return { name, {} };
//    else
//        return { name, pool.CreateArray(parts) };
//}


}
