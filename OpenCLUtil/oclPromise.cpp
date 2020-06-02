#include "oclPch.h"
#include "oclPromise.h"
#include "oclUtil.h"
#include "common/Linq2.hpp"

namespace oclu
{
using namespace std::string_view_literals;


DependEvents::DependEvents(std::vector<cl_event>&& events) : Events(std::move(events))
{
    for (const auto evt : Events)
        clRetainEvent(evt);
}
DependEvents::~DependEvents()
{
    for (const auto evt : Events)
        clReleaseEvent(evt);
}
DependEvents DependEvents::Create(const common::PromiseStub& pmss) noexcept
{
    auto clpmss = pmss.FilterOut<oclPromiseCore>();
    auto evts = common::linq::FromIterable(clpmss)
        .Select([](const auto& clpms) { return clpms->GetEvent(); })
        .Where([](const auto& evt) { return evt != nullptr; })
        .ToVector();
    return std::move(evts);
}


oclPromiseCore::oclPromiseCore(PrevType&& prev, const cl_event e, oclCmdQue que)
    : Event(e), Queue(std::move(que)), Prev(std::move(prev)) { }
oclPromiseCore::~oclPromiseCore()
{
    if (Event)
        clReleaseEvent(Event);
}
void oclPromiseCore::Flush()
{
    if (Queue)
        clFlush(Queue->CmdQue);
}
[[nodiscard]] common::PromiseState oclPromiseCore::QueryState() noexcept
{
    using common::PromiseState;
    cl_int status;
    const auto ret = clGetEventInfo(Event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
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
void oclPromiseCore::Wait()
{
    if (Event)
        clWaitForEvents(1, &Event);
}
[[nodiscard]] uint64_t oclPromiseCore::ElapseNs()
{
    if (Event)
    {
        uint64_t from = 0, to = 0;
        const auto ret1 = clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &from, nullptr);
        const auto ret2 = clGetEventProfilingInfo(Event, CL_PROFILING_COMMAND_END,   sizeof(cl_ulong), &to,   nullptr);
        if (ret1 == CL_SUCCESS && ret2 == CL_SUCCESS)
            return to - from;
    }
    return 0;
}
[[nodiscard]] uint64_t oclPromiseCore::ChainedElapseNs()
{
    auto time = ElapseNs();
    switch (Prev.index())
    {
    case 2: return time + std::get<2>(Prev)->ChainedElapseNs();
    case 1:
        for (const auto& prev : std::get<1>(Prev))
        {
            time += prev->ChainedElapseNs();
        }
        return time;
    default:
    case 0: return time;
    }
}
[[nodiscard]] std::pair<std::vector<std::shared_ptr<oclPromiseCore>>, oclPromiseCore::oclEvents>
oclPromiseCore::ParsePms(const common::PromiseStub& pmss) noexcept
{
    auto clpmss = pmss.FilterOut<oclPromiseCore>();
    auto evts = common::linq::FromIterable(clpmss)
        .Select([](const auto& clpms) { return clpms->GetEvent(); })
        .Where([](const auto& evt) { return evt != nullptr; })
        .ToVector();
    if (evts.size() > 0)
        return { std::move(clpmss), std::move(evts) };
    else
        return { std::move(clpmss), {} };
}

uint64_t oclPromiseCore::QueryTime(oclPromiseCore::TimeType type) const noexcept
{
    if (Event)
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
            clGetEventProfilingInfo(Event, info, sizeof(cl_ulong), &time, nullptr);
            return time;
        }
    }
    return 0;
}

std::string_view oclPromiseCore::GetEventName() const noexcept
{
    if (Event)
    {
        cl_command_type type;
        const auto ret = clGetEventInfo(Event, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &type, nullptr);
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

}
