#pragma once

#include "oclRely.h"

namespace oclu
{

namespace detail
{
class OCLUAPI oclPromise_ : public ::common::detail::PromiseResult_<void>
{
    friend class _oclBuffer;
    friend class _oclImage;
    friend class _oclKernel;
protected:
    cl_event eventPoint = nullptr;
    oclPromise_(const cl_event e) : eventPoint(e)
    { }
public:
    oclPromise_(oclPromise_&&);
    ~oclPromise_() override;
    common::PromiseState virtual state() override;
    void wait() override;
    uint64_t ElapseNs() override;
};
}

using oclPromise = std::shared_ptr<detail::oclPromise_>;

}
