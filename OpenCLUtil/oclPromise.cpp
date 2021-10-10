#include "oclPch.h"
#include "oclPromise.h"
#include "oclPlatform.h"
#include "oclCmdQue.h"
#include "oclUtil.h"
#include "common/ContainerEx.hpp"
#include "common/Linq2.hpp"
#include <algorithm>

namespace oclu
{
using namespace std::string_view_literals;


DependEvents::DependEvents(const common::PromiseStub& pmss)
{
    auto clpmss = pmss.FilterOut<oclPromiseCore>();
    if (clpmss.size() < pmss.size())
        oclLog().warning(u"Some non-ocl promise detected as dependent events, will be ignored!\n");
    std::optional<std::pair<const detail::PlatFuncs*, const oclPlatform_*>> plat;
    for (const auto& clpms : clpmss)
    {
        if (plat)
        {
            if (plat->second != clpms->GetPlatform())
                COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Cannot mixing events from different platforms");
        }
        else
            plat.emplace(clpms->Funcs, clpms->GetPlatform());
        for (const auto& que : clpms->Depends.Queues)
        {
            if (que && !common::container::ContainInVec(Queues, que))
                Queues.push_back(que);
        }
        if (*clpms->Event != nullptr)
        {
            Events.push_back(clpms->Event);
            const auto& que = clpms->Queue;
            if (!common::container::ContainInVec(Queues, que))
                Queues.push_back(que);
        }
    }
    if (plat)
        Funcs = plat->first;
    Events.shrink_to_fit();
}
DependEvents::DependEvents() noexcept
{ }

void DependEvents::FlushAllQueues() const
{
    for (const auto& que : Queues)
        que->Flush();
}


void CL_CALLBACK oclPromiseCore::EventCallback(void* event, [[maybe_unused]]cl_int event_command_exec_status, void* user_data)
{
    auto& self = *reinterpret_cast<common::PmsCore*>(user_data);
    Expects(event == *static_cast<oclPromiseCore&>(self->GetPromise()).Event);
    self->ExecuteCallback();
    delete &self;
}

oclPromiseCore::oclPromiseCore(DependEvents&& depend, void* e, oclCmdQue que, const bool isException) :
    detail::oclCommon(*que), Depends(std::move(depend)), Event(e), Queue(std::move(que)), IsException(isException)
{
    if (const auto it = std::find(Depends.Queues.begin(), Depends.Queues.end(), Queue); it != Depends.Queues.end())
    {
        Depends.Queues.erase(it);
    }
    for (const auto evt : Depends.Events)
        Funcs->clRetainEvent(*evt);
}
oclPromiseCore::~oclPromiseCore()
{
    for (const auto evt : Depends.Events)
        Funcs->clReleaseEvent(*evt);
    if (*Event)
        Funcs->clReleaseEvent(*Event);
}

const oclPlatform_* oclPromiseCore::GetPlatform() noexcept 
{
    return Queue->Device->Platform;
}

void oclPromiseCore::PreparePms()
{
    Depends.FlushAllQueues();
    if (Queue)
        Funcs->clFlush(*Queue->CmdQue);
}

inline void oclPromiseCore::MakeActive(common::PmsCore&& pms)
{
    if (RegisterCallback(pms))
        return;
    common::PromiseProvider::MakeActive(std::move(pms));
}

[[nodiscard]] common::PromiseState oclPromiseCore::GetState() noexcept
{
    using common::PromiseState;
    if (IsException)
        return PromiseState::Error;
    cl_int status;
    const auto ret = Funcs->clGetEventInfo(*Event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
    if (ret != CL_SUCCESS)
    {
        oclLog().warning(u"Error in reading cl_event's status: {}\n", oclUtil::GetErrorString(ret));
        return PromiseState::Invalid;
    }
    if (status < 0)
    {
        oclLog().warning(u"cl_event's status shows an error: {}\n", oclUtil::GetErrorString(status));
        return PromiseState::Error;
    }
    switch (status)
    {
    case CL_QUEUED:     return PromiseState::Unissued;
    case CL_SUBMITTED:  return PromiseState::Issued;
    case CL_RUNNING:    return PromiseState::Executing;
    case CL_COMPLETE:   return PromiseState::Success;
    default:            return PromiseState::Invalid;
    }
}

common::PromiseState oclPromiseCore::WaitPms() noexcept
{
    if (!*Event)
        return common::PromiseState::Invalid;
    if (WaitException)
        return common::PromiseState::Error;
    const auto ret = Funcs->clWaitForEvents(1, reinterpret_cast<const cl_event*>(&Event));
    if (ret != CL_SUCCESS)
    {
        if (Queue) Queue->Finish();
        WaitException = CREATE_EXCEPTION(OCLException, OCLException::CLComponent::Driver, ret, u"wait for event error").InnerInfo();
        return common::PromiseState::Error;
    }
    return common::PromiseState::Executed;
}

bool oclPromiseCore::RegisterCallback(const common::PmsCore& pms)
{
    if (Queue->Context->Version < 11)
        return false;
    const auto ptr = new common::PmsCore(pms);
    /*void (CL_CALLBACK * pfn_notify)(cl_event event,
        cl_int   event_command_status,
        void* user_data)*/
    
    const auto ret = Funcs->clSetEventCallback(*Event, CL_COMPLETE, 
        reinterpret_cast<void (CL_CALLBACK *)(cl_event, cl_int, void*)>(&EventCallback), ptr);
    return ret == CL_SUCCESS;
}

[[nodiscard]] uint64_t oclPromiseCore::ElapseNs() noexcept
{
    if (*Event)
    {
        uint64_t from = 0, to = 0;
        const auto ret1 = Funcs->clGetEventProfilingInfo(*Event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
        const auto ret2 = Funcs->clGetEventProfilingInfo(*Event, CL_PROFILING_COMMAND_END,   sizeof(cl_ulong), &to,   nullptr);
        if (ret1 == CL_SUCCESS && ret2 == CL_SUCCESS)
            return to - from;
    }
    return 0;
}

uint64_t oclPromiseCore::QueryTime(oclPromiseCore::TimeType type) const noexcept
{
    if (*Event)
    {
        cl_profiling_info info = UINT32_MAX;
        switch (type)
        {
        case TimeType::Queued: info = CL_PROFILING_COMMAND_QUEUED; break;
        case TimeType::Submit: info = CL_PROFILING_COMMAND_SUBMIT; break;
        case TimeType::Start:  info = CL_PROFILING_COMMAND_START;  break;
        case TimeType::End:    info = CL_PROFILING_COMMAND_END;    break;
        default: break;
        }
        if (info != UINT32_MAX)
        {
            uint64_t time;
            Funcs->clGetEventProfilingInfo(*Event, info, sizeof(cl_ulong), &time, nullptr);
            return time;
        }
    }
    return 0;
}

std::string_view oclPromiseCore::GetEventName() const noexcept
{
    if (*Event)
    {
        cl_command_type type;
        const auto ret = Funcs->clGetEventInfo(*Event, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &type, nullptr);
        if (ret != CL_SUCCESS)
        {
            oclLog().warning(u"Error in reading cl_event's coommand type: {}\n", oclUtil::GetErrorString(ret));
            return "Error";
        }
        switch (type)
        {
        case CL_COMMAND_NDRANGE_KERNEL:         return "NDRangeKernel"sv;
        case CL_COMMAND_TASK:                   return "Task"sv;
        case CL_COMMAND_NATIVE_KERNEL:          return "NativeKernel"sv;
        case CL_COMMAND_READ_BUFFER:            return "ReadBuffer"sv;
        case CL_COMMAND_WRITE_BUFFER:           return "WriteBuffer"sv;
        case CL_COMMAND_COPY_BUFFER:            return "CopyBuffer"sv;
        case CL_COMMAND_READ_IMAGE:             return "ReadImage"sv;
        case CL_COMMAND_WRITE_IMAGE:            return "WriteImage"sv;
        case CL_COMMAND_COPY_IMAGE:             return "CopyImage"sv;
        case CL_COMMAND_COPY_BUFFER_TO_IMAGE:   return "CopyBufferToImage"sv;
        case CL_COMMAND_COPY_IMAGE_TO_BUFFER:   return "CopyImageToBuffer"sv;
        case CL_COMMAND_MAP_BUFFER:             return "MapBuffer"sv;
        case CL_COMMAND_MAP_IMAGE:              return "MapImage"sv;
        case CL_COMMAND_UNMAP_MEM_OBJECT:       return "UnmapMemObject"sv;
        case CL_COMMAND_MARKER:                 return "Maker"sv;
        case CL_COMMAND_ACQUIRE_GL_OBJECTS:     return "AcquireGLObject"sv;
        case CL_COMMAND_RELEASE_GL_OBJECTS:     return "ReleaseGLObject"sv;
        case CL_COMMAND_READ_BUFFER_RECT:       return "ReadBufferRect"sv;
        case CL_COMMAND_WRITE_BUFFER_RECT:      return "WriteBufferRect"sv;
        case CL_COMMAND_COPY_BUFFER_RECT:       return "CopyBufferRect"sv;
        case CL_COMMAND_USER:                   return "User"sv;
        case CL_COMMAND_BARRIER:                return "Barrier"sv;
        case CL_COMMAND_MIGRATE_MEM_OBJECTS:    return "MigrateMemObject"sv;
        case CL_COMMAND_FILL_BUFFER:            return "FillBuffer"sv;
        case CL_COMMAND_FILL_IMAGE:             return "FillImage"sv;
        case CL_COMMAND_SVM_FREE:               return "SVMFree"sv;
        case CL_COMMAND_SVM_MEMCPY:             return "SVMMemcopy"sv;
        case CL_COMMAND_SVM_MEMFILL:            return "SVMMemfill"sv;
        case CL_COMMAND_SVM_MAP:                return "SVMMap"sv;
        case CL_COMMAND_SVM_UNMAP:              return "SVMUnmap"sv;
        // case CL_COMMAND_SVM_MIGRATE_MEM:        return "SVMMigrateMem"sv;
        default:                                return "Unknown"sv;
        }
    }
    return ""sv;
}
std::optional<common::BaseException> oclPromiseCore::GetException() const
{
    if (WaitException)
        return WaitException->GetException();
    return {};
}


oclCustomEvent::oclCustomEvent(common::PmsCore&& pms, void* evt) : oclPromiseCore({}, evt, {}), Pms(std::move(pms))
{ }
oclCustomEvent::~oclCustomEvent()
{ }

void oclCustomEvent::Init()
{
    Pms->AddCallback([self = std::static_pointer_cast<oclCustomEvent>(shared_from_this())]()
        {
        self->Funcs->clSetUserEventStatus(*self->Event, CL_COMPLETE);
        self->ExecuteCallback();
        });
}


}
