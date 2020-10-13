#pragma once

#include "oclRely.h"
#include "XComputeBase/XCompDebug.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu::debug
{
struct NLCLDebugExtension;

OCLUAPI void SetAllowDebug(const NLCLContext& context) noexcept;
xcomp::debug::DebugManager* ExtractDebugManager(const NLCLContext& context) noexcept;


struct oclThreadInfo : public xcomp::debug::WorkItemInfo
{
    uint16_t SubgroupId, SubgroupLocalId;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

