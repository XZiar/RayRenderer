#include "oclPch.h"
#include "oclKernelDebug.h"

namespace oclu
{
using common::simd::VecDataInfo;


oclDebugBlock::oclDebugBlock(common::span<const VecDataInfo> infos, const std::u32string_view formatter, const uint16_t align) :
    Blocks(std::make_unique<DataBlock[]>(infos.size())), ArgIdxs(std::make_unique<uint16_t[]>(infos.size())),
    Formatter(formatter), TotalSize(0), ArgCount(gsl::narrow_cast<uint32_t>(infos.size()))
{
    std::vector<DataBlock> tmp;
    uint16_t offset = 0;
    { 
        uint16_t idx = 0;
        for (const auto info : infos)
        {
            Blocks[idx].Info   = info;
            const auto size = info.Bit * info.Dim0 / 8;
            if (size % align == 0)
            {
                Blocks[idx].Offset = offset;
                offset = static_cast<uint16_t>(offset + size);
            }
            else
            {
                Blocks[idx].Offset = UINT16_MAX;
                tmp.emplace_back(info, idx);
            }
            idx++;
        }
    }
    std::sort(tmp.begin(), tmp.end(), [](const auto& left, const auto& right)
        { 
            return left.Info.Bit == right.Info.Bit ? 
                left.Info.Bit * left.Info.Dim0 > right.Info.Bit * right.Info.Dim0 : 
                left.Info.Bit > right.Info.Bit; 
        });

    for (const auto [info, idx] : tmp)
    {
        Blocks[idx].Offset = offset;
        const auto size = info.Bit * info.Dim0 / 8;
        offset = static_cast<uint16_t>(offset + size);
    }
    TotalSize = (offset + (align - 1)) / align * align;
}


}
