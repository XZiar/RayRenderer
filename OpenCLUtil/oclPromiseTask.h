#pragma once

#include "oclRely.h"


namespace oclu
{

class OCLUAPI oclPromise : public ::common::detail::PromiseResult_<void>
{
    friend class detail::_oclBuffer;
    friend class detail::_oclKernel;
protected:
    cl_event eventPoint = nullptr;
    oclPromise(const cl_event e) : eventPoint(e)
    { }
public:
    oclPromise(oclPromise&&);
    ~oclPromise() override;
    void wait() override;
    uint64_t ElapseNs();
};


}
